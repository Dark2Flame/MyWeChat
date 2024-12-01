#include "StatusServerServiceImpl.h"
#include "const.h"
#include "ConfigMgr.h"

std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);
    return unique_string;
}

StatusServerServiceImpl::StatusServerServiceImpl()
    :_server_index(0)
{
    //auto& cfg = ConfigMgr::GetInstance();
    //ChatServer server;

    //server.port = cfg["ChatServer1"]["Port"];
    //server.host = cfg["ChatServer1"]["Host"];
    //_servers.push_back(server);

    //server.port = cfg["ChatServer2"]["Port"];
    //server.host = cfg["ChatServer2"]["Host"];
    //_servers.push_back(server);

    auto& cfg = ConfigMgr::GetInstance();
    auto server_list = cfg["Chatservers"]["Names"];

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
        ChatServer server;
        server.name = cfg[word]["Name"];
        server.port = cfg[word]["Port"];
        server.host = cfg[word]["Host"];
        _servers[cfg[word]["Name"]] = server;
    }
}

Status StatusServerServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* response)
{

    auto server = getServer();
    std::cout << "find chat server host: " << server.host << " port: " << server.port << std::endl;
    auto uid = request->uid();
    auto token = generate_unique_string();

    response->set_host(server.host);
    response->set_port(server.port);
    response->set_error(ErrorCodes::Success);
    response->set_token(token);
    _tokens[uid] = token;
    insertToken(uid, token);
    std::cout << "emplace uid: " << uid << " token: " << _tokens[uid] << std::endl;
	return Status::OK;
}

Status StatusServerServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* response)
{
    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string r_token = "";

    bool success = RedisMgr::GetInstance()->Get(uid_str, r_token);
    if (!success)
    {
        response->set_error(ErrorCodes::UidInvalid);
        return Status::OK;
    }

    if (token != r_token)
    {
        std::cout << "Server saved token: " << r_token << std::endl;
        std::cout << "Server recv token: " << token;
        response->set_error(ErrorCodes::TokenInvalid);
        return Status::OK;
    }


    //std::lock_guard<std::mutex> guard(_token_mutex);

    //auto iter = _tokens.find(uid);
    //if (iter == _tokens.end())
    //{
    //    response->set_error(ErrorCodes::UidInvalid);
    //    return Status::OK;
    //}

    //if (iter->second != token)
    //{
    //    std::cout << "Server saved token: " << iter->second << std::endl;
    //    std::cout << "Server recv token: " << token;
    //    response->set_error(ErrorCodes::TokenInvalid);
    //    return Status::OK;
    //}

    response->set_error(ErrorCodes::Success);
    response->set_uid(uid);
    response->set_token(token);
    return Status::OK;
}

ChatServer StatusServerServiceImpl::getServer()
{
    //std::lock_guard<std::mutex> lock(_token_mutex);
    //auto& server = _servers[_server_index];
    //_server_index++;
    //if (_server_index == _servers.size())
    //{
    //    _server_index = 0; // 实现循环取用
    //}
    //return server;
    
    std::lock_guard<std::mutex> lock(_server_mutex);
    auto minServer = _servers.begin()->second;
    auto login_count = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
    // 在Redis中无此 ChatServer 的连接数记录
    if (login_count.empty())
    {
        // 将此ChatServer连接数设置为默认值——最大
        minServer.con_count = INT_MAX;
    }
    else
    {
        minServer.con_count = stoi(login_count);
    }
    for (auto& server : _servers)
    {
        if (server.second.name == minServer.name)
        {
            continue;
        }
        // 更新_servers中各个server的连接数
        auto s_count = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
        if (s_count.empty())
        {
            // 将此ChatServer连接数设置为默认值——最大
            server.second.con_count = INT_MAX;
        }
        else
        {
            server.second.con_count = stoi(s_count);
        }

        if (server.second.con_count < minServer.con_count)
        {
            minServer = server.second;
        }
    }
    return minServer;
}

void StatusServerServiceImpl::insertToken(int uid, std::string token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;

    RedisMgr::GetInstance()->SetEx(token_key, token, 60 * 3);
}
