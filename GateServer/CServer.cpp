#include "CServer.h"
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	:_ioc(ioc), _acceptor(ioc,tcp::endpoint(tcp::v4(), port)), _socket(ioc)
{
	std::cout << "Server start success on port " << port << std::endl;
}

void CServer::Start()
{
	
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	// ���������ӣ�������HttpConnection������������
	auto new_connection = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_connection->GetSocket(), [self, new_connection](beast::error_code ec) {
		try
		{
			std::cout << "accept a link" << std::endl;
			// ������������ӣ�����������������
			if (ec)
			{
				std::cout << "accept eror" << std::endl;
				self->Start();
				return;
			}

			new_connection->Start();
			self->Start();
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception: " << e.what();
		}
		});
}

