#pragma once
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <queue>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <grpcpp/grpcpp.h> 

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::AddFriendReq;
using message::AddFriendRsp;

using message::AuthFriendReq;
using message::AuthFriendRsp;

using message::GetChatServerRsp;
using message::LoginRsp;
using message::LoginReq;
using message::ChatService;

using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;


class ChatConPool {
public:
	ChatConPool(size_t poolSize, std::string host, std::string port);

	~ChatConPool();

	std::unique_ptr<ChatService::Stub> GetConnection();

	void ReturnConnection(std::unique_ptr<ChatService::Stub> context);

	void Close();

private:
	std::atomic<bool> _b_stop;
	size_t _pool_size;
	std::string _host;
	std::string _port;
	std::queue<std::unique_ptr<ChatService::Stub>> _connections;
	std::mutex _mutex;
	std::condition_variable _cond;
};

class ChatGrpcClient :public Singleton<ChatGrpcClient>
{
	friend class Singleton<ChatGrpcClient>;
public:
	~ChatGrpcClient() {
	}
	AddFriendRsp NotifyAddFriend(std::string server_ip, const AddFriendReq& req);
	AuthFriendRsp NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req);
	bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
	TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const TextChatMsgReq& req, const Json::Value& rtvalue);
private:
	ChatGrpcClient();
	std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};



