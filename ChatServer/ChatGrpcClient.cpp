#include "ChatGrpcClient.h"

ChatGrpcClient::ChatGrpcClient()
{
    auto& cfg = ConfigMgr::GetInstance();
    auto server_list = cfg["PeerServer"]["Servers"];

    std::stringstream ss(server_list);
    std::vector<std::string> words;
    std::string word;

    while (std::getline(ss, word, ','))
    {
        words.push_back(word);
    }

    for (auto& word : words)
    {
        if (cfg[word]["Name"].empty())
        {
            continue;
        }
        _pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
    }
}
AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
    AddFriendRsp rsp;

    Defer defer([this, &rsp, &req]() {
        
        rsp.set_applyuid(req.applyuid());
        rsp.set_touid(req.touid());
        });

    // 从_pools中找到server_ip对应的连接池
    auto pool_it = _pools.find(server_ip);
    if (pool_it == _pools.end())
    {
        return rsp;
    }

    // 从连接池中获取一个连接 记得 RAII 构建Rsp
    auto& pool = pool_it->second;
    auto stub = pool->getConnect();

    // 使用这个连接获取ChatGrpcServer的NotifyAddFriend服务
    ClientContext context;
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    // 记得 RAII 还连接
    Defer defer_recon([this, &stub, &pool]() {
        pool->returnConnect(std::move(stub));
        });
    if (!(status.ok()))
    {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    rsp.set_error(ErrorCodes::Success);
    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
    AuthFriendRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    auto stub = pool->getConnect();
    Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnect(std::move(stub));
        });

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    return false;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue)
{

    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    Defer defer([&rsp, &req]() {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        for (const auto& text_data : req.textmsgs()) {
            TextChatData* new_msg = rsp.add_textmsgs();
            new_msg->set_msgid(text_data.msgid());
            new_msg->set_msgcontent(text_data.msgcontent());
        }

        });

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        return rsp;
    }

    auto& pool = find_iter->second;
    ClientContext context;
    auto stub = pool->getConnect();
    Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    Defer defercon([&stub, this, &pool]() {
        pool->returnConnect(std::move(stub));
        });

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}
ChatGrpcClient::~ChatGrpcClient()
{
}

