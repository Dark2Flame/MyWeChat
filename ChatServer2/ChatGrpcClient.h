#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include <grpcpp/grpcpp.h> 
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include "const.h"
#include "data.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <memory>
#include <unordered_map>
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;

class ChatConPool
{
public:
	ChatConPool(size_t poolSize, std::string host, std::string port)
		:_pool_size(poolSize), _host(host), _port(port), _b_stop(false)
	{
		for (size_t i = 0; i < poolSize; i++)
		{
			// channel 是管道， stub是介质 ，客户端利用stub使用服务器的服务
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			_connects.push(ChatService::NewStub(channel));
		}
	}
	std::unique_ptr<ChatService::Stub> getConnect()
	{
		// 条件变量和unique_lock搭配
		std::unique_lock<std::mutex> lock(_mutex);
		_cond.wait(lock, [this](){
			if (_b_stop)
			{
				return true;
			}
			return !(_connects.empty());
		});
		
		if (_b_stop)
		{
			return nullptr;
		}
		
		auto stub = std::move(_connects.front());
		_connects.pop();
		return stub;

		//_connects.front() 实际上是在返回 std::unique_ptr 的一个引用
		//return _connects.front();
	}

	void returnConnect(std::unique_ptr<ChatService::Stub> stub)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_b_stop)
		{
			return;
		}
		_connects.push(std::move(stub));
		_cond.notify_one();
	}
	void Close()
	{
		_b_stop = true;
		_cond.notify_all();
	}

	~ChatConPool()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		Close();
		while (!(_connects.empty()))
		{
			_connects.pop();
		}
	}
private:
	size_t _pool_size;
	std::string _host;
	std::string _port;

	std::atomic_bool _b_stop;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue<std::unique_ptr<ChatService::Stub>> _connects;
};


class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient();
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
private:
	ChatGrpcClient();
	std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};


