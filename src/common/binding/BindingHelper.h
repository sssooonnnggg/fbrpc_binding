#pragma once

#include <napi.h>

#include "PODTypeBinding.h"
#include "CppStaticBinding.h"
#include "CppClassBinding.h"

template <class T>
class BindingHelperBase
{
public:
    BindingHelperBase(Napi::Env env, Napi::Object& exports)
        : m_env(env), m_exports(exports) {}

    T& begin(const char* className)
    {
        m_currentObj = Napi::Object::New(m_env);
        m_exports.Set(className, m_currentObj);
        return self();
    }

    template<auto F>
    T& addStaticFunction(const char* funcName)
    {
        constexpr auto wrapperCall = T::template Wrapper<F>::call;
        m_currentObj.Set(funcName, Napi::Function::New(m_env, CppStaticBinding<wrapperCall>::call));
        return self();
    }

    T& end()
    {
        m_currentObj = Napi::Object();
        return self();
    }

    template<auto F>
    T& addGlobalFunction(const char* funcName)
    {
        m_exports.Set(funcName, Napi::Function::New(m_env, CppStaticBinding<F>::call));
        return self();
    }

private:
    T& self()
    {
        return static_cast<T&>(*this);
    }

private:
    Napi::Env m_env;
    Napi::Object& m_exports;
    Napi::Object m_currentObj;
};

class BindingHelper : public BindingHelperBase<BindingHelper>
{
public:
    BindingHelper(Napi::Env env, Napi::Object& exports)
        : BindingHelperBase<BindingHelper>(env, exports)
    {}

    template <auto F>
    struct Wrapper;

    template <class Ret, class ... Args, Ret(*F)(Args...)>
    struct Wrapper<F>
    {
        static Ret call(Args&& ... args)
        {
            return F(std::forward<Args>(args)...);
        }
    };
};