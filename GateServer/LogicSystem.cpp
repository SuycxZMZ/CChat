#include "LogicSystem.h"
#include "HttpConnection.h"
#include "const.h"
#include "VarifyGrpcClient.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"

LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params) {
			++i;
			beast::ostream(connection->_response.body()) << "param" << i << "key is " << elem.first;
			beast::ostream(connection->_response.body()) << "param" << i << "val is " << elem.second << std::endl;
		}
	});

	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		// 将请求体缓冲区转换为字符串
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;

		// 设置响应的内容类型为 JSON
		connection->_response.set(boost::beast::http::field::content_type, "text/json");

		Json::Value root;      // 用于构建响应 JSON
		Json::Value src_root;  // 用于解析请求 JSON

		// 创建 CharReaderBuilder 和 CharReader
		Json::CharReaderBuilder readerBuilder;
		std::string errs;

		// 使用智能指针管理 CharReader 的生命周期
		std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

		// 解析 JSON 字符串
		bool parse_success = reader->parse(body_str.c_str(), body_str.c_str() + body_str.size(), &src_root, &errs);

		if (!parse_success) {
			std::cout << "Failed to parse JSON data! Error: " << errs << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			// 将响应 JSON 转换为字符串
			std::string jsonstr = root.toStyledString();
			// 写入响应体
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		// 从解析后的 JSON 中提取 "email" 字段
		if (!src_root.isMember("email") || !src_root["email"].isString()) {
			std::cout << "JSON does not contain a valid 'email' field!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		std::cout << "email is " << email << std::endl;

		// 要给验证码服务器发送grpc请求
		auto reply = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);

		// 构建成功的响应 JSON
		root["error"] = reply.error();
		root["email"] = email;
		// 将响应 JSON 转换为字符串
		std::string jsonstr = root.toStyledString();
		// 写入响应体
		boost::beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		// 将请求体缓冲区转换为字符串
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;

		// 设置响应的内容类型为 JSON
		connection->_response.set(boost::beast::http::field::content_type, "text/json");

		Json::Value root;      // 用于构建响应 JSON
		Json::Value src_root;  // 用于解析请求 JSON

		// 创建 CharReaderBuilder 和 CharReader
		Json::CharReaderBuilder readerBuilder;
		std::string errs;

		// 使用智能指针管理 CharReader 的生命周期
		std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

		// 解析 JSON 字符串
		bool parse_success = reader->parse(body_str.c_str(), body_str.c_str() + body_str.size(), &src_root, &errs);

		if (!parse_success) {
			std::cout << "Failed to parse JSON data! Error: " << errs << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			// 将响应 JSON 转换为字符串
			std::string jsonstr = root.toStyledString();
			// 写入响应体
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::string varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired " << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 判断用户名在mysql中是否存在
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		int uid = MySqlMgr::GetInstance()->RegUser(name, email, pwd);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		root["error"] = 0;
		root["email"] = src_root["email"];
		root["user"] = src_root["user"].asString();
		root["passwd"] = src_root["passwd"].asString();
		root["confirm"] = src_root["confirm"].asString();
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
}

LogicSystem::~LogicSystem() {
	std::cout << "---------- LogicSystem::~LogicSystem ----------" << std::endl;
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[path](conn);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	_post_handlers[path](con);
	return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.emplace(url, handler);
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.emplace(url, handler);
}