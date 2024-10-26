#include "HttpConnection.h"
#include "const.h"
#include "LogicSystem.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc) :
	_socket(ioc)
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

tcp::socket& HttpConnection::GetSocket() {
	return _socket;
}

void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {
		if (!ec) {
			self->_socket.close(ec);
		}
	});
}

// 简单短链接
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
		PreParseGetParam();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
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

	if (http::verb::post == _request.method()) {
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found \r\n";
			WriteResponse();
			return;
		}

		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

void HttpConnection::PreParseGetParam() {
	// 提取 URI  
	auto uri = _request.target();

	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	// url
	_get_url = uri.substr(0, query_pos);

	// 切出参数
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;

	// 每个 & 切一对 key value
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
