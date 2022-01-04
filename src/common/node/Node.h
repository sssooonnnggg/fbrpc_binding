#pragma once

#include <napi.h>

class Node
{
public:
    static void setEnv(Napi::Env env);
    static Napi::Env getEnv();
};