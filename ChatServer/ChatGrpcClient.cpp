#include "ChatGrpcClient.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include <string>
#include <sstream>

ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port)
	: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
	for (size_t i = 0; i < poolSize_; ++i) {

		std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
			grpc::InsecureChannelCredentials());

		connections_.push(ChatService::NewStub(channel));
	}
}

ChatConPool::~ChatConPool() {
	std::lock_guard<std::mutex> lock(mutex_);
	Close();
	while (!connections_.empty()) {
		connections_.pop();
	}
}

std::unique_ptr<ChatService::Stub> ChatConPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if (b_stop_) {
			return true;
		}
		return !connections_.empty();
		});
	if (b_stop_) {
		return  nullptr;
	}
	auto context = std::move(connections_.front());
	connections_.pop();
	return context;
}

void ChatConPool::ReturnConnection(std::unique_ptr<ChatService::Stub> context)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context));
	cond_.notify_one();
}

void ChatConPool::Close()
{
	b_stop_ = true;
	cond_.notify_all();
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

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const AuthFriendReq& req)
{
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

//bool ChatGrpcClient::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
//{
//	std::string info_str = "";
//	bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
//	if (b_base) {
//		Json::Reader reader;
//		Json::Value root;
//		reader.parse(info_str, root);
//		userinfo->uid = root["uid"].asInt();
//		userinfo->name = root["name"].asString();
//		userinfo->pwd = root["pwd"].asString();
//		userinfo->email = root["email"].asString();
//		userinfo->nick = root["nick"].asString();
//		userinfo->desc = root["desc"].asString();
//		userinfo->sex = root["sex"].asInt();
//		userinfo->icon = root["icon"].asString();
//		std::cout << "user login uid is  " << userinfo->uid << " name  is "
//			<< userinfo->name << " pwd is " << userinfo->pwd << " email is " << userinfo->email << std::endl;
//	}
//	else {
//		std::shared_ptr<UserInfo> user_info = nullptr;
//		user_info = MySqlMgr::GetInstance()->GetUser(uid);
//		if (user_info == nullptr) {
//			return false;
//		}
//
//		userinfo = user_info;
//		Json::Value redis_root;
//		redis_root["uid"] = uid;
//		redis_root["pwd"] = userinfo->pwd;
//		redis_root["name"] = userinfo->name;
//		redis_root["email"] = userinfo->email;
//		redis_root["nick"] = userinfo->nick;
//		redis_root["desc"] = userinfo->desc;
//		redis_root["sex"] = userinfo->sex;
//		redis_root["icon"] = userinfo->icon;
//		RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
//	}
//}
