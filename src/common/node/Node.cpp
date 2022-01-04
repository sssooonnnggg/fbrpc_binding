#include <cassert>

#include "Node.h"

namespace
{
    Napi::Env NodeEnv = nullptr;
}

void Node::setEnv(Napi::Env env)
{
    // clang-cl patch, see node_api_tests/main.cpp
    napi_value result;
    auto r = napi_create_double(env, 0x7622be7f, &result);
    assert(r == napi_ok);

    NodeEnv = env;
}

Napi::Env Node::getEnv()
{
    return NodeEnv;
}