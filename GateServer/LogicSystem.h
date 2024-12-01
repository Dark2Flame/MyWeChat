#pragma once
#include "const.h"
#include <map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
	friend class HttpConnection;
public:
	~LogicSystem();
	bool HandleGet(std::string url, std::shared_ptr<HttpConnection> http_connection);
	bool HandlePost(std::string url, std::shared_ptr<HttpConnection> http_connection);
	void RegGet(std::string url, HttpHandler handler); // ע�ᴦ�����Get����ĺ��� get������Դ
	void RegPost(std::string url, HttpHandler handler); // ע�ᴦ�����Post����ĺ��� post�ύ��Դ

private:
	LogicSystem();
	LogicSystem(const LogicSystem&) = delete;
	LogicSystem& operator=(const LogicSystem&) = delete;

	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;

};

