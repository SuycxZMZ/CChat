#include "LogicSystem.h"
#include "HttpConnection.h"
#include "const.h"
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
