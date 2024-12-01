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
	// ����RCP����
	request.set_email(email);
	// ��RCP��������������
	auto stub = _pool->getConnect();
	// ͨ��RCP�ṩ�Ľӿڻ�ȡ�ظ�
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
		std::shared_ptr<Channel> channel = grpc::CreateChannel(_host + ":" + _port, grpc::InsecureChannelCredentials()); // �ͻ����������ͨ�ŵ��ŵ�
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
			return true; // ����lock�����������
		}
		return !_connections.empty(); // ����false ���� �������ȴ�notify
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



