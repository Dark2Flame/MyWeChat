#include "Server.h"
#include "Session.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
using namespace std;

Session::Session(boost::asio::io_context &ioc, CServer* pServer) 
    :_socket(ioc), _server(pServer),_b_head_parse(false),_timer(ioc)
{
    boost::uuids::uuid  a_session_id = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(a_session_id);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LENGTH);
}

void Session::Start()
{
    // 初始化消息缓冲区
    memset(_data, 0, MAX_LENGTH);
    // 将Server接收到的数据读到_data里
    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                            bind(&Session::handle_read, this, placeholders::_1, placeholders::_2, shared_from_this()));
}

void Session::handle_read(const boost::system::error_code error, size_t bytes_transferred,
                          std::shared_ptr<Session> _self_shared)
{
    // 数据读完后的操作
    if(!error) // 检查读取是否报错
    {
        int copy_len = 0; // 已处理的数据的长度
        short msg_id = 0;
        while (bytes_transferred > 0)
        {
            // 检查头部数据是否被完整放入_rev_head_node
            if(!_b_head_parse)
            {
                // 如果接收到的数据加上_rev_head_node中已经接收到的数据长度小于数据包头部长度
                if(bytes_transferred + _recv_head_node->_cur_len < HEAD_TOTAL_LENGTH)
                {
                    memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                           _data + copy_len, bytes_transferred);
                    _recv_head_node->_cur_len += bytes_transferred;
                    // 继续接收数据

                    std::memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            bind(&Session::handle_read, this,
                                                 placeholders::_1, placeholders::_2,
                                                 _self_shared));
                    return;
                }

                // 如果接收到的数据加上_recv_head_node中已经接收到的数据长度大于等于数据包头部长度
                // 头部未接受长度
                int head_remain = HEAD_TOTAL_LENGTH - _recv_head_node->_cur_len;
                // 将剩余头部放入 _recv_head_node
                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                       _data + copy_len, head_remain);
                // 更新已处理的data长度和剩余未处理长度
                copy_len += head_remain;
                bytes_transferred -= head_remain;

                // 头部中的数据包长度信息
                std::memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LENGTH);
                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                std::cout << "msg_id is " << msg_id << std::endl;

                // msg_id是否非法
                if( msg_id > MAX_LENGTH)
                {
                    std::cout << "invalid mag_id is " << msg_id << std::endl;
                    _server->ClearSessions(_session_id);
                    return;
                }


                // 头部中的数据包长度信息
                short data_len = 0;
                std::memcpy(&data_len, _recv_head_node->_data + HEAD_ID_LENGTH, HEAD_DATA_LENGTH);
                data_len=boost::asio::detail::socket_ops::network_to_host_short(data_len);
                std::cout << "data len is " << data_len << std::endl;

                // 头部长度非法
                if(data_len > MAX_LENGTH)
                {
                    std::cout << "invalid data length is " << data_len << std::endl;
                    _server->ClearSessions(_session_id);
                    return;
                }

                // 初始化接收消息节点
                _recv_msg_node = make_shared<RecvNode>(data_len,msg_id);

                // 如果bytes_transferred < data_len 说明数据未接受全，先保存到接收消息节点中
                if(bytes_transferred < data_len)
                {
                    std::memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                                _data + copy_len, bytes_transferred);
                    _recv_msg_node->_cur_len += bytes_transferred;
                    std::memset(_data, 0 , MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            bind(&Session::handle_read, this,
                                                 placeholders::_1, placeholders::_2,
                                                 _self_shared));
                    // 接收到数据说明头部数据已经处理完成
                    _b_head_parse = true;
                    return;
                }

                // 如果bytes_transferred > data_len 说明数据包粘连
                // 粘连包处理
                // 先将头部指明的长度内的数据放入接收节点
                std::memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
                _recv_msg_node->_cur_len += data_len;
                // 更新已处理处理长度
                copy_len += data_len;
                // 更新接收数据的剩余长度
                bytes_transferred -= data_len;
                // 将接收到的数据反序列化并输出
                _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';

                if (msg_id == ID_HEART_BEAT) {
                    this->stayAlive();
                }
                else {
                    // 将接收到的其中一个完整包发送
                    LogicSystem::GetInst()->PostLogicNodeToQue(make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
                }

                //Send(_recv_msg_node->_data, _recv_msg_node->_total_len, msg_id);
                // 处理粘连的下一个包的数据
                _b_head_parse = false;
                _recv_head_node->Clear();
                if(bytes_transferred <= 0) // 如果没有剩余数据
                {
                    // 继续挂起一个监听去读取下一个传来的包
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                            bind(&Session::handle_read, this,
                                                 placeholders::_1, placeholders::_2,
                                                 _self_shared));
                    // 结束回调函数
                    return;
                }
                // 如果还有剩余数据，说明粘包了，继续循环处理下一个包的数据
                continue;
            }

            // 如果头部数据已经进入了 _recv_head_node
            // 计算上次未处理完的数据的长度
            int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
            // 如果本次接收到的数据长度不足
            if(bytes_transferred < remain_msg)
            {
                // 暂存到接收节点中
                std::memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len,
                            bytes_transferred);
                _recv_msg_node->_cur_len += bytes_transferred;
                // 继续挂起一个监听去读取下一个传来的包
                memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        bind(&Session::handle_read, this,
                                             placeholders::_1, placeholders::_2,
                                             _self_shared));
                return;
            }
            // 如果本次接收到的数据长度过长
            // 将剩余长度内的数据保存到接收消息节点

            std::memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len,
                        remain_msg);
            _recv_msg_node->_cur_len += remain_msg;
            bytes_transferred -= remain_msg;
            copy_len += remain_msg;
            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';

            if (msg_id == ID_HEART_BEAT) {
                this->stayAlive();
            }
            else {
                // 将接收到的其中一个完整包发送
                LogicSystem::GetInst()->PostLogicNodeToQue(make_shared<LogicNode>(shared_from_this(), _recv_msg_node));
            }
            // 继续处理粘连的下一个包的数据
            _b_head_parse = false;
            _recv_head_node->Clear();
            // 如果没有粘连下一个包
            if(bytes_transferred <= 0)
            {
                // 继续挂起一个监听去读取下一个传来的包
                memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        bind(&Session::handle_read, this,
                                             placeholders::_1, placeholders::_2,
                                             _self_shared));
                return;
            }
            continue;
        }
    }
    else
    {
        // 出错时报错并结束会话
        cout << "read error" << endl;
        // delete this;
        _server->ClearSessions(_session_id);
    }
}


