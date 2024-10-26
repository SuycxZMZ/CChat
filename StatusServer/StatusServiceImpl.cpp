#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string generate_unique_string() {
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	std::string unique_string = to_string(uuid);
	return unique_string;
}

// 读取配置文件中的ChatServer信息，初始化ChatServer表
StatusServiceImpl::StatusServiceImpl()
{
	auto& config_mgr = ConfigMgr::GetInstance();
	std::string server_list = config_mgr["ChatServers"]["name"];
	std::vector<std::string> words;
	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& w : words) {
		if (config_mgr[w]["name"].empty()) {
			continue;
		}
		ChatServer server;
		server.port = config_mgr[w]["port"];
		server.host = config_mgr[w]["host"];
		server.name = config_mgr[w]["name"];
		_servers[server.name] = server;
 	}
}

::grpc::Status StatusServiceImpl::GetChatServer(::grpc::ServerContext* context, const message::GetChatServerReq* request, message::GetChatServerRsp* reply)
{
	std::cout << " ---------------- searching a suitable server ----------------" << std::endl;
	const auto& server = GetChatServer();
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	InsertToken(request->uid(), reply->token());
	return ::grpc::Status::OK;
}

::grpc::Status StatusServiceImpl::Login(::grpc::ServerContext* context, const message::LoginReq* request, message::LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return ::grpc::Status::OK;
	}
	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return ::grpc::Status::OK;
	}

	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return ::grpc::Status::OK;
}

void StatusServiceImpl::InsertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}

ChatServer StatusServiceImpl::GetChatServer()
{
	std::lock_guard<std::mutex> lock(_mtx);
	auto min_server = _servers.begin()->second;
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, min_server.name);
	if (count_str.empty()) {
		min_server.con_count = INT_MAX;
	}
	else {
		min_server.con_count = std::stoi(count_str);
	}

	for (auto& server : _servers) {
		if (server.second.name == min_server.name) {
			continue;
		}
		count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}
		if (server.second.con_count < min_server.con_count) {
			min_server = server.second;
		}
	}
	return min_server;
}
