#pragma once

#include "const.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

class LogicSystem;

class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
  friend class LogicSystem;

public:
  HttpConnection(boost::asio::io_context &);
  void Start();
  tcp::socket &GetSocket();

private:
  void CheckDeadline();
  void WriteResponse();
  void HandleRequest();

  void PreParseGetParam();

private:
  tcp::socket _socket;
  boost::beast::flat_buffer _buffer{8192};
  boost::beast::http::request<boost::beast::http::dynamic_body> _request;
  boost::beast::http::response<boost::beast::http::dynamic_body> _response;
  boost::asio::steady_timer _deadline{_socket.get_executor(),
                                      std::chrono::seconds(60)};

  // get参数解析需要的成员变量
  std::string _get_url;
  std::unordered_map<std::string, std::string> _get_params;
};
