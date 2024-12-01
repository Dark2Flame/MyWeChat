//
// Created by 25004 on 2024/7/10.
//

#ifndef ASYNCSERVER_LOGICSYSTEM_H
#define ASYNCSERVER_LOGICSYSTEM_H
#include "Singleton.h"
#include <queue>
#include <thread>
#include "Session.h"
#include <map>
#include <functional>
#include "const.h"
#include <json/value.h>
#include <json/json.h>
#include <json/reader.h>
#include "data.h"
class Session;

class LogicNode;
typedef std::function<void(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data)> FunCallBack;

class LogicSystem:public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>; // 为了使模板类的GetInst生成实例时能调用当前类的构造函数
private:
    LogicSystem();
public:
    ~LogicSystem();
    void RegisterCallBacks();
    void HelloWorldCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);
    void ChatLoginCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);
    void SearchInfoCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);
    void AddFriendCallBack(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);
    void AuthFriendApply(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);
    void DealChatTextMsg(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data);

    void DealMsg();
    void PostLogicNodeToQue(std::shared_ptr<LogicNode> msg);
    bool isPureDigit(const std::string& str); // 判断字符串是否由纯数字组成
    void getUserInfoByUid(std::string uid_str, Json::Value& rtvalue);
    void getUserInfoByName(std::string name, Json::Value& rtvalue);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);
private:
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;

    std::thread _worker; // 工作线程，取待处理回调函数
    bool _b_stop;
    std::map<short, FunCallBack> _fun_callback;
    std::map<int, std::shared_ptr<UserInfo>> _users;
};


#endif //ASYNCSERVER_LOGICSYSTEM_H
