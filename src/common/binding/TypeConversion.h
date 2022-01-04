#pragma once

#include <type_traits>
#include <tuple>
#include <optional>

#include <windows.h>
#include <napi.h>

#include "CallbackWrapper.h"

template<class T>
struct DeduceFunctionType;

namespace TypeConversion
{
    //
    // type conversion c++ => js
    //

    template<class T, class Enable = void>
    struct CppToJs
    {
        static Napi::Value convert(const Napi::Env& env, const T& value)
        {
            return PODTypeBinding::Get<T>.toJs(env, value);
        }
    };

    template<>
    struct CppToJs<Napi::Value>
    {
        static Napi::Value convert(const Napi::Env& env, const Napi::Value& value)
        {
            return value;
        }
    };
        
    template<class T>
    struct CppToJs<T, std::enable_if_t<
        std::is_integral_v<std::decay_t<T>> || std::is_floating_point_v<std::decay_t<T>> || std::is_enum_v<std::decay_t<T>>
        >>
    {
        static Napi::Value convert(const Napi::Env& env, const T& value)
        {
            if constexpr (std::is_same_v<bool, std::decay_t<T>>)
                return Napi::Boolean::New(env, value);
            else
                return Napi::Number::New(env, static_cast<double>(value));
        }
    };
    
    template<>
    struct CppToJs<HWND>
    {
        static Napi::Value convert(const Napi::Env& env, const HWND& value)
        {
            return Napi::BigInt::New(env, reinterpret_cast<uint64_t>(value));
        }
    };

    template<>
    struct CppToJs<std::string>
    {
        static Napi::Value convert(const Napi::Env& env, const std::string& value)
        {
            return Napi::String::New(env, value);
        }
    };

    template <class T>
    struct CppToJs<std::tuple<std::unique_ptr<T[]>, int>>
    {
        static Napi::Value convert(const Napi::Env& env, const std::tuple<std::unique_ptr<T[]>, int>& value)
        {
            const auto& [cppBuffer, size] = value;
            Napi::Buffer<T> buffer = Napi::Buffer<T>::Copy(env, cppBuffer.get(), size);
            return buffer;
        }
    };

    template<>
    struct CppToJs<const char*>
    {
        static Napi::Value convert(const Napi::Env& env, const char* value)
        {
            return Napi::String::New(env, value);
        }
    };

    template<class T, class Alloc>
    struct CppToJs<std::vector<T, Alloc>>
    {
        static Napi::Value convert(const Napi::Env& env, const std::vector<T, Alloc>& value)
        {
            auto vec = Napi::Array::New(env, value.size());
            for (std::uint32_t i = 0; i < value.size(); ++i)
                vec[i] = TypeConversion::CppToJs<T>::convert(env, value[i]);
            return vec;
        }
    };


	template<class T, class Alloc>
	struct CppToJs<std::list<T, Alloc>>
	{
		static Napi::Value convert(const Napi::Env& env, const std::list<T, Alloc>& value)
		{
			auto vec = Napi::Array::New(env, value.size());
            std::uint32_t i = 0;
			for (auto it = value.begin(); it != value.end(); ++it)
				vec[i++] = TypeConversion::CppToJs<T>::convert(env, *it);
			return vec;
		}
	};

    template<class ... Args>
    struct CppToJs<std::tuple<Args...>>
    {
        static Napi::Value convert(const Napi::Env& env, const std::tuple<Args...>& tpl)
        {
            auto vec = Napi::Array::New(env, sizeof...(Args));
            fillVec(env, vec, tpl, std::make_index_sequence<sizeof...(Args)> {});
            return vec;
        }

        template<class Tuple, std::size_t ... I>
        static void fillVec(const Napi::Env& env, Napi::Array& vec, Tuple&& tpl, std::index_sequence<I...>)
        {
            ((vec[static_cast<uint32_t>(I)] = 
                TypeConversion::CppToJs<std::tuple_element_t<I, std::decay_t<Tuple>>>::convert(env, std::get<I>(tpl))), ...);
        }
    };

    template<class T>
    struct CppToJs<std::optional<T>>
    {
        static Napi::Value convert(const Napi::Env& env, const std::optional<T>& value)
        {
            if (value.has_value())
                return CppToJs<T>::convert(env, *value);
            else
                return env.Null();
        }
    };

    //
    // type conversion : js => c++
    //
    template<class T, class Enable = void>
    struct JsToCpp
    {
        static T convert(const Napi::Value& value)
        {
            return PODTypeBinding::Get<T>.template fromJs<T>(value);
        }
    };

