//
// Created by suycx on 24-10-12.
//

#ifndef CSERVER_H
#define CSERVER_H

#include "CSession.h"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <map>
#include <memory>
#include <string>
class CServer {
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	void ClearSession(std::string&);

private:
	void StartAccept();
	void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
private:
	short _port;
	boost::asio::io_context& _io_context;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};

#endif // CSERVER_H
