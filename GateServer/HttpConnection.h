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
	void CheckDeadLine(); // ����Ƿ�ʱ
	void PreParseGetParam(); // ���������url
	void HandleReq(); // ��������
	void WriteResponse(); // �ظ�����
	tcp::socket _socket;
	beast::flat_buffer  _buffer{ 8192 }; // ���ڶ�ȡ��д�����ݵĻ�����
	http::request<http::dynamic_body> _request; // ���ڴ洢���յ���HTTP��������ͣ�������
	http::response<http::dynamic_body> _response; // ���ڹ����ͷ���HTTP��Ӧ�����ͣ�������
	boost::asio::steady_timer deadline_{ // ��ʱ�� �����������Ӵ���Ľ�ֹʱ�䣬�������ӹ������
		_socket.get_executor(), std::chrono::seconds(60)
	};
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

