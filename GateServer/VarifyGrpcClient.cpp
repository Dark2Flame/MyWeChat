#include "VarifyGrpcClient.h"


VerifyGrpcClient::VerifyGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::GetInstance();
	std::string host_str = gCfgMgr["VarifyServer"]["Host"];
	std::string port_str = gCfgMgr["VarifyServer"]["Port"];
	_pool.reset(new RPCConnectPool(5, host_str, port_str));
}
GetVarifyRsp VerifyGrpcClient::GetVarifyCode(std::string email)
{
	ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	// 创建RCP请求
	request.set_email(email);
	// 与RCP服务器简历连接
	auto stub = _pool->getConnect();
	// 通过RCP提供的接口获取回复
	Status status = stub->GetVarifyCode(&context, request, &reply);

	if (status.ok()) {
		_pool->returnConnect(std::move(stub));
		return reply;
	}
	else {
		_pool->returnConnect(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

RPCConnectPool::RPCConnectPool(const size_t& pool_size, const std::string& host, const std::string& port)
	: _pool_size(pool_size), _host(host), _port(port), _b_stop(false)
{
	for (size_t i = 0; i < _pool_size; i++)
	{
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials()); // 客户端与服务器通信的信道
		_connections.push(VarifyService::NewStub(channel));
	}
}

RPCConnectPool::~RPCConnectPool()
{
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while (!_connections.empty())
	{
		_connections.pop();
	}
}

void RPCConnectPool::Close()
{
	_b_stop = true;
	_cond.notify_all();
}

std::unique_ptr<VarifyService::Stub> RPCConnectPool::getConnect()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this]() {
		if (_b_stop)
		{
			return true; // 解锁lock程序继续运行
		}
		return !_connections.empty(); // 返回false 解锁 程序挂起等待notify
		});
	if (_b_stop)
	{

		return nullptr;
	}
	auto connect = std::move(_connections.front());
	_connections.pop();
	return connect;
}

void RPCConnectPool::returnConnect(std::unique_ptr<VarifyService::Stub> connect)
{
	std::lock_guard<std::mutex> lock(_mutex);
	if(_b_stop)
	{
		return;
	}
	_connections.push(std::move(connect));
	_cond.notify_one();
}



