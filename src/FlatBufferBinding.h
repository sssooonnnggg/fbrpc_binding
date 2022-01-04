#pragma once

#include <napi.h>

#include "fbrpc/ssFlatBufferRpc.h"
#include "common/binding/BindingHelper.h"
#include "common/node/Node.h"

struct Result
{
	bool result;
	std::string message;
};

namespace PODTypeBinding
{
	template <>
	struct Bind<fbrpc::sTCPOption>
	{
		static constexpr auto Binder = makeBinder(
			"address", &fbrpc::sTCPOption::address,
			"port", &fbrpc::sTCPOption::port
		);
	};

	template <>
	struct Bind<Result>
	{
		static constexpr auto Binder = makeBinder(
			"result", &Result::result,
			"message", &Result::message
		);
	};
}

template <>
struct TypeCheck<fbrpc::sBuffer>
{
	static bool check(const Napi::Value& value)
	{
		return value.IsBuffer();
	}
};

namespace TypeConversion
{
	template <>
	struct JsToCpp<fbrpc::sBuffer>
	{
		static fbrpc::sBuffer convert(const Napi::Value& value)
		{
			Napi::Buffer buffer = value.As<Napi::Buffer<char>>();
			return fbrpc::sBuffer::clone(buffer.Data(), buffer.Length());
		}
	};

	template <>
	struct CppToJs<fbrpc::sBuffer>
	{
		static Napi::Value convert(const Napi::Env& env, const fbrpc::sBuffer& input)
		{
			Napi::Buffer<char> buffer = Napi::Buffer<char>::Copy(env, input.data.get(), input.length);
			return buffer;
		}
	};
}

template <class T>
class Resolver : public Napi::AsyncWorker 
{
public:
	Resolver(Napi::Promise::Deferred promise)
		: Napi::AsyncWorker(promise.Env()), m_promise(promise)
	{
		SuppressDestruct();
	}

	void call(T&& arg)
	{
		m_arg = std::forward<T>(arg);
		Queue();
	}

	void Execute() override {}
	void OnOK() override
	{
		m_promise.Resolve(TypeConversion::CppToJs<T>::convert(m_promise.Env(), m_arg));
	}
private:
	Napi::Promise::Deferred m_promise;
	std::decay_t<T> m_arg;
};

class FlatbufferClient
{
public:
	static Napi::Value connect(fbrpc::sTCPOption option)
	{
		auto promise = Napi::Promise::Deferred::New(Node::getEnv());
		auto resolver = std::make_shared<Resolver<Result>>(promise);

		if (!m_client)
		{
			m_client = fbrpc::sFlatBufferRpcClient::create(std::move(option));

			m_client->on<fbrpc::sError>([resolver](const fbrpc::sError& e)
				{
					resolver->call(Result{ false, e.msg });
				}
			);

			m_client->on<fbrpc::sConnectionEvent>([resolver](const fbrpc::sConnectionEvent& e)
				{
					resolver->call(Result{ true });
				}
			);
		}

		m_client->connect();
		return promise.Promise();
	}

	static fbrpc::sFlatBufferRpcClient* get() 
	{
		if (!m_client || !m_client->isConnected())
			throw std::runtime_error("client is not connected");

		return m_client.get(); 
	}

private:
	inline static std::unique_ptr<fbrpc::sFlatBufferRpcClient> m_client = nullptr;
};