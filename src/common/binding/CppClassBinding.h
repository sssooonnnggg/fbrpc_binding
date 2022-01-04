#pragma once

#include <tuple>
#include <optional>
#include <utility>
#include <cstdint>
#include <vector>

#include <windows.h>
#include <napi.h>

#include "TypeCheck.h"
#include "TypeConversion.h"
#include "TypeDecay.h"
#include "CallbackWrapper.h"
#include "CppBinding.h"

template<auto F, class C, class R, class ...Args>
struct CppClassBindingImpl
{
    static Napi::Value call(C* instance, const Napi::CallbackInfo& info)
    {
        try
        {
            Napi::Env env = info.Env();

            constexpr auto parameterCount = sizeof...(Args);
            if constexpr (parameterCount > 0)
            {
                CppBinding::checkArgCountAndType<Args...>(info);
                return CppBinding::invoke<C, F, R, Args...>(instance, info);
            }
            else
            {
                if constexpr (std::is_void_v<R>)
                {
                    (instance->*F)();
                    return env.Null();
                }
                else
                {
                    R ret = (instance->*F)();
                    return TypeConversion::CppToJs<typename std::decay_t<R>>::convert(env, ret);
                }
            }
        }
        catch (std::runtime_error& err)
        {
            throw Napi::Error::New(info.Env(), err.what());
        }

    }
};

template<auto F>
struct CppClassBinding;

template<class C, class R, class ...Args, auto (C::*F)(Args...) ->R>
struct CppClassBinding<F>
{
    static Napi::Value call(C* instance, const Napi::CallbackInfo& info)
    {
        return CppClassBindingImpl<F, C, R, Args...>::call(instance, info);
    }
};

template<class C, class R, class ...Args, auto (C::* F)(Args...) const ->R>
struct CppClassBinding<F>
{
    static Napi::Value call(C* instance, const Napi::CallbackInfo& info)
    {
        return CppClassBindingImpl<F, C, R, Args...>::call(instance, info);
    }
};
