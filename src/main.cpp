#include "generated/FlatBufferRpcBinding_generated.h"

Napi::Object init(Napi::Env env, Napi::Object exports)
{
	Node::setEnv(env);

	BindingHelper helper(env, exports);
	helper.addGlobalFunction<FlatbufferClient::connect>("connect");
	FlatBufferBinding::bind(helper);

	return exports;
}

NODE_API_MODULE(fbrpc_binding, init)