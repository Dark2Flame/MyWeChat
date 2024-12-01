#include "MysqlDao.h"

#include "ConfigMgr.h"

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::GetInstance();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MySqlConPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		// ׼�����ô洢����
		std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// �����������
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// ����PreparedStatement��ֱ��֧��ע�����������������Ҫʹ�ûỰ������������������ȡ���������ֵ

		  // ִ�д洢����
		stmt->execute();
		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����
	   // ���磬����洢����������һ���Ự����@result���洢������������������ȡ��
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email)
{
	auto con = pool_->getConnection();
	try
	{
		if (con == nullptr)
		{
			pool_->returnConnection(std::move(con));
			return false;
		}

		/*
		* PreparedStatement ��һ����SQL�����ʹ�õ����ݿ�����ӿڣ�
		* ���������дһ��SQL��䣬���ҿ��Զ��ִ�������䣬ͬʱ���԰�ȫ�ش��ݲ���
		* ��������ѯ��
		*	PreparedStatement ������ʹ��ռλ����ͨ����?���������ѯ�еĲ���λ�á�
		*	��Щ����������ִ�в�ѯ֮ǰ����ȫ�����ã��Ӷ�������SQLע�빥����
		*/

		// ׼����ѯ��� ��ѯ�����û��� name ��Ӧ ����
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));
		// �󶨲��� �û���
		pstmt->setString(1, name);
		// ִ�в�ѯ ��ò�ѯ�����
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// ���������
		while (res->next())
		{
			// res->next()�������Ƕ�ȡ��ѯ�������һ�У���ʼ״̬�µ��û��ƶ�����һ��
			// �������ǵ�ϵͳ�����û������ظ�һ��ֻ�᷵��0�л�1��
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email"))
			{
				// ���������һ�л��Բ��� ����false
				pool_->returnConnection(std::move(con));
				return false;
			}
			// �����˷��� true
			pool_->returnConnection(std::move(con));
			return true;
		} 
	}
	catch (sql::SQLException& e)
	{
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
	auto con = pool_->getConnection();
	try
	{
		if (con == nullptr)
		{
			pool_->returnConnection(std::move(con));
			return false;
		}

		// ׼���������
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));
		// �󶨲���
		pstmt->setString(1, newpwd);
		pstmt->setString(2, name);
		// ִ�и������
		int updateCount = pstmt->executeUpdate();
		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e)
	{
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return false;
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
	auto con = pool_->getConnection();

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con)); // ����Ȩ��ת�ƣ�ʹ��move
		});

	try
	{
		if (con == nullptr)
		{
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email);
		std::unique_ptr<sql::ResultSet> rsp(pstmt->executeQuery());
		std::string origin_pwd = "";
		while (rsp->next())
		{
			origin_pwd = rsp->getString("pwd");
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}
		if (pwd != origin_pwd)
		{
			return false;
		}

		userInfo.email = email;
		userInfo.name = rsp->getString("name");
		userInfo.pwd = origin_pwd;
		userInfo.uid = rsp->getInt("uid");
		return true;
	}
	catch (sql::SQLException& e)
	{
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

