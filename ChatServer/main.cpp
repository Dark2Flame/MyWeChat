#include <iostream>
#include <boost/asio.hpp>
#include "Server.h"
#include "AsioIOServicePool.h"
#include <string>
#include "const.h"
#include "ChatServiceImpl.h"
bool isBigEndian()
{
    int val = 1;
    if(*(char*)(&val) == 1)
    {
        return false;
    }

    return true;
}
int main() {
    try
    {
        auto pool = AsioIOServicePool::GetInst();
        auto& gCfgMgr = ConfigMgr::GetInstance();
        auto server_name = gCfgMgr["SelfServer"]["Name"];
        // 启动gRPC服务器
        // 服务器启动时将redis中的登录数设置为0
        RedisMgr::GetInst()->HSet(LOGIN_COUNT, server_name, "0");

        // 定义一个gRPC ChatServer
        std::string server_address(gCfgMgr["SelfServer"]["Host"] + ":" + gCfgMgr["SelfServer"]["RPCPort"]);
        grpc::ServerBuilder bulider;
        // 监听端口和添加服务
        ChatServiceImpl service;
        bulider.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        bulider.RegisterService(&service);
        //构建并启动gRPC服务器
        std::unique_ptr<grpc::Server> server(bulider.BuildAndStart());
        std::cout << "RPC server listening on: " << server_address << std::endl;
        std::thread grpc_thread([&server]() {
            server->Wait();
            });


        boost::asio::io_context ioc;
        boost::asio::signal_set signalSet(ioc, SIGINT, SIGTERM); // 注册一个信号集h
        signalSet.async_wait([&ioc,&pool](auto, auto){ // 异步等待信号集内的函数触发h
            ioc.stop(); // 当触发时会执行 lambda 函数，停止ioc的事件循环h
            pool->Stop();

        });

        CServer s(ioc, std::stoi(gCfgMgr["SelfServer"]["Port"]));
        ioc.run(); // 事件处理循环h
        /*
         * asio会在这个循环中轮询各个socket的事件————通过异步的方式挂载在事件处理循环的事件
         * 比如 accept read write 等并执行响应的处理并触发相应的回调函数
         * l
         */
        RedisMgr::GetInst()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInst()->Close();
        grpc_thread.join();

    }
    catch (std::exception& e){
        std::cerr << "Exception" << e.what() << std::endl;
    }
    return 0;
}
