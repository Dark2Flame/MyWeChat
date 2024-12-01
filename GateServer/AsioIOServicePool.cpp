#include "AsioIOServicePool.h"

AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_works(size), _nextIOService(0)
{
    for (size_t i = 0; i < size; ++i)
    {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }

    // Ϊÿһ��IOService����һ��thread
    for (size_t i = 0; i < _ioServices.size(); ++i) {
        // emplace_back ��� thread �Ĺ��캯������һ�� thread ��������ֻ��Ҫ���ݻص����� i
        _threads.emplace_back([this, i] {
            _ioServices[i].run(); // ��� _ioServices ����this�� _ioServices
            });
    }
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService];
    _nextIOService++;
    if (_nextIOService == _ioServices.size())
    {
        _nextIOService = 0; // ʵ��ѭ��ȡ�� i
    }
    return service;
}

// ֹͣ���е�ioService
void AsioIOServicePool::Stop()
{
    for (auto& work : _works)
    {
        work.reset();
    }

    // �ȴ�ÿ���߳̽��� i
    for (auto& t : _threads)
    {
        t.join();
    }
}

AsioIOServicePool::~AsioIOServicePool()
{
    Stop();
    std::cout << "AsioIOServicePool destruct" << std::endl;
}