#include "Server.h"
#include "AsioIOServicePool.h"
#include "UserMgr.h"
using namespace std;

void CServer::start_accept()
{
    auto& io_context = AsioIOServicePool::GetInst()->GetIOService();
    std::shared_ptr<Session> new_session = make_shared<Session>(io_context, this);

    // 异步监听新连接，即不会阻塞当前线程，实际是在ioContext的事件循环中注册一个监听事件
    _acceptor.async_accept(new_session->Socket(), bind(&CServer::handle_accept, this, new_session, placeholders::_1));
}

void CServer::handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error)
{
    if(!error) // 接收连接未发生错误
    {
        // 新会话开始处理消息
        new_session->Start();
        _sessions.insert(make_pair(new_session->GetSessionId(),new_session));
    }
    else // 发生错误
    {
        // 发生错误则删除准备好的新会话
//        delete new_session;
    }

    // 处理完上一个会话 开始接收下一个连接
    start_accept();
}

void CServer::ClearSessions(const std::string& session_id) {
    if (_sessions.find(session_id) != _sessions.end())
    {
        UserMgr::GetInst()->remvUserSession(_sessions[session_id]->GetUserId());
    }
    {
        lock_guard<mutex> lock(_mutex);
        _sessions.erase(session_id);
    }
}
