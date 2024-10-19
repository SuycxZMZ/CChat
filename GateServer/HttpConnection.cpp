#include "HttpConnection.h"
#include "const.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(tcp::socket socket) :
	_socket(std::move(socket))
{
}

void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try {
			if (ec) {
				std::cout << "http read error and errorno is : " << ec.message() << std::endl;
				return;
			}

			// 处理到的数据
			boost::ignore_unused(bytes_transferred);
			self->HandleRequest();
			self->CheckDeadline();
		} 
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
		}
	});
}

void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {
		if (!ec) {
			self->_socket.close(ec);
		}
	});
}

void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		self->_deadline.cancel();
	});
}

void HttpConnection::HandleRequest() {
	// 设置版本
	_response.version(_request.version());

	// 设置短链接
	_response.keep_alive(false);

	if (http::verb::get == _request.method()) {
		bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		// 正常
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}