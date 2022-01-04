#pragma once

#include <type_traits>
#include <vector>
#include <list>
#include <unordered_set>
#include <napi.h>

#include <Windows.h>

template<class T>
struct DeduceFunctionType;

template<class T, class Enable = void>
struct TypeCheck
{
    static bool check(const Napi::Value& value)
    {
        return PODTypeBinding::Get<T>.check(value);
    }
};

template<>
struct TypeCheck<Napi::Function>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsFunction();
    }
};


template<>
struct TypeCheck<Napi::Value>
{
    static bool check(const Napi::Value& value)
    {
        return true;
    }
};

template<>
struct TypeCheck<HWND>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsBigInt();
    }
};

template<class T>
struct TypeCheck<T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
{
    static bool check(const Napi::Value& value)
    {
        if constexpr (std::is_same_v<T, bool>)
            return value.IsBoolean();
        else
            return value.IsNumber();
    }
};

template<class T>
struct TypeCheck<T, std::enable_if_t<std::is_enum_v<T>>>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsNumber();
    }
};

template<class T>
struct TypeCheck<std::tuple<std::unique_ptr<T[]>, int>>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsBuffer();
    }
};

template <class ...Args>
struct TypeCheck<std::tuple<Args...>>
{
    static bool check(const Napi::Value& value)
    {
        if (!value.IsArray())
            return false;

        Napi::Array array = value.As<Napi::Array>();
        constexpr auto length = sizeof...(Args);
        if (length != array.Length())
            return false;

        return checkTuple(array, std::make_index_sequence<length>{});
    }

    template <size_t ... I>
    static bool checkTuple(const Napi::Array& value, std::index_sequence<I...>)
    {
        using Tuple = std::tuple<Args...>;
        return (... && TypeCheck<std::tuple_element_t<I, Tuple>>::check(Napi::Value((value[static_cast<uint32_t>(I)]))));
    }
};

template<class T>
struct TypeCheck<T, std::enable_if_t<std::is_same_v<std::string, T>>>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsString();
    }
};

template<class Ret, class ... Args>
struct TypeCheck<std::function<Ret(Args...)>>
{
    static bool check(const Napi::Value& value)
    {
        return value.IsFunction();
    }
};

template<class T>
struct TypeCheckContainer
{
    static bool check(const Napi::Value& value)
    {
        if (value.IsNull() || value.IsUndefined())
            return true;

        if (!value.IsArray())
            return false;

        auto arr = value.As<Napi::Array>();
        auto arrLen = arr.Length();
        for (uint32_t i = 0; i < arrLen; ++i)
        {
            Napi::Value element = arr[i];
            if (!TypeCheck<T>::check(element))
                return false;
        }
        return true;
    }
};

template<class T, class Alloc>
struct TypeCheck<std::vector<T, Alloc>> : TypeCheckContainer<T> {};

template<class T, class Alloc>
struct TypeCheck<std::list<T, Alloc>> : TypeCheckContainer<T> {};

template<class T>
struct TypeCheck<std::unordered_set<T>> : TypeCheckContainer<T> {};
