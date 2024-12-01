#include "AsioIOServicePool.h"
AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),
_works(size), _nextIOService(0)
{
    for (size_t i = 0; i < size; ++i)
    {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }

    // 为每一个IOService创建一个thread
    for (size_t i = 0; i < _ioServices.size(); ++i) {
        // emplace_back 会调 thread 的构造函数创建一个 thread 对象，所以只需要传递回调函数 i
        _threads.emplace_back([this, i]{
            _ioServices[i].run(); // 这个 _ioServices 就是this的 _ioServices
        });
    }
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService];
    _nextIOService++;
    if(_nextIOService == _ioServices.size())
    {
        _nextIOService = 0; // 实现循环取用 i
    }
    return service;
}

// 停止所有的ioService
void AsioIOServicePool::Stop()
{
    for(auto& work: _works)
    {
        work.reset();
    }

    // 等待每个线程结束 i
    for(auto& t : _threads)
    {
        t.join();
    }
}

AsioIOServicePool::~AsioIOServicePool()
{
    std::cout << "AsioIOServicePool destruct" << std::endl;
}



