//
// Created by 25004 on 2024/7/11.
//


#include "Singleton.h"
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>
class AsioIOServicePool: public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // 使用 round-robin 的方式返回一个io_service
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t = std::thread::hardware_concurrency());
    std::vector<IOService> _ioServices; // io_service = io_context
    std::vector<WorkPtr> _works; // 用来保证_ioServices中的的各个service在空闲时不关闭 i
    // 上一个版本不关闭是因为我们一开始就注册了监听事件，会一直监听，保证不空闲 i
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;
};

