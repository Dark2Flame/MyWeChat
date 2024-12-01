#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPCConnectPool
{
public:
	RPCConnectPool(const size_t& pool_size, const std::string& host, const std::string& port);
	~RPCConnectPool();
	void Close();
	std::unique_ptr<VarifyService::Stub> getConnect();
	void returnConnect(std::unique_ptr<VarifyService::Stub> connect);
private:
	atomic<bool> _b_stop;
	size_t _pool_size;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<VarifyService::Stub>> _connections;
	std::condition_variable _cond;
	std::mutex _mutex;

};

class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:

	GetVarifyRsp GetVarifyCode(std::string email);

private:
	VerifyGrpcClient();

	std::unique_ptr<VarifyService::Stub> stub_; // 客户端与gRPC服务器通信的媒介

	std::unique_ptr<RPCConnectPool> _pool;
};

