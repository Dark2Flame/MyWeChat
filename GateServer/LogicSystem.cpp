#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VarifyGrpcClient.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
LogicSystem::LogicSystem()
{
	// ע�����Get����Ļص�����
	// get_test����
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "recive get_test req\n";
		int i = 0;
		for (auto& elem : connection->_get_params)
		{
			i++;
			beast::ostream(connection->_response.body()) << "param " << i << " key is " << elem.first << "\n";
			beast::ostream(connection->_response.body()) << "param " << i << " value is " << elem.second << "\n";
		}
		
		});

	// ��ȡ��֤�������
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is: " << body_str << std::endl;
		// ��������ͻ��˶�ʹ��Json��װ����
		connection->_response.set(http::field::content_type, "text/json"); // ���÷������ݵĸ�ʽΪJson
		Json::Value root; // ���͵�Json����
		Json::Reader reader;
		Json::Value src_root;// ���յ�Json����
		bool parse_success = reader.parse(body_str, src_root); // �����յ����л�Json���ݻ�ԭ�ظ�ʽ��Json
		if (!parse_success)
		{
			// ����ʧ�� �����˳� ���Ƿ��ʹ�����Ϣ���ͻ���
			std::cout << "Failed to parse JSON data! " << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string json_str = root.toStyledString();
			beast::ostream(connection->_response.body()) << json_str;
			return true;
		}

		// �����ɹ�
		if (!src_root.isMember("email"))
		{
			// ����ʧ�� �����˳� ���Ƿ��ʹ�����Ϣ���ͻ���
			std::cout << "Data is no key email" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string json_str = root.toStyledString();
			beast::ostream(connection->_response.body()) << json_str;
			return true;
		}
		auto email = src_root["email"].asString();
		GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email); // ʹ��gRCP�ӿڷ��� VerifyServer ��ȡ��֤��
		std::cout << "Email is: " << email << std::endl;
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string json_str = root.toStyledString();
		beast::ostream(connection->_response.body()) << json_str;
		return true;
		});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//����redis����
		//bool b_usr_exist = RedisMgr::GetInstance()->ExistsKey(src_root["user"].asString());
		//if (b_usr_exist) {
		//	std::cout << " user exist" << std::endl;
		//	root["error"] = ErrorCodes::UserExist;
		//	std::string jsonstr = root.toStyledString();
		//	beast::ostream(connection->_response.body()) << jsonstr;
		//	return true;
		//}

		//�������ݿ��ж��û��Ƿ����
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "handle request /reset_pwd" << std::endl;
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//�������ݿ��ж��û����ͽ�����֤��������Ƿ�ƥ��
		int email_match = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_match) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		int b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << "update password failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		std::cout << "success to update password !!" << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "handle request /user_login" << std::endl;
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}


		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();

		UserInfo userInfo;
		//�������ݿ��жϵ�¼���������������Ƿ�ƥ��
		int pwd_match = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_match) {
			std::cout << " pwd email not match" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto resp = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		if (resp.error()) {
			std::cout << "grpc get chat server failed error is: " << resp.error() << std::endl;
			root["error"] = ErrorCodes::RPCFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "success to load userInfo, user name is: " << userInfo.name << std::endl;
		root["error"] = 0;
		root["user"] = userInfo.name;
		root["uid"] = userInfo.uid;
		root["token"] = resp.token();
		root["host"] = resp.host();
		root["port"] = resp.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
}

LogicSystem::~LogicSystem()
{
}

bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> connection)
{
	if (_get_handlers.find(url) == _get_handlers.end())
	{
		return false;
	}
	_get_handlers[url](connection);
	return true;
}

bool LogicSystem::HandlePost(std::string url, std::shared_ptr<HttpConnection> connection)
{

	if (_post_handlers.find(url) == _post_handlers.end())
	{
		return false;
	}
	_post_handlers[url](connection);
	return true;
}


void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
	_get_handlers.emplace(url, handler);
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
	_post_handlers.emplace(url, handler);
}