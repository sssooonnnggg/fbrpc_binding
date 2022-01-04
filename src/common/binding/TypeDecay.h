#pragma once

#include <string>

template<class T, class Enable = void>
struct TypeDecay
{
    using Type = T;
    static Type forward(T&& input)
    {
        return std::forward<T>(input);
    }
};

template<>
struct TypeDecay<const char*>
{
    using Type = std::string;
    static const char* forward(const std::string& input)
    {
        return input.c_str();
    }
};

template<class T>
using GetDecayType = typename TypeDecay<T>::Type;