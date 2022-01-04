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

template<auto F>
struct CppStaticBinding {};

template<class R, class ...Args, auto (*F)(Args...)->R>
struct CppStaticBinding<F>
{
    static Napi::Value call(const Napi::CallbackInfo& info)
    {
        try
        {
            Napi::Env env = info.Env();

            constexpr auto parameterCount = sizeof...(Args);
            if constexpr (parameterCount > 0)
            {
                CppBinding::checkArgCountAndType<Args...>(info);
                return CppBinding::invoke<F, R, Args...>(info);
            }
            else
            {
                if constexpr (std::is_void_v<R>)
                {
                    F();
                    return env.Null();
                }
                else
                {
                    R ret = F();
                    return TypeConversion::CppToJs<R>::convert(env, ret);
                }
            }
        }
        catch (std::runtime_error& err)
        {
            throw Napi::Error::New(info.Env(), err.what());
        }
    }
};