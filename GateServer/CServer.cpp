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
				// 一个连接失败，继续监听其他连接
				self->Start();
				return;
			}

			// 创建新连接，并创建一个HttpConnection类来管理连接
			std::make_shared<HttpConnection>(std::move(self->_socket))->Start();

			// 继续监听
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is:" << exp.what() << std::endl;
		}
	});
}