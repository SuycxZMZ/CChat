#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "MsgNode.h"
#include "const.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/write.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>

CSession::CSession(boost::asio::io_context& io_context, CServer* server)
    : _io_context(io_context), _socket(io_context), _server(server),
    _b_close(false), _b_head_pase(false), _user_id(0) 
{
    boost::uuids::uuid uid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(uid);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}
CSession::~CSession() {
    try {
        std::cout << "CSession dtor" << std::endl;
        Close();
    }
    catch (std::exception& exp) {
        std::cout << "exception is : " << exp.what() << std::endl;
    }
}
std::string& CSession::GetUuid() { return _session_id; }
boost::asio::ip::tcp::socket& CSession::GetSocket() { return _socket; }
void CSession::Start() {
    auto shared_this = shared_from_this();
    boost::asio::co_spawn(
        _io_context,
        [shared_this, this]() -> boost::asio::awaitable<void> {
            try {
                for (; !_b_close;) {
                    _recv_head_node->Clear();
                    std::size_t n = co_await boost::asio::async_read(
                        _socket,
                        boost::asio::buffer(_recv_head_node->_data, HEAD_TOTAL_LEN),
                        boost::asio::use_awaitable);
                    if (n == 0) {
                        std::cout << "recv peer closed" << std::endl;
                        this->Close();
                        _server->ClearSession(_session_id);
                        co_return;
                    }

                    // 获取头部数据
                    short msg_id = 0;
                    memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
                    msg_id =
                        boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                    std::cout << "msg_id is:" << msg_id << std::endl;
                    if (msg_id > MAX_LENGTH) {
                        std::cout << "invalid msg_id is " << msg_id << std::endl;
                        _server->ClearSession(_session_id);
                        co_return;
                    }

                    // 接收包长度信息
                    short msg_len = 0;
                    memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN,
                        HEAD_DATA_LEN);
                    msg_len =
                        boost::asio::detail::socket_ops::network_to_host_short(msg_len);
                    std::cout << "msg_len is : " << msg_len << std::endl;
                    if (msg_len > MAX_LENGTH) {
                        std::cout << "invalid msg_len is " << msg_id << std::endl;
                        _server->ClearSession(_session_id);
                        co_return;
                    }

                    // 取出有效载荷，即数据包
                    _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
                    n = co_await boost::asio::async_read(
                        _socket,
                        boost::asio::buffer(_recv_msg_node->_data,
                            _recv_msg_node->_total_len),
                        boost::asio::use_awaitable);
                    if (0 == n) {
                        std::cout << "recv peer closed" << std::endl;
                        this->Close();
                        _server->ClearSession(_session_id);
                        co_return;
                    }
                    _recv_msg_node->_data[n] = '\0';
                    std::cout << "recv msg : " << _recv_msg_node->_data << std::endl;
                    // 投递给逻辑线程
                    LogicSystem::GetInstance().PostMsgToQue(
                        std::make_shared<LogicNode>(shared_this, _recv_msg_node));
                }
            }
            catch (std::exception& e) {
                std::cout << "exception : " << e.what() << std::endl;
                Close();
                _server->ClearSession(_session_id);
            }
        },
        boost::asio::detached);
}
void CSession::Send(const char* msg, short max_length, short msgid) {
    std::unique_lock<std::mutex> lock(_send_mutex);
    std::size_t send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE) {
        std::cout << "session: " << _session_id << " send que fulled, size is "
            << MAX_SENDQUE << std::endl;
        return;
    }
    _send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
    if (send_que_size > 0)
        return;
    auto msg_node = _send_que.front();
    // 还是用异步来发送
    boost::asio::async_write(
        _socket, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1,
            shared_from_this()));
}
void CSession::Send(std::string& msg, short msgid) {
    Send(msg.c_str(), msg.size(), msgid);
}
void CSession::Close() {
    _socket.close();
    _b_close = true;
}

int CSession::GetUserId()
{
    return _user_id;
}

void CSession::SetUserId(int uid)
{
    _user_id = uid;
}

void CSession::HandleWrite(const boost::system::error_code& error,
    std::shared_ptr<CSession> shared_self) 
{
    try {
        if (!error) {
            std::unique_lock<std::mutex> lock(_send_mutex);
            std::cout << "----------- message: " << std::string(_send_que.front()->_data, _send_que.front()->_total_len) << "send finish" << std::endl;
            _send_que.pop();
            if (!_send_que.empty()) {
                auto msg_node = _send_que.front();
                lock.unlock();
                boost::asio::async_write(
                    _socket, boost::asio::buffer(msg_node->_data, msg_node->_total_len),
                    std::bind(&CSession::HandleWrite, this, std::placeholders::_1,
                        shared_self));
            }
        }
        else {
            std::cout << "handle write failed, error is " << error.message()
                << std::endl;
            Close();
            _server->ClearSession(_session_id);
        }
    }
    catch (std::exception& e) {
        std::cout << "CSession::HandleWrite failed : " << e.what() << std::endl;
        Close();
        _server->ClearSession(_session_id);
    }
}

LogicNode::LogicNode(std::shared_ptr<CSession> session,
    std::shared_ptr<RecvNode> recvNode)
    : _session(session), _recv_node(recvNode) {}

