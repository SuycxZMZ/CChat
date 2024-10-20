#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
#include <iostream>
#include <exception>

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) :
	_acceptor(ioc, tcp::endpoint(tcp::v4(), port)),
	_socket(ioc),
	_ioc(ioc)
{

}

void CServer::Start() {
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	auto new_connection = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_connection->GetSocket(), [self, new_connection](beast::error_code ec) {
		try {
			if (ec) {
				// һ������ʧ�ܣ�����������������
				self->Start();
				return;
			}

			// ���ӽ���֮���Ҫ��ʼ������
			new_connection->Start();

			// ��������
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is:" << exp.what() << std::endl;
			self->Start();
		}
	});
}