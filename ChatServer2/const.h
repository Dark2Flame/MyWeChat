//
// Created by 25004 on 2024/7/9.
//
#pragma once
#include <iostream>
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <json/value.h>
#include <json/json.h>
#include <json/reader.h>
#include <Functional>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "Singleton.h"
#include "ConfigMgr.h"
#include "hiredis.h"
#include "RedisMgr.h"
#include "data.h"
#define CODEPREFIX "code_"

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //Json��������
	RPCFailed = 1002,  //RPC�������
	VarifyExpired = 1003, //��֤�����
	VarifyCodeErr = 1004, //��֤�����
	UserExist = 1005,       //�û��Ѿ�����
	PasswdErr = 1006,    //�������
	EmailNotMatch = 1007,  //���䲻ƥ��
	PasswdUpFailed = 1008,  //��������ʧ��
	PasswdInvalid = 1009,   //�������ʧ��
	TokenInvalid = 1010,   //TokenʧЧ
	UidInvalid = 1011,  //uid��Ч
};

// Defer��
class Defer {
public:
	// ����һ��lambda����ʽ���ߺ���ָ��
	Defer(std::function<void()> func) : func_(func) {}

	// ����������ִ�д���ĺ���
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;
};

#ifndef ASYNCSERVER_CONST_H
#define ASYNCSERVER_CONST_H
#pragma once
#define MAX_LENGTH  1024*2
#define HEAD_TOTAL_LENGTH 4
#define HEAD_ID_LENGTH 2
#define HEAD_DATA_LENGTH 2
#define HEAD_LENGTH 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum {
    MSG_HELLO_WORLD = 1001,
    MSG_CHAT_LOGIN = 1005,
	MSG_CHAT_LOGIN_RSP = 1006, 
	ID_SEARCH_USER_REQ = 1007, 
	ID_SEARCH_USER_RSP = 1008, 
	ID_ADD_FRIEND_REQ = 1009, 
	ID_ADD_FRIEND_RSP = 1010, 
	ID_NOTIFY_ADD_FRIEND_REQ = 1011, 
	ID_AUTH_FRIEND_REQ = 1013,
	ID_AUTH_FRIEND_RSP = 1014,
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015, 
	ID_TEXT_CHAT_MSG_REQ = 1017,
	ID_TEXT_CHAT_MSG_RSP = 1018,
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
	ID_HEART_BEAT = 1020,
};

//struct UserInfo {
//	std::string name;
//	std::string pwd;
//	int uid;
//	std::string email;
//	std::string nick;
//	int sex;
//	std::string desc;
//	std::string icon;
//};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"


#endif //ASYNCSERVER_CONST_H
