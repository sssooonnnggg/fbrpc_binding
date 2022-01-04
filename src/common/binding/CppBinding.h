#pragma once

#include <napi.h>
#include <spdlog/fmt/fmt.h>

#include "TypeCheck.h"
#include "TypeConversion.h"
#include "TypeDecay.h"

namespace CppBinding
{
    template<class ...Args>
    static void checkArgCountAndType(const Napi::CallbackInfo& info)
    {
        Napi::Env env = info.Env();
        constexpr auto parameterCount = sizeof...(Args);
        if (info.Length() != parameterCount)
            throw Napi::TypeError::New(env, fmt::format("Wrong number of arguments, expected : {}, provided : {}", parameterCount, info.Length()));

        checkInput<GetDecayType<std::decay_t<Args>>...>(info, std::make_index_sequence<parameterCount>{});
    }

    template<class ... Args, std::size_t ... Index>
    static void checkInput(const Napi::CallbackInfo& info, std::index_sequence<Index...>)
    {
        int currentIndex = 0;
        std::string extraErrorInfo;
        bool checkResult;
        try
        {
            checkResult = ((currentIndex++, TypeCheck<Args>::check(info[Index])) && ...);
        }
        catch (const Napi::TypeError& e)
        {
            checkResult = false;
            extraErrorInfo = e.Message();
        }
        
        if (!checkResult)
            throw Napi::TypeError::New(
                info.Env(), 
                fmt::format("Wrong argument type, argument index {}\n\t{}", currentIndex, extraErrorInfo));
    }

    template<class ... Args, std::size_t ... Index>
    static std::tuple<Args...> parseJsInput(const Napi::CallbackInfo& info, std::index_sequence<Index...>)
    {
        return std::make_tuple<Args...>(TypeConversion::JsToCpp<std::decay_t<Args>>::convert(info[Index])...);
    }

    template<class ...Args>
    static auto getDecayInputs(const Napi::CallbackInfo& info)
    {
        //
        // because node can only get std::string(not const char*) from javascript
        // here we do type decay, eg : cosnt char* => std::string
        // if function signature is void foo(const char*) after decay we get a tuple : std::tuple<std::string>
        //
        return parseJsInput<GetDecayType<std::remove_reference_t<Args>>...>(info, std::make_index_sequence<sizeof...(Args)>{});
    }

    template<auto Indice, class Tuple, class Tuple2>
    static auto getUnDecayOutput(Tuple&& tpl, Tuple2&&)
    {
        // here we do undecay opecation : std::string => const char*
        // so we can call foo(const char*) correctly
        return TypeDecay<typename std::tuple_element<Indice, Tuple2>::type>::forward(
            std::get<Indice>(std::forward<Tuple>(tpl)));
    }

    namespace InvokeDetail
    {
        template <class Callable, class Tuple, class Tuple2, size_t... Indices>
        static auto invokeCppImpl(Callable&& callable, Tuple&& tpl, Tuple2&& originalTpl, std::index_sequence<Indices...>)
        {
            return std::invoke(
                std::forward<Callable>(callable),
                getUnDecayOutput<Indices>(std::forward<Tuple>(tpl), std::forward<Tuple2>(originalTpl))...
            );
        }

        template <class Inst, class Callable, class Tuple, class Tuple2, size_t... Indices>
        static auto invokeCppImpl(Inst* inst, Callable&& callable, Tuple&& tpl, Tuple2&& originalTpl, std::index_sequence<Indices...>)
        {
            return std::invoke(
                std::forward<Callable>(callable),
                inst,
                getUnDecayOutput<Indices>(std::forward<Tuple>(tpl), std::forward<Tuple2>(originalTpl))...
            );
        }
    }

    template <class Inst, class Callable, class Tuple, class Tuple2>
    static auto invokeCpp(Inst* inst, Callable&& callable, Tuple&& tpl, Tuple2&& originalTpl)
    {
        return InvokeDetail::invokeCppImpl(
            inst,
            std::forward<Callable>(callable),
            std::forward<Tuple>(tpl),
            std::forward<Tuple2>(originalTpl),
            std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    template <class Callable, class Tuple, class Tuple2>
    static auto invokeCpp(Callable&& callable, Tuple&& tpl, Tuple2&& originalTpl)
    {
        return InvokeDetail::invokeCppImpl(
            std::forward<Callable>(callable),
            std::forward<Tuple>(tpl),
            std::forward<Tuple2>(originalTpl),
            std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    template<auto F, class R, class ...Args>
    static Napi::Value invoke(const Napi::CallbackInfo& info)
    {
        auto inputs = getDecayInputs<Args...>(info);

        if constexpr (std::is_void_v<R>)
        {
            invokeCpp(F, std::move(inputs), std::tuple<std::remove_reference_t<Args>...>{});
            return info.Env().Null();
        }
        else
        {
            auto ret = invokeCpp(F, std::move(inputs), std::tuple<std::remove_reference_t<Args>...>{});
            return TypeConversion::CppToJs<R>::convert(info.Env(), ret);
        }
    }


    template<class C, auto F, class R, class ...Args>
    static Napi::Value invoke(C* instance, const Napi::CallbackInfo& info)
    {
        auto inputs = getDecayInputs<Args...>(info);

        if constexpr (std::is_void_v<R>)
        {
            invokeCpp(instance, F, std::move(inputs), std::tuple<std::remove_reference_t<Args>...>{});
            return info.Env().Null();
        }
        else
        {
            auto ret = invokeCpp(instance, F, std::move(inputs), std::tuple<std::remove_reference_t<Args>...>{});
            return TypeConversion::CppToJs<R>::convert(info.Env(), ret);
        }
    }
}