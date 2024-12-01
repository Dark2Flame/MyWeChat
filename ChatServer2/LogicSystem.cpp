#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "MysqlMgr.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "ChatGrpcClient.h"

LogicSystem::LogicSystem():_b_stop(false)
{
    RegisterCallBacks();
    _worker = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    _consume.notify_one();
    _worker.join();
}

void LogicSystem::RegisterCallBacks()
{
    _fun_callback[MSG_HELLO_WORLD] = std::bind(&LogicSystem::HelloWorldCallBack, this,
                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callback[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::ChatLoginCallBack, this,
                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callback[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfoCallBack, this,
                                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callback[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendCallBack, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callback[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _fun_callback[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    std::cout << "receive msg id is " << root["id"].asInt() << "\t"
              << "receive msg data is " << root["data"].asString() << std::endl;

    // 修改root的data
    root["data"] = "server has receive msg is " + root["data"].asString();
    //std::string return_str = root.toStyledString();
    //session->Send(return_str, root["id"].asInt());
    session->Send(root.toStyledString(), root["id"].asInt());
}

void LogicSystem::DealMsg()
{
    std::cout << "DealMsg run" << std::endl;
    for(;;)
    {
        std::unique_lock<std::mutex> unique_lk(_mutex); // 和条件变量配合使用

        // 队列为空则等待
        while(_msg_que.empty() && !_b_stop)
        {
            _consume.wait(unique_lk); // 将线程挂起，并释放占用的资源，并解锁mutex，等待被唤醒并再次上锁
        }

        // 如果处在关闭状态则清空逻辑队列并及时处理和退出循环
        if(_b_stop)
        {
            // 清空逻辑队列并处理
            while(!_msg_que.empty())
            {
                auto msg_node = _msg_que.front();
                auto call_back_iter =  _fun_callback.find(msg_node->_recvnode->getMsgId());
                if(call_back_iter == _fun_callback.end())
                {
                    _msg_que.pop();
                    continue;
                }
                call_back_iter->second(msg_node->_session, msg_node->_recvnode->getMsgId(), std::string(
                        msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
                _msg_que.pop();
            }

            // 逻辑队列清空完毕，退出处理函数
            break;
        }

        // 没进入停止状态并有数据在队列中则继续处理数据
        auto msg_node = _msg_que.front();
        auto call_back_iter =  _fun_callback.find(msg_node->_recvnode->getMsgId());
        if(call_back_iter == _fun_callback.end())
        {
            _msg_que.pop();
            continue;
        }
        call_back_iter->second(msg_node->_session, msg_node->_recvnode->getMsgId(), std::string(
                msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
        _msg_que.pop();
    }
}

void LogicSystem::PostLogicNodeToQue(std::shared_ptr<LogicNode> msg)
{
     std::unique_lock<std::mutex> unique_lk(_mutex);
     _msg_que.push(msg);

     // 唤醒一个挂起的处理函数
     if(_msg_que.size() == 1)
     {
         _consume.notify_one();
     }

}

void LogicSystem::ChatLoginCallBack(std::shared_ptr<Session> session, const short &msg_id, const std::string &msg_data) {
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto token = root["token"].asString();

    std::cout << "receive uid is " << root["uid"].asInt() << "\t"
              << "receive token is " << root["token"].asString() << std::endl;

    // 链接这台服务器验证Token是否有效
    //auto resp = StatusGrpcClient::GetInst()->Login(uid, token);


    // 构建回复信息
    Json::Value  rtvalue;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
        });

    // 在Redis中查询token是否有效
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInst()->Get(token_key, token_value);
    if (!success) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    if (token_value != token) {
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }

    //rtvalue["error"] = resp.error();
    //// 登录失败 直接返回
    //if (resp.error() != ErrorCodes::Success)
    //{
    //    std::cout << "token is Invalid" << std::endl;
    //    return;
    //}

    rtvalue["error"] = ErrorCodes::Success;
    // 登录验证成功 查询一下uid是否在缓存的用户信息中
    std::cout << "token is valid" << std::endl;
    auto iter = _users.find(uid);
    std::shared_ptr<UserInfo> userInfo = nullptr;
    if (iter == _users.end())
    {
        // 不在缓存中就去数据库中查找
        userInfo = MysqlMgr::GetInst()->GetUser(uid);
        if (userInfo == nullptr)
        {
            // 没有查询到这个uid
            std::cout << "uid " << uid << " not exit" << std::endl;
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }
        _users[uid] = userInfo;
    }
    else
    {
        userInfo = iter->second;
    }

    //从数据库获取申请列表
    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    auto b_apply = GetFriendApplyInfo(uid, apply_list);
    if (b_apply) {
        for (auto& apply : apply_list) {
            Json::Value obj;
            obj["name"] = apply->_name;
            obj["uid"] = apply->_uid;
            obj["icon"] = apply->_icon;
            obj["nick"] = apply->_nick;
            obj["sex"] = apply->_sex;
            obj["desc"] = apply->_desc;
            obj["status"] = apply->_status;
            rtvalue["apply_list"].append(obj);
        }
    }
    //获取好友列表
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = GetFriendList(uid, friend_list);
    for (auto& friend_ele : friend_list) {
        Json::Value obj;
        obj["name"] = friend_ele->name;
        obj["uid"] = friend_ele->uid;
        obj["icon"] = friend_ele->icon;
        obj["nick"] = friend_ele->nick;
        obj["sex"] = friend_ele->sex;
        obj["desc"] = friend_ele->desc;
        obj["back"] = friend_ele->back;
        rtvalue["friend_list"].append(obj);
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = userInfo->name;
    rtvalue["email"] = userInfo->email;
    rtvalue["nick"] = userInfo->nick;
    rtvalue["desc"] = userInfo->desc;
    rtvalue["sex"] = userInfo->sex;
    rtvalue["icon"] = userInfo->icon;

    auto server_name = ConfigMgr::GetInstance()["SelfServer"]["Name"];
    //将登录数量增加
    auto rd_res = RedisMgr::GetInst()->HGet(LOGIN_COUNT, server_name);

    int count = 0;
    if (!rd_res.empty()) {
        count = std::stoi(rd_res);
    }

    count++;
    auto count_str = std::to_string(count);
    RedisMgr::GetInst()->HSet(LOGIN_COUNT, server_name, count_str);
    //session绑定用户uid
    session->SetUserId(uid);
    //为用户设置登录ip server的名字
    std::string  ipkey = USERIPPREFIX + uid_str;
    RedisMgr::GetInst()->Set(ipkey, server_name);
    //uid和session绑定管理,方便以后踢人操作
    UserMgr::GetInst()->setUserSession(uid, session);
}

void LogicSystem::SearchInfoCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    auto uid_str = root["uid"].asString();
    std::cout << "SearchInfo uid is " << uid_str << std::endl;

    Json::Value rtvalue;
    Defer defer([&rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_SEARCH_USER_RSP);
    });

    bool b_digit = isPureDigit(uid_str);
    if (b_digit)
    {
        getUserInfoByUid(uid_str, rtvalue);
    }
    else
    {
        getUserInfoByName(uid_str, rtvalue);
    }

}

void LogicSystem::AddFriendCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
    // 申请人 A 想加 B 为好友
    // A B 连接同一个 ChatServer时
    // A的client -> ID_ADD_FRIEND_REQ -> A的ChatServer -> AddFriendCallBack处理 -> ID_NOTIFY_ADD_FRIEND_REQ -> B的client
    Json::Value root;
    Json::Reader reader;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto applyname = root["applyname"].asString();
    auto touid = root["touid"].asInt();
    auto bakname = root["bakname"].asString();
    std::cout << "Login user uid is " << uid 
                << "apply name is " << applyname 
                << "bakname is " << bakname
                << "touid is " << touid
                << std::endl;

    Json::Value rtvalue;
    Defer defer([&rtvalue, session]() {
        std::string rtstr = rtvalue.toStyledString();
        session->Send(rtstr, ID_ADD_FRIEND_RSP);
    });
    MysqlMgr::GetInst()->AddFriendApply(uid, touid);
    // 查找 touid 对应的客户端在那个chatServer中
    // 去redis中查找，找到touid所属chatServer，再通过ChatGrpcClient找到的对应gRPC Server的 host + port进行 gRPC通信
    std::string toServerName = "";
    bool success = RedisMgr::GetInst()->Get(USERIPPREFIX + root["touid"].asString(), toServerName);
    if (!success)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    auto& cfg = ConfigMgr::GetInstance();
    auto selfServerName = cfg["SelfServer"]["Name"];
    if (toServerName == selfServerName)
    {
        // 在内存中（UserMgr）查找 touid 对应的Session
        auto toSession = UserMgr::GetInst()->getSession(touid);
        if (toSession != nullptr)
        {
            Json::Value notify;
            notify["error"] = ErrorCodes::Success;
            notify["applyuid"] = uid;
            notify["name"] = applyname;
            notify["desc"] = "";
            std::string send_str = notify.toStyledString();
            std::cout << uid << " send apply to " << touid << std::endl;
            // 在内存中找到说明touid在此服务器中，则通过对应的Session发送Tcp请求携带信息给对方
            session->Send(send_str, ID_NOTIFY_ADD_FRIEND_REQ); // 向对方通知
        }
        return;
    }

    std::string base_key = USER_BASE_INFO + std::to_string(uid);
    auto apply_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, uid, apply_info);

    message::AddFriendReq add_req;
    add_req.set_name(applyname);
    add_req.set_applyuid(uid);
    add_req.set_touid(touid);
    add_req.set_desc("");
    if (b_info)
    {
        add_req.set_desc(apply_info->desc);
        add_req.set_icon(apply_info->icon);
        add_req.set_sex(apply_info->sex);
        add_req.set_nick(apply_info->nick);
    }
    auto resp = ChatGrpcClient::GetInst()->NotifyAddFriend(toServerName, add_req);
    
    
}

void LogicSystem::AuthFriendApply(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();
    auto back_name = root["back"].asString();
    std::cout << "from " << uid << " auth friend to " << touid << std::endl;
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    auto user_info = std::make_shared<UserInfo>();
    std::string base_key = USER_BASE_INFO + std::to_string(touid);
    bool b_info = GetBaseInfo(base_key, touid, user_info);
    if (b_info) {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
        rtvalue["uid"] = touid;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_AUTH_FRIEND_RSP);
        });
    //先更新数据库
    MysqlMgr::GetInst()->AuthFriendApply(uid, touid);
    //更新数据库添加好友
    MysqlMgr::GetInst()->AddFriend(uid, touid, back_name);
    //查询redis 查找touid对应的server ip
    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInst()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        return;
    }
    auto& cfg = ConfigMgr::GetInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    //直接通知对方有认证通过消息
    if (to_ip_value == self_name) {
        auto session = UserMgr::GetInst()->getSession(touid);
        if (session) {
            //在内存中则直接发送通知对方
            Json::Value  notify;
            notify["error"] = ErrorCodes::Success;
            notify["fromuid"] = uid;
            notify["touid"] = touid;
            std::string base_key = USER_BASE_INFO + std::to_string(uid);
            auto user_info = std::make_shared<UserInfo>();
            bool b_info = GetBaseInfo(base_key, uid, user_info);
            if (b_info) {
                notify["name"] = user_info->name;
                notify["nick"] = user_info->nick;
                notify["icon"] = user_info->icon;
                notify["sex"] = user_info->sex;
            }
            else {
                notify["error"] = ErrorCodes::UidInvalid;
            }
            std::string return_str = notify.toStyledString();
            session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
        }
        return;
    }
    AuthFriendReq auth_req;
    auth_req.set_fromuid(uid);
    auth_req.set_touid(touid);
    //发送通知
    ChatGrpcClient::GetInst()->NotifyAuthFriend(to_ip_value, auth_req);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid = root["fromuid"].asInt();
    auto touid = root["touid"].asInt();

    const Json::Value  arrays = root["text_array"];

    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["text_array"] = arrays;
    rtvalue["fromuid"] = uid;
    rtvalue["touid"] = touid;

    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
        });


    //查询redis 查找touid对应的server ip
    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInst()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        return;
    }

    auto& cfg = ConfigMgr::GetInstance();
    auto self_name = cfg["SelfServer"]["Name"];
    //直接通知对方有认证通过消息
    if (to_ip_value == self_name) {
        auto session = UserMgr::GetInst()->getSession(touid);
        if (session) {
            //在内存中则直接发送通知对方
            std::string return_str = rtvalue.toStyledString();
            session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        }

        return;
    }


    TextChatMsgReq text_msg_req;
    text_msg_req.set_fromuid(uid);
    text_msg_req.set_touid(touid);
    for (const auto& txt_obj : arrays) {
        auto content = txt_obj["content"].asString();
        auto msgid = txt_obj["msgid"].asString();
        std::cout << "content is " << content << std::endl;
        std::cout << "msgid is " << msgid << std::endl;
        auto* text_msg = text_msg_req.add_textmsgs();
        text_msg->set_msgid(msgid);
        text_msg->set_msgcontent(content);
    }


    //发送通知 todo...
    ChatGrpcClient::GetInst()->NotifyTextChatMsg(to_ip_value, text_msg_req, rtvalue);
}

bool LogicSystem::isPureDigit(const std::string& str)
{
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

void LogicSystem::getUserInfoByUid(std::string uid_str, Json::Value& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = USER_BASE_INFO + uid_str;

    //优先查redis中查询用户信息
    std::string info_str = "";
    bool b_base = RedisMgr::GetInst()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << " icon is " << icon << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }

    auto uid = std::stoi(uid_str);
    //redis中没有则查询mysql
    //查询数据库
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::GetInst()->GetUser(uid);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    //将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    RedisMgr::GetInst()->Set(base_key, redis_root.toStyledString());

    //返回数据
    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}

void LogicSystem::getUserInfoByName(std::string name, Json::Value& rtvalue)
{


    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = NAME_INFO + name;

    //优先查redis中查询用户信息
    std::string info_str = "";
    bool b_base = RedisMgr::GetInst()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        auto uid = root["uid"].asInt();
        auto name = root["name"].asString();
        auto pwd = root["pwd"].asString();
        auto email = root["email"].asString();
        auto nick = root["nick"].asString();
        auto desc = root["desc"].asString();
        auto sex = root["sex"].asInt();
        auto icon = root["icon"].asString();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }

    //redis中没有则查询mysql
    //查询数据库
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::GetInst()->GetUser(name);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    //将数据库内容写入redis缓存
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    RedisMgr::GetInst()->Set(base_key, redis_root.toStyledString());

    //返回数据
    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    //优先查redis中查询用户信息
    std::string info_str = "";
    bool b_base = RedisMgr::GetInst()->Get(base_key, info_str);
    if (b_base) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->pwd = root["pwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
        std::cout << "user login uid is  " << userinfo->uid << " name  is "
            << userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
    }
    else {
        //redis中没有则查询mysql
        //查询数据库
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInst()->GetUser(uid);
        if (user_info == nullptr) {
            return false;
        }

        userinfo = user_info;

        //将数据库内容写入redis缓存
        Json::Value redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = userinfo->pwd;
        redis_root["name"] = userinfo->name;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInst()->Set(base_key, redis_root.toStyledString());
    }

}

bool LogicSystem::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list) {
    //从mysql获取好友申请列表
    return MysqlMgr::GetInst()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
    //从mysql获取好友列表
    return MysqlMgr::GetInst()->GetFriendList(self_id, user_list);
}
