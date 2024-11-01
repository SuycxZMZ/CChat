#include "ChatGrpcClient.h"
#include "Singleton.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"

ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port)
	: _pool_size(poolSize), _host(host), _port(port), _b_stop(false) {
	for (size_t i = 0; i < _pool_size; ++i) {

		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());

		_connections.push(ChatService::NewStub(channel));
	}
}

ChatConPool::~ChatConPool() {
	std::lock_guard<std::mutex> lock(_mutex);
	Close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<ChatService::Stub> ChatConPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this] {
		if (_b_stop) {
			return true;
		}
		return !_connections.empty();
		});
	if (_b_stop) {
		return  nullptr;
	}
	auto context = std::move(_connections.front());
	_connections.pop();
	return context;
}

void ChatConPool::ReturnConnection(std::unique_ptr<ChatService::Stub> context) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}
	_connections.push(std::move(context));
	_cond.notify_one();
}

void ChatConPool::Close() {
	_b_stop = true;
	_cond.notify_all();
}

ChatGrpcClient::ChatGrpcClient()
{
	auto& cfg = ConfigMgr::GetInstance();
	auto server_list = cfg["PeerServer"]["Servers"];

	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["name"].empty()) {
			continue;
		}
		_pools[cfg[word]["name"]] = std::make_unique<ChatConPool>(5, cfg[word]["host"], cfg[word]["port"]);
	}

}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const AddFriendReq& req)
{
	AddFriendRsp rsp;
	Defer defer([&rsp, &req]() {
		rsp.set_error(ErrorCodes::Success);
		rsp.set_applyuid(req.applyuid());
		rsp.set_touid(req.touid());
	});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyAddFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}


bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
	std::string info_str = "";
	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
	if (b_base) {
		Json::Value root;      // 用于构建响应 JSON
		Json::Value src_root;  // 用于解析请求 JSON

		// 创建 CharReaderBuilder 和 CharReader
		Json::CharReaderBuilder readerBuilder;
		std::string errs;

		// 使用智能指针管理 CharReader 的生命周期
		std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

		// 解析 JSON 字符串
		bool parse_success = reader->parse(info_str.c_str(), info_str.c_str() + info_str.size(), &src_root, &errs);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data! Error: " << errs << std::endl;
			return false;
		}

		userinfo->uid = root["uid"].asInt();
		userinfo->name = root["name"].asString();
		userinfo->pwd = root["pwd"].asString();
		userinfo->email = root["email"].asString();
		userinfo->nick = root["nick"].asString();
		userinfo->desc = root["desc"].asString();
		userinfo->sex = root["sex"].asInt();
		userinfo->icon = root["icon"].asString();
		std::cout << "user login uid is  " << userinfo->uid << " name  is "
			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
	}
	else {
		std::shared_ptr<UserInfo> user_info = nullptr;
		user_info = MySqlMgr::GetInstance()->GetUser(uid);
		if (user_info == nullptr) {
			return false;
		}

		userinfo = user_info;
		Json::Value redis_root;

		redis_root["uid"] = uid;
		redis_root["pwd"] = userinfo->pwd;
		redis_root["name"] = userinfo->name;
		redis_root["email"] = userinfo->email;
		redis_root["nick"] = userinfo->nick;
		redis_root["desc"] = userinfo->desc;
		redis_root["sex"] = userinfo->sex;
		redis_root["icon"] = userinfo->icon;
		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
	}

}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req) {
	AuthFriendRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
	});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyAuthFriend(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}
	return rsp;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip,
	const TextChatMsgReq& req, const Json::Value& rtvalue) {

	TextChatMsgRsp rsp;
	rsp.set_error(ErrorCodes::Success);

	Defer defer([&rsp, &req]() {
		rsp.set_fromuid(req.fromuid());
		rsp.set_touid(req.touid());
		for (const auto& text_data : req.textmsgs()) {
			TextChatData* new_msg = rsp.add_textmsgs();
			new_msg->set_msgid(text_data.msgid());
			new_msg->set_msgcontent(text_data.msgcontent());
		}
	});

	auto find_iter = _pools.find(server_ip);
	if (find_iter == _pools.end()) {
		return rsp;
	}

	auto& pool = find_iter->second;
	ClientContext context;
	auto stub = pool->GetConnection();
	Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
	Defer defercon([&stub, this, &pool]() {
		pool->ReturnConnection(std::move(stub));
	});

	if (!status.ok()) {
		rsp.set_error(ErrorCodes::RPCFailed);
		return rsp;
	}

	return rsp;
}