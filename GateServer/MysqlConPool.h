#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>


class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}
	unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlConPool {
public:
	MySqlConPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize);

	void checkConnection();

	std::unique_ptr<SqlConnection> getConnection();

	void returnConnection(std::unique_ptr<SqlConnection> con);


	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlConPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolSize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

