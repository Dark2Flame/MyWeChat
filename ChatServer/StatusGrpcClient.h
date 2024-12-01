#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "message.pb.h"
#include "message.grpc.pb.h"
#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusConPool
{
public:
	StatusConPool(std::size_t poolSize, std::string host, std::string port)
		:_pool_size(poolSize), _host(host), _port(port), _b_stop(false)
	{
		for (size_t i = 0; i < poolSize; i++)
		{
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			_connections.push(StatusService::NewStub(channel));
		}
	}

	~StatusConPool()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		Close();
		while (!_connections.empty())
		{
			_connections.pop(); // ��ϵͳ�������ӵ��ڴ�
		}
	}

	std::unique_ptr<StatusService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		// �����������������ĵȴ����ȴ���ʱ���������
		_cond.wait(lock, [this] {
			if (_b_stop)
			{
				return true;
			}
			return !_connections.empty();
		});

		if (_b_stop)
		{
			// ��ȡ������ʱ���յ�ֹͣ�źžͷ���һ����ָ��
			return nullptr;
		}

		auto con = std::move(_connections.front());
		_connections.pop();
		return con;// ���� unique_ptr Ĭ��������Ȩת��Ҳ���� move
	}

	void returnConnection(std::unique_ptr<StatusService::Stub> con)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (_b_stop)
		{
			return;
		}
		_connections.push(std::move(con));
		_cond.notify_one();
	}

	void Close()
	{
		_b_stop = true; // ����ֹͣ�ź�
		_cond.notify_all(); // ֪ͨ���еȴ��߳�
	}
private:
	std::atomic<bool> _b_stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mutex;
	std::condition_variable _cond;
	std::queue<std::unique_ptr<StatusService::Stub>> _connections;
	
};

class StatusGrpcClient: public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient();
	GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusConPool> _pool;
};

