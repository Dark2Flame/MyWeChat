#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include "ConfigMgr.h"
#include <hiredis.h>
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServerServiceImpl.h"
#include "StatusServer.h"
void RunServer()
{
    // 从配置文件中读取服务器地址
    auto& cfg = ConfigMgr::GetInstance();
    std::string address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
    // 实例化一个服务对象
    StatusServerServiceImpl service;
    // 创建一个builder
    grpc::ServerBuilder builder;
    // 将 adress绑定到builder中
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    // 为builder添加服务
    builder.RegisterService(&service);
    // 使用builder启动服务器
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Statust server start success on port: "<< cfg["StatusServer"]["Port"] << std::endl;
    //优雅退出
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&server](const boost::system::error_code& ec, int signal_number) {
        if (!ec)
        {
            std::cout << "Shutting down server......" << std::endl;
            server->Shutdown();
        }
    });

    // 关闭信号的监测放到一个单独的线程中进行并与主线程分离
    std::thread([&ioc]() { ioc.run(); }).detach();

    // 等待服务器关闭
    server->Wait();
    ioc.stop();
}

int main()
{
    try
    {
        //RunServer();
        std::shared_ptr<StatusServerServiceImpl> service = std::make_shared<StatusServerServiceImpl>();
        auto& cfg = ConfigMgr::GetInstance();
        std::string address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);
        StatusServer server(address, service);
        server.Start();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
