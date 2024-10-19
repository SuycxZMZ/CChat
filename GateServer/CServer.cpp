#include "CServer.h"
#include "HttpConnection.h"
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
	_acceptor.async_accept(_socket, [self](beast::error_code ec) {
		try {
			if (ec) {
				// һ������ʧ�ܣ�����������������
				self->Start();
				return;
			}

			// ���������ӣ�������һ��HttpConnection������������
			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();

			// ��������
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is:" << exp.what() << std::endl;
		}
	});
}