#pragma once

#include <iostream>
#include <type_traits>
#include <functional>
#include <napi.h>

namespace PODTypeBinding
{

    template <class T>
    struct DeduceGetter;

    template <class Class, class Type>
    struct DeduceGetter<Type (Class::*)()>
    {
        using ClassType = Class;
        using RetType = Type;
    };

    template <class Class, class Type>
    struct DeduceGetter<Type (Class::*)() const>
    {
        using ClassType = Class;
        using RetType = Type;
    };

    template <class T>
    struct DeduceAccessor;

    template <class Class, class Type>
    struct DeduceAccessor<Type(Class::*)>
    {
        using ClassType = Class;
        using RetType = Type;
    };

    template <class T, class Enable = void>
    struct IsGetterSetterTuple : std::false_type
    {
    };

    template <class Getter, class Setter>
    struct IsGetterSetterTuple<
        std::tuple<Getter, Setter>, 
        std::enable_if_t<std::is_member_function_pointer_v<Getter> && std::is_member_function_pointer_v<Setter>>
    > : std::true_type
    {
        using RetType = typename DeduceGetter<Getter>::RetType;
        using ClassType = typename DeduceGetter<Getter>::ClassType;
        static_assert(std::is_invocable_r_v<RetType, Getter, ClassType>, "invalid getter!");
        static_assert(std::is_invocable_v<Setter, ClassType, RetType>, "invalid setter!");
    };

    template <class Getter, class Setter>
    using GetterSetter = std::tuple<Getter, Setter>;

    template <class Getter, class Setter>
    constexpr GetterSetter<Getter, Setter> bindGetterSetter(Getter getter, Setter setter)
    {
        return std::make_tuple(getter, setter);
    }

    template <class Accessor, class Enable = void>
    struct KeyAccessor;

    template <class Accessor>
    struct KeyAccessor<Accessor, std::enable_if_t<std::is_member_object_pointer_v<Accessor>>>
    {
        const char *key;
        Accessor value;

        using MemberType = typename DeduceAccessor<Accessor>::RetType;
        using ClassType = typename DeduceAccessor<Accessor>::ClassType;

        void toJs(Napi::Object &obj, const ClassType &instance) const
        {
            obj[key] = TypeConversion::CppToJs<MemberType>::convert(obj.Env(), instance.*value);
        }

        bool check(const Napi::Object &obj) const
        {
            if (!obj.Has(key))
                throw Napi::TypeError::New(obj.Env(), fmt::format("can't find key : {} from object", key));

            Napi::Value value = obj[key];
            return TypeCheck<MemberType>::check(value);
        }

        void fromJs(const Napi::Object &obj, ClassType &instance) const
        {
            Napi::Value v = obj[key];
            (instance.*value) = TypeConversion::JsToCpp<MemberType>::convert(v);
        }
    };

    template <class Accessor>
    struct KeyAccessor<Accessor, std::enable_if_t<IsGetterSetterTuple<Accessor>::value>>
    {
        const char *key;
        Accessor value;

        using Getter = std::tuple_element_t<0, Accessor>;
        using Setter = std::tuple_element_t<1, Accessor>;
        using MemberType = typename DeduceGetter<Getter>::RetType;
        using ClassType = typename DeduceGetter<Getter>::ClassType;

        void toJs(Napi::Object &obj, const ClassType &instance) const
        {
            obj[key] = TypeConversion::CppToJs<MemberType>::convert(obj.Env(), (instance.*(std::get<0>(value)))());
        }

        bool check(const Napi::Object &obj) const
        {
            if (!obj.Has(key))
                throw Napi::TypeError::New(obj.Env(), fmt::format("can't find key : {} from object", key));

            Napi::Value value = obj[key];
            return TypeCheck<MemberType>::check(value);
        }

        void fromJs(const Napi::Object &obj, ClassType &instance) const
        {
            Napi::Value v = obj[key];
            (instance.*(std::get<1>(value)))(TypeConversion::JsToCpp<MemberType>::convert(v));
        }
    };

    template <class... Args>
    struct Binder;

    template <class... Key, class... Accessor>
    struct Binder<KeyAccessor<Key, Accessor>...>
    {
        using Tuple = std::tuple<KeyAccessor<Key, Accessor>...>;
        Tuple t;

        template <std::size_t... I>
        bool check(const Napi::Object &obj, std::index_sequence<I...>) const
        {
            return (std::get<I>(t).check(obj) && ...);
        }

        bool check(const Napi::Value &value) const
        {
            if (!value.IsObject())
                return false;

            Napi::Object obj = value.ToObject();
            return check(obj, std::make_index_sequence<sizeof...(Key)>{});
        }

        template <class T, std::size_t... I>
        Napi::Value toJs(const Napi::Env &env, const T &instance, std::index_sequence<I...>) const
        {
            Napi::Object obj = Napi::Object::New(env);
            (std::get<I>(t).toJs(obj, instance), ...);
            return obj;
        }

        template <class T>
        Napi::Value toJs(const Napi::Env &env, const T &instance) const
        {
            return toJs(env, instance, std::make_index_sequence<sizeof...(Key)>{});
        }

        template <class T, std::size_t... I>
        void fromJs(const Napi::Object &obj, T &instance, std::index_sequence<I...>) const
        {
            (std::get<I>(t).fromJs(obj, instance), ...);
        }

        template <class T>
        T fromJs(const Napi::Value &value) const
        {
            Napi::Object obj = value.ToObject();
            T result;
            fromJs(obj, result, std::make_index_sequence<sizeof...(Key)>{});
            return result;
        }
    };

    template <class A, class B, class... Args>
    constexpr auto make(A key, B acc, Args... args)
    {
        if constexpr (sizeof...(args) > 0)
            return std::tuple_cat(std::make_tuple(KeyAccessor<B>{key, acc}), make(args...));
        else
            return std::make_tuple(KeyAccessor<B>{key, acc});
    }

    template <class... Key, class... Accessor>
    constexpr Binder<KeyAccessor<Accessor>...> makeBinder(std::tuple<KeyAccessor<Accessor>...> t)
    {
        return Binder<KeyAccessor<Accessor>...>{t};
    }

    template <class... Args>
    constexpr auto makeBinder(Args... args)
    {
        return makeBinder(make(args...));
    }

    template <class T, class Enable = void>
    struct Bind;

    template <class T>
    const auto &Get = Bind<T>::Binder;
}
