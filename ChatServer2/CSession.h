#ifndef CSESSION_H
#define CSESSION_H

#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <queue>
class CServer;
class LogicSystem;

class CSession : public std::enable_shared_from_this<CSession> {
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession();
	std::string& GetUuid();
	boost::asio::ip::tcp::socket& GetSocket();
	void Start();
	void Send(const char* msg, short max_length, short msgid);
	void Send(std::string& msg, short msgid);
	void Close();

	int GetUserId();
	void SetUserId(int uid);

private:
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

private:

	boost::asio::ip::tcp::socket _socket;
	std::string _session_id;
	char _data[MAX_LENGTH];
	CServer* _server;
	bool _b_close;
	std::queue<std::shared_ptr<SendNode>> _send_que;
	std::mutex _send_mutex;
	std::shared_ptr<RecvNode> _recv_msg_node;
	bool _b_head_pase;
	std::shared_ptr<MsgNode> _recv_head_node;
	boost::asio::io_context& _io_context;
	int _user_id;
};

class LogicNode {
	friend class LogicSystem;

public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);

private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recv_node;
};

#endif // CSESSION_H
