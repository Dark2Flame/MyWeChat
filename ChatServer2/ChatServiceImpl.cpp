#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "MysqlMgr.h"
ChatServiceImpl::ChatServiceImpl()
{
}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request, AddFriendRsp* response)
{
	// A �� B ���ͼӺ�������
	// ���ȣ���request�л�� B �� uid ���� touid
	auto touid = request->touid();
	// ���ڴ漴 UserMgr �в�ѯ ��Ӧuid��session
	auto session = UserMgr::GetInst()->getSession(touid);
	// ���session������ ֱ�ӷ��� ok
	if(session == nullptr)
	{
		return Status::OK;
	}
	// ���session���ڣ����� ID_NOTIFY_ADD_FRIEND_REQ ��Ϣ ͨ��session ���͸� B�Ŀͻ���
	Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["applyuid"] = request->applyuid();
	rtvalue["name"] = request->name();
	rtvalue["desc"] = request->desc();
	rtvalue["icon"] = request->icon();
	rtvalue["sex"] = request->sex();
	rtvalue["nick"] = request->nick();
	std::string ss = rtvalue.toStyledString();
	std::cout << request->applyuid() << " send apply to " << touid << std::endl;
	session->Send(ss, ID_NOTIFY_ADD_FRIEND_REQ);
	
	return Status::OK;
}

Status ChatServiceImpl::RplyAddFriend(ServerContext* context, const RplyFriendReq* request, RplyFriendRsp* response)
{
	return Status::OK;
}

Status ChatServiceImpl::SendChatMsg(ServerContext* context, const SendChatMsgReq* request, SendChatMsgRsp* response)
{
	return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context, const AuthFriendReq* request, AuthFriendRsp* response)
{
	//�����û��Ƿ��ڱ�������
	auto touid = request->touid();
	auto fromuid = request->fromuid();
	auto session = UserMgr::GetInst()->getSession(touid);

	Defer defer([request, response]() {
		response->set_error(ErrorCodes::Success);
		response->set_fromuid(request->fromuid());
		response->set_touid(request->touid());
		});

	//�û������ڴ�����ֱ�ӷ���
	if (session == nullptr) {
		return Status::OK;
	}

	//���ڴ�����ֱ�ӷ���֪ͨ�Է�
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
	auto user_info = std::make_shared<UserInfo>();
	bool b_info = GetBaseInfo(base_key, fromuid, user_info);
	if (b_info) {
		rtvalue["name"] = user_info->name;
		rtvalue["nick"] = user_info->nick;
		rtvalue["icon"] = user_info->icon;
		rtvalue["sex"] = user_info->sex;
	}
	else {
		rtvalue["error"] = ErrorCodes::UidInvalid;
	}

	std::string return_str = rtvalue.toStyledString();

	session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
	return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(ServerContext* context, const TextChatMsgReq* request, TextChatMsgRsp* response)
{
	//�����û��Ƿ��ڱ�������
	auto touid = request->touid();
	auto session = UserMgr::GetInst()->getSession(touid);
	response->set_error(ErrorCodes::Success);

	//�û������ڴ�����ֱ�ӷ���
	if (session == nullptr) {
		return Status::OK;
	}

	//���ڴ�����ֱ�ӷ���֪ͨ�Է�
	Json::Value  rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = request->fromuid();
	rtvalue["touid"] = request->touid();

	//������������֯Ϊ����
	Json::Value text_array;
	for (auto& msg : request->textmsgs()) {
		Json::Value element;
		element["content"] = msg.msgcontent();
		element["msgid"] = msg.msgid();
		text_array.append(element);
	}
	rtvalue["text_array"] = text_array;

	std::string return_str = rtvalue.toStyledString();

	session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
	return Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	//���Ȳ�redis�в�ѯ�û���Ϣ
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
		//redis��û�����ѯmysql
		//��ѯ���ݿ�
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MysqlMgr::GetInst()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;

		//�����ݿ�����д��redis����
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