    template<>
    struct JsToCpp<Napi::Value>
    {
        static Napi::Value convert(const Napi::Value& value)
        {
            return value;
        }
    };

    template<class T>
    struct JsToCpp<std::tuple<std::unique_ptr<T[]>, int>>
    {
        static std::tuple<std::unique_ptr<T[]>, int> convert(const Napi::Value& value)
        {
            Napi::Buffer buf = value.As<Napi::Buffer<T>>();
            std::unique_ptr<T[]> array(new T[buf.Length()]);
            memcpy(array.get(), buf.Data(), buf.Length() * sizeof(T));
            return std::make_tuple(std::move(array), buf.Length());
        }
    };

    template<class ... Args>
    struct JsToCpp<std::tuple<Args...>>
    {
        static std::tuple<Args...> convert(const Napi::Value& value)
        {
            constexpr auto length = sizeof...(Args);
            return convertImpl(value, std::make_index_sequence<length>{});
        }

        template <std::size_t ... I>
        static std::tuple<Args...> convertImpl(const Napi::Value& value, std::index_sequence<I...>)
        {
            Napi::Array array = value.As<Napi::Array>();
            return std::make_tuple(JsToCpp<Args>::convert(array[static_cast<uint32_t>(I)])...);
        }
    };

    template<class T>
    struct JsToCpp<T, std::enable_if_t<std::is_integral_v<T>>>
    {
        static T convert(const Napi::Value& value)
        {
            if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>)
                return static_cast<T>(value.As<Napi::Number>().Uint32Value());
            else if constexpr (std::is_same_v<T, bool>)
                return value.As<Napi::Boolean>().Value();
            else
                return static_cast<T>(value.As<Napi::Number>().Uint32Value());
        }
    };

    template<class T>
    struct JsToCpp<T, std::enable_if_t<std::is_floating_point_v<T>>>
    {
        static T convert(const Napi::Value& value)
        {
            return value.As<Napi::Number>().FloatValue();
        }
    };

    template<>
    struct JsToCpp<HWND>
    {
        static HWND convert(const Napi::Value& value)
        {
            bool loss = false;
            return reinterpret_cast<HWND>(value.As<Napi::BigInt>().Uint64Value(&loss));
        }
    };

    template<class T>
    struct JsToCpp<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        static T convert(const Napi::Value& value)
        {
            return static_cast<T>(value.As<Napi::Number>().Int32Value());
        }
    };

    template<>
    struct JsToCpp<std::string>
    {
        static std::string convert(const Napi::Value& value)
        {
            return value.As<Napi::String>().Utf8Value();
        }
    };

    template<class Ret, class ... Args>
    struct JsToCpp<std::function<Ret(Args...)>>
    {
        static std::function<Ret(Args...)> convert(const Napi::Value& value)
        {
            return CallbackWrapper::create<Ret, Args...>(value.As<Napi::Function>());
        }
    };

    template<>
    struct JsToCpp<Napi::Function>
    {
        static Napi::Function convert(const Napi::Value& value)
        {
            return value.As<Napi::Function>();
        }
    };

    template<class T, class Alloc>
    struct JsToCpp<std::vector<T, Alloc>>
    {
        static std::vector<T, Alloc> convert(const Napi::Value& value)
        {
            if (value.IsNull() || value.IsUndefined())
                return {};

            std::vector<T, Alloc> result;
            Napi::Array arr = value.As<Napi::Array>();
            for (uint32_t i = 0; i < arr.Length(); ++i)
            {
                Napi::Value element = arr[i];
                result.push_back(JsToCpp<T>::convert(element));
            }
            return result;
        }
    };

    template<class T>
    struct JsToCpp<std::unordered_set<T>>
    {
        static std::unordered_set<T> convert(const Napi::Value& value)
        {
            if (value.IsNull() || value.IsUndefined())
                return {};

            std::unordered_set<T> result;
            Napi::Array arr = value.As<Napi::Array>();
            for (uint32_t i = 0; i < arr.Length(); ++i)
            {
                Napi::Value element = arr[i];
                result.insert(JsToCpp<T>::convert(element));
            }
            return result;
        }
    };

    template<class T>
    struct JsToCpp<std::optional<T>>
    {
        static std::optional<T> convert(const Napi::Value& value)
        {
            if (value.IsUndefined() || value.IsNull())
                return {};
            else
                return JsToCpp<T>::convert(value);
        }
    };
}