void Session::handle_write(const boost::system::error_code error, std::shared_ptr<Session> _self_shared)
{
    // 回复消息后的操作
    if(!error)
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        _send_que.pop(); // 调用这个回调说明上一条消息已经发送完，pop出队列的首元素

        if(!_send_que.empty())
        {
            auto &msgnode = _send_que.front();
            boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                                     std::bind(&Session::handle_write, this, std::placeholders::_1,
                                               _self_shared));
        }
    }
    else
    {
        // 发生错误报错并结束会话
        cout << "write error" << endl;
        _server->ClearSessions(_session_id);
    }
}

void Session::stayAlive()
{
    
    cout << "Receive heartbeat" << endl;
    _timer.expires_after(10s);
    auto self(shared_from_this());
    _timer.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            cout << "Long time no heartbeat close the session" << endl;
            boost::system::error_code ec;
            _socket.shutdown(tcp::socket::shutdown_both, ec); // 关闭读写操作
            _socket.close(ec); // 关闭套接字
            //_server->ClearSessions(_session_id);
        }
        });
}
const string &Session::GetSessionId() const {
    return _session_id;
}


void Session::SetUserId(int uid)
{
    // userId是数据库中的用户对应的唯一id 0 1 2 3 ...
    _user_uid = uid;
}

int Session::GetUserId()
{
    return _user_uid;
}


void Session::Send(char *msg, int max_length, short msg_id)
{
    bool pending = false; // pending 为 false 则表示 上一次发送的消息已经发送完，消息队列中无数据
    std::lock_guard<std::mutex> lock(_send_lock); // 上锁

    // 控制消息队列的大小
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE) {
        cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    if(_send_que.size() > 0) // 检查队列中是否有消息
    {
        pending = true; // 有则 pending 置为 true
    }

    _send_que.push(make_shared<SendNode>(msg, max_length, msg_id)); // 将消息插入消息队列

    if(pending)
    {
        return; // 如果有消息正在发送则直接退出
    }

    auto& msgNode = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgNode->_data, msgNode->_total_len),
                       std::bind(&Session::handle_write, this, std::placeholders::_1,
                                 shared_from_this()));
}
void Session::Send(std::string msg, short msgid) {
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE) {
        std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }
    _send_que.push(make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgNode = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgNode->_data, msgNode->_total_len),
                             std::bind(&Session::handle_write, this, std::placeholders::_1,
                                       shared_from_this()));
}
LogicNode::LogicNode(std::shared_ptr<Session> session, std::shared_ptr<RecvNode> recvnode) :_session(session),
_recvnode(recvnode)
{}