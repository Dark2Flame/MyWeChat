#include "UserMgr.h"

UserMgr::~UserMgr()
{
    _uid_to_session.clear();
}

std::shared_ptr<Session> UserMgr::getSession(int uid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		return nullptr;
	}

	return iter->second;
}

void UserMgr::setUserSession(int uid, std::shared_ptr<Session> session)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_uid_to_session[uid] = session;
}

void UserMgr::remvUserSession(int uid)
{
	//��Ϊ�ٴε�¼���������������������Ի���ɱ�������ɾ��key������������ע��key�����
	// �п������������¼������ɾ��key����Ҳ���key�����

	//auto uid_str = std::to_string(uid);
	//RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

	{
		std::lock_guard<std::mutex> lock(_mutex);
		_uid_to_session.erase(uid);
	}
}

UserMgr::UserMgr()
{
}
