#pragma once

#include <tuple>
#include <optional>
#include <utility>
#include <cstdint>
#include <vector>
#include <atomic>
#include <thread>
#include <unordered_set>
#include <cassert>

#include <windows.h>
#include <napi.h>

#include "common/utils/Singleton.h"

class ThreadSafeFunctionWrapper
{
public:
    ThreadSafeFunctionWrapper(const Napi::ThreadSafeFunction& f) : func(f) {}
    ~ThreadSafeFunctionWrapper() { func.Release(); }
    Napi::ThreadSafeFunction* operator ->() { return &func; }
    Napi::ThreadSafeFunction func;
};

class CallbackWrapper
{
public:
    template <class Ret, class ... Args>
    static std::function<Ret(Args...)> create(const Napi::Function& callback)
    {
        auto threadSafeFunction = Napi::ThreadSafeFunction::New(callback.Env(), callback, "ElectronSafeCallback", 0, 1);
        auto threadSafeFunctionPtr = std::make_shared<ThreadSafeFunctionWrapper>(threadSafeFunction);

        return [threadSafeFunctionPtr](Args&& ... args)
        {
            auto tuple = std::make_tuple(std::forward<Args>(args)...);
            using TupleType = decltype(tuple);
            std::shared_ptr<TupleType> tuplePtr(std::tuple_size_v<TupleType> == 0 ? nullptr : new TupleType(std::move(tuple)));
            (*threadSafeFunctionPtr)->BlockingCall(
                [tuplePtr](Napi::Env env, Napi::Function jsCallback)
                {
                    try
                    {
                        jsCallback.Call(tuplePtr ? tupleToArgs(env, *tuplePtr, std::make_index_sequence<sizeof...(Args)>{}) : std::vector<napi_value>{});
                    }
                    catch (const Napi::Error& error)
                    {
                        error.ThrowAsJavaScriptException();
                    }
                }
            );
        };
    }

    template <class Tuple, std::size_t ... Index>
    static std::vector<napi_value> tupleToArgs(const Napi::Env& env, Tuple&& tuple, std::index_sequence<Index...>)
    {
        return { TypeConversion::CppToJs<std::tuple_element_t<Index, std::decay_t<Tuple>>>::convert(env, std::get<Index>(std::forward<Tuple>(tuple)))... };
    }
};