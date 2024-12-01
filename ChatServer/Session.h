#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <queue>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "MsgNode.h"
#include "const.h"
#include "LogicSystem.h"
#include "Server.h"
using boost::asio::ip::tcp;

class CServer;
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_context &ioc, CServer* pServer);
    ~Session() {
        std::cout << "Destruct session" << std::endl;
    }
    tcp::socket& Socket(){
        return _socket;
    }

    const std::string &GetSessionId() const;
    /*std::string& GetSessionId();*/
    void SetUserId(int uid);
    int GetUserId();
    void Start(); // 开启服务器的监听

    void Send(char* msg,  int max_length, short msg_id);
    void Send(std::string msg, short msgid);
    void stayAlive();
private:

    void handle_read(boost::system::error_code error, size_t bytes_transferred, std::shared_ptr<Session> _self_shared);// 读操作的回调函数
    void handle_read_2(boost::system::error_code error, size_t bytes_transferred, std::shared_ptr<Session> _self_shared);// 读操作的回调函数
    void handle_write(boost::system::error_code error, std::shared_ptr<Session> _self_shared);// 写操作的回调函数
    
    tcp::socket _socket;
    //enum {MAX_LENGTH = 1024 * 2}; //单条消息的最大长度
    char _data[MAX_LENGTH]{}; // 存储消息的字符数组
    std::string _data_t = ""; // 存储消息的字符数组
    CServer* _server;
    std::string _session_id;
    std::queue<std::shared_ptr<MsgNode> > _send_que;
    std::mutex _send_lock;
    std::shared_ptr<RecvNode> _recv_msg_node;
    int _user_uid;
    bool _b_head_parse;
    std::shared_ptr<MsgNode> _recv_head_node;
    boost::asio::steady_timer _timer;
};

class LogicNode
{
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recvnode);
    ~LogicNode() { std::cout << "Logic Node Destructor" << std::endl; }
private:
    std::shared_ptr<Session> _session;
    std::shared_ptr<RecvNode> _recvnode;
};

