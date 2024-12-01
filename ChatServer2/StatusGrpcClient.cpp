#include "StatusGrpcClient.h"


StatusGrpcClient::StatusGrpcClient()
{
    auto& gCfgMgr = ConfigMgr::GetInstance();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    _pool.reset(new StatusConPool(5, host, port));
}

StatusGrpcClient::~StatusGrpcClient()
{
    std::cout << "StatusGrpcClient destruct" << std::endl;
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    ClientContext contex;
    GetChatServerReq request;
    GetChatServerRsp response;

    request.set_uid(uid);
    auto stub = _pool->getConnection();
    Status status = stub->GetChatServer(&contex, request, &response); // �൱�ڵ��ñ��ط���һ������Զ�̷������ķ���
    // �����������Զ��������ӻ����ӳ�
    Defer defer([&stub, this] {
        _pool->returnConnection(std::move(stub));
    });

    if (!status.ok())
    {
        response.set_error(ErrorCodes::RPCFailed);  
    }
    return response;
}
LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
    ClientContext contex;
    LoginReq request;
    LoginRsp response;

    request.set_uid(uid);
    request.set_token(token);
    auto stub = _pool->getConnection();
    Status status = stub->Login(&contex, request, &response); // �൱�ڵ��ñ��ط���һ������Զ�̷������ķ���
    // �����������Զ��������ӻ����ӳ�
    Defer defer([&stub, this] {
        _pool->returnConnection(std::move(stub));
        });

    if (!status.ok())
    {
        response.set_error(ErrorCodes::RPCFailed);
    }
    return response;
}


