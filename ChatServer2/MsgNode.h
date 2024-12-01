//
// Created by 25004 on 2024/7/8.
//

#ifndef ASYNCSERVER_MSGNODE_H
#define ASYNCSERVER_MSGNODE_H
#define HEAD_LENGTH 2
#include <boost/asio.hpp>
#include <iostream>
#include "const.h"
class MsgNode
{
    friend class Session;
public:
    MsgNode(short max_len):_total_len(max_len), _cur_len(0)
    {
        _data = new char[_total_len + 1]();
        _data[_total_len] = '\0';
    }

    ~MsgNode() {
        std::cout << "destruct MsgNode " << std::endl;
        delete[] _data;
    }

    void Clear()
    {
        memset(_data, 0, _total_len);
        _cur_len = 0;
    }

    int _total_len;
    int _cur_len; // 节点中已接收/发送的消息的长度
//    int _max_len;
    char* _data;
};


class RecvNode:public MsgNode
{
public:
    RecvNode(short max_len, short msg_id);

    short getMsgId() const {
        return _msg_id;
    }

private:
    short _msg_id;
};

class SendNode:public MsgNode
{
public:
    SendNode(const char* msg, short max_len, short msg_id);
    short getMsgId() const {
        return _msg_id;
    }
private:
    short _msg_id;
};
#endif //ASYNCSERVER_MSGNODE_H
