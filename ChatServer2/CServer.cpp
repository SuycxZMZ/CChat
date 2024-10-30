#include "CServer.h"
#include "CSession.h"
#include "AsioServicePool.h"
#include "UserMgr.h"
#include <functional>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <mutex>

CServer::CServer(boost::asio::io_context& io_context, short port)
    : _io_context(io_context), _port(port),
    _acceptor(io_context, boost::asio::ip::tcp::endpoint(
        boost::asio::ip::address_v4::any(), port)) {
    std::cout << "Server start and listen on port : " << port << std::endl;
    StartAccept();
}
CServer::~CServer() {
    std::cout << "Server dtor on port : " << _port << std::endl;
}
void CServer::ClearSession(std::string& session_id) {
    if (_sessions.find(session_id) != _sessions.end()) {
        //移除用户和session的关联
        UserMgr::GetInstance()->RmvUserSession(_sessions[session_id]->GetUserId());
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.erase(session_id);
    }
}

void CServer::StartAccept() {
    auto& io_context = AsioServicePool::GetInstance().GetIOService();
    std::shared_ptr<CSession> new_session =
        std::make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->GetSocket(),
        std::bind(&CServer::HandleAccept, this, new_session,
            std::placeholders::_1));
}
void CServer::HandleAccept(std::shared_ptr<CSession> new_session,
    const boost::system::error_code& error) {
    if (!error) {
        new_session->Start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.emplace(new_session->GetUuid(), new_session);
    }
    else {
        std::cout << "session accept failed, error is " << error.message() << std::endl;
    }
    // 再次调用接收
    StartAccept();
}