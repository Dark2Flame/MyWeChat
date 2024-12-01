#pragma once
#include "Singleton.h"
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>

class AsioIOServicePool : public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // ʹ�� round-robin �ķ�ʽ����һ��io_service
    boost::asio::io_context& GetIOService();
    void Stop();
private:
    AsioIOServicePool(std::size_t = std::thread::hardware_concurrency());
    std::vector<IOService> _ioServices; // io_service = io_context
    std::vector<WorkPtr> _works; // ������֤_ioServices�еĵĸ���service�ڿ���ʱ���ر�
    // ��һ���汾���ر�����Ϊ����һ��ʼ��ע���˼����¼�����һֱ��������֤������
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;
};


