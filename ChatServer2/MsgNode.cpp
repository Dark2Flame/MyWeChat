//
// Created by 25004 on 2024/7/10.
//

#include "MsgNode.h"

RecvNode::RecvNode(short max_len, short msg_id): MsgNode(max_len),_msg_id(msg_id)
{

}

SendNode::SendNode(const char *msg, short max_len, short msg_id): MsgNode(max_len + HEAD_TOTAL_LENGTH),
_msg_id(msg_id)
{
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &msg_id_host, HEAD_ID_LENGTH);
    short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + HEAD_ID_LENGTH, &max_len_host, HEAD_DATA_LENGTH);
    memcpy(_data + HEAD_TOTAL_LENGTH, msg, max_len);
}



