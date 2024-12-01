#pragma once
#include "const.h"
#include <unordered_map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(net::io_context& ioc);
	void Start();
	tcp::socket& GetSocket();
private:
	void CheckDeadLine(); // 检查是否超时
	void PreParseGetParam(); // 解析请求的url
	void HandleReq(); // 处理请求
	void WriteResponse(); // 回复请求
	tcp::socket _socket;
	beast::flat_buffer  _buffer{ 8192 }; // 用于读取和写入数据的缓冲区
	http::request<http::dynamic_body> _request; // 用于存储接收到的HTTP请求的类型（容器）
	http::response<http::dynamic_body> _response; // 用于构建和发送HTTP响应的类型（容器）
	boost::asio::steady_timer deadline_{ // 定时器 用于设置连接处理的截止时间，避免连接挂起过久
		_socket.get_executor(), std::chrono::seconds(60)
	};
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

