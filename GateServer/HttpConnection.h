#pragma once

#include "const.h"

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(tcp::socket socket);
	void Start();
	tcp::socket& GetSocket();

private:
	void CheckDeadline();
	void WriteResponse();
	void HandleRequest();

	void PreParseGetParam();

private:
	tcp::socket _socket;
	beast::flat_buffer _buffer{ 8192 };
	http::request<http::dynamic_body> _request;
	http::response<http::dynamic_body> _response;
	net::steady_timer _deadline{
		_socket.get_executor(), std::chrono::seconds(60)
	};

	// get参数解析需要的成员变量
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
};

