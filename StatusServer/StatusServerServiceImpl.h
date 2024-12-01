#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

struct ChatServer
{
	std::string name;
	std::string host;
	std::string port;
	int con_count;
};

class StatusServerServiceImpl final: public StatusService::Service
{
public:
	StatusServerServiceImpl();
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* response) override;
	Status Login(ServerContext* context, const LoginReq* request, LoginRsp* response) override;
	ChatServer getServer();
	void insertToken(int uid, std::string token);
private:
	std::unordered_map<std::string, ChatServer> _servers;
	int _server_index;
	std::mutex _token_mutex;
	std::mutex _server_mutex;
	std::map<int, std::string> _tokens;
};


