#pragma once
#include "Singleton.h"
#include "Session.h"
#include <unordered_map>
#include <memory>
#include <mutex>

class UserMgr :public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<Session> getSession(int uid);
	void setUserSession(int uid, std::shared_ptr<Session> session);
	void remvUserSession(int uid);
private:
	UserMgr();
	std::mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _uid_to_session;
};



