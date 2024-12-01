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

			boost::ignore_unused(bytes_transfered); // 不需要使用 bytes_transfered 使用ignore_unused函数防止编译器报错
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
		// 超时触发回调
		self->_socket.close(ec); // 关闭链接
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
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
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
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
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

// 这样一个url: http://localhost:8080/get_test?key1=value1&key2=value2
/*
会变成这样一个http请求发送给目标服务器
GET /get_test?key1=value1&key2=value2 HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0
Accept:
/get_test 是请求的资源路径 我们使用这一字段区分不同的请求如register login等
?key1=value1&key2=value2 是附加的参数
*/
void HttpConnection::PreParseGetParam() {
	// 提取 URI  
	auto uri = _request.target(); // .target 就是这一段：/get_test?key1=value1&key2=value2
	// 查找查询字符串的开始位置（即 '?' 的位置）  
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
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
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
		// 解析参数
		PreParseGetParam();
		// _request.target()是链接的url
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success)
		{
			// 填充http消息头
			_response.result(http::status::not_found); // 设置HTTP响应的状态码为404 Not Found
			_response.set(http::field::content_type, "text/plain"); //设置HTTP响应的Content-Type头部字段 指定HTTP响应中数据的媒体类型
			// 填充http消息体
			beast::ostream(_response.body()) << "url not found\r\n";
			// 发送消息
			WriteResponse();
			return;
		}

		// 填充http消息头
		_response.result(http::status::ok); // 设置HTTP响应的状态码为 ok
		_response.set(http::field::server, "GateServer"); // 设置HTTP响应的Server头部字段 标识生成响应的服务器
		// 成功的话 消息体部分在LogicSystem中已经填充了
		// 发送消息
		WriteResponse();
	}
	if (_request.method() == http::verb::post)
	{
		// _request.target()是链接的url
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success)
		{
			// 填充http消息头
			_response.result(http::status::not_found); // 设置HTTP响应的状态码为404 Not Found
			_response.set(http::field::content_type, "text/plain"); //设置HTTP响应的Content-Type头部字段 指定HTTP响应中数据的媒体类型
			// 填充http消息体
			beast::ostream(_response.body()) << "url not found\r\n";
			// 发送消息
			WriteResponse();
			return;
		}

		// 填充http消息头
		_response.result(http::status::ok); // 设置HTTP响应的状态码为 ok
		_response.set(http::field::server, "GateServer"); // 设置HTTP响应的Server头部字段 标识生成响应的服务器
		// 成功的话 消息体部分在LogicSystem中已经填充了
		// 发送消息
		WriteResponse();
	}


}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	http::async_write(_socket, _response, [self](beast::error_code ec, ::std::size_t bytes_transfered) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec); // 关闭发送端
		self->deadline_.cancel(); // 取消定时器
		});
}
