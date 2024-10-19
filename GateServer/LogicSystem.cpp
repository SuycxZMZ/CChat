#include "LogicSystem.h"
#include "HttpConnection.h"
#include "const.h"

LogicSystem::LogicSystem() {
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
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

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.emplace(url, handler);
}