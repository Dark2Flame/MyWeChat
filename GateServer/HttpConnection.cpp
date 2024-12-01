#include "HttpConnection.h"
#include "LogicSystem.h"
HttpConnection::HttpConnection(net::io_context& ioc)
	:_socket(ioc)
{

}

void HttpConnection::Start()
{
	auto self = shared_from_this();
	std::cout << "start to read request" << std::endl;
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transfered) {
		try
		{
			if (ec)
			{
				std::cout << "http read error " << ec.what() << std::endl;
				return;
			}

			boost::ignore_unused(bytes_transfered); // ����Ҫʹ�� bytes_transfered ʹ��ignore_unused������ֹ����������
			self->HandleReq();
			self->CheckDeadLine();
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception " << e.what() << std::endl;
		}
		});
}

tcp::socket& HttpConnection::GetSocket()
{
	return _socket;
}

void HttpConnection::CheckDeadLine()
{
	auto self = shared_from_this();
	deadline_.async_wait([self](boost::system::error_code ec) {
		// ��ʱ�����ص�
		self->_socket.close(ec); // �ر�����
		});
}

unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//�ж��Ƿ�������ֺ���ĸ����
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //Ϊ���ַ�
			strTemp += "+";
		else
		{
			//�����ַ���Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16����
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//��ԭ+Ϊ��
		if (str[i] == '+') strTemp += ' ';
		//����%������������ַ���16����תΪchar��ƴ��
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

// ����һ��url: http://localhost:8080/get_test?key1=value1&key2=value2
/*
��������һ��http�����͸�Ŀ�������
GET /get_test?key1=value1&key2=value2 HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0
Accept:
/get_test ���������Դ·�� ����ʹ����һ�ֶ����ֲ�ͬ��������register login��
?key1=value1&key2=value2 �Ǹ��ӵĲ���
*/
void HttpConnection::PreParseGetParam() {
	// ��ȡ URI  
	auto uri = _request.target(); // .target ������һ�Σ�/get_test?key1=value1&key2=value2
	// ���Ҳ�ѯ�ַ����Ŀ�ʼλ�ã��� '?' ��λ�ã�  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // ������ url_decode ����������URL����  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// �������һ�������ԣ����û�� & �ָ�����  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

void HttpConnection::HandleReq()
{
	_response.version(_request.version());
	_response.keep_alive(false);
	if (_request.method() == http::verb::get)
	{
		// ��������
		PreParseGetParam();
		// _request.target()�����ӵ�url
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success)
		{
			// ���http��Ϣͷ
			_response.result(http::status::not_found); // ����HTTP��Ӧ��״̬��Ϊ404 Not Found
			_response.set(http::field::content_type, "text/plain"); //����HTTP��Ӧ��Content-Typeͷ���ֶ� ָ��HTTP��Ӧ�����ݵ�ý������
			// ���http��Ϣ��
			beast::ostream(_response.body()) << "url not found\r\n";
			// ������Ϣ
			WriteResponse();
			return;
		}

		// ���http��Ϣͷ
		_response.result(http::status::ok); // ����HTTP��Ӧ��״̬��Ϊ ok
		_response.set(http::field::server, "GateServer"); // ����HTTP��Ӧ��Serverͷ���ֶ� ��ʶ������Ӧ�ķ�����
		// �ɹ��Ļ� ��Ϣ�岿����LogicSystem���Ѿ������
		// ������Ϣ
		WriteResponse();
	}
	if (_request.method() == http::verb::post)
	{
		// _request.target()�����ӵ�url
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success)
		{
			// ���http��Ϣͷ
			_response.result(http::status::not_found); // ����HTTP��Ӧ��״̬��Ϊ404 Not Found
			_response.set(http::field::content_type, "text/plain"); //����HTTP��Ӧ��Content-Typeͷ���ֶ� ָ��HTTP��Ӧ�����ݵ�ý������
			// ���http��Ϣ��
			beast::ostream(_response.body()) << "url not found\r\n";
			// ������Ϣ
			WriteResponse();
			return;
		}

		// ���http��Ϣͷ
		_response.result(http::status::ok); // ����HTTP��Ӧ��״̬��Ϊ ok
		_response.set(http::field::server, "GateServer"); // ����HTTP��Ӧ��Serverͷ���ֶ� ��ʶ������Ӧ�ķ�����
		// �ɹ��Ļ� ��Ϣ�岿����LogicSystem���Ѿ������
		// ������Ϣ
		WriteResponse();
	}


}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	http::async_write(_socket, _response, [self](beast::error_code ec, ::std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec); // �رշ��Ͷ�
		self->deadline_.cancel(); // ȡ����ʱ��
		});
}
