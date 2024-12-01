#pragma once
#include <iostream>
#include "Session.h"
#include <boost/asio.hpp>
#include <map>
#include <memory>
using boost::asio::ip::tcp;

class CServer
{
public:
    /**
        * @brief Server 有参构造函数
        * @param ioc 服务的上下文
        * @param port 监听的端口号
        */
    CServer(boost::asio::io_context& ioc, short port) :_ioc(ioc),
        _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server start success on port " << port << std::endl;
        // 成员变量_acceptor用于监听连接，监听所有ipv 4地址 对端口号port的连接
        start_accept();
    }

    void ClearSessions(const std::string& uuid);
private:
    void start_accept(); // 启动监听
    void handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error); // 监听操作的回调函数


    boost::asio::io_context& _ioc;
    tcp::acceptor _acceptor;
    std::mutex _mutex;
    std::map<std::string, std::shared_ptr<Session>> _sessions;
};

