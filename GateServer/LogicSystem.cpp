#include "LogicSystem.h"
#include "HttpConnection.h"
#include "const.h"

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
		// �������建����ת��Ϊ�ַ���
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;

		// ������Ӧ����������Ϊ JSON
		connection->_response.set(boost::beast::http::field::content_type, "text/json");

		Json::Value root;      // ���ڹ�����Ӧ JSON
		Json::Value src_root;  // ���ڽ������� JSON

		// ���� CharReaderBuilder �� CharReader
		Json::CharReaderBuilder readerBuilder;
		std::string errs;

		// ʹ������ָ����� CharReader ����������
		std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

		// ���� JSON �ַ���
		bool parse_success = reader->parse(body_str.c_str(), body_str.c_str() + body_str.size(), &src_root, &errs);

		if (!parse_success) {
			std::cout << "Failed to parse JSON data! Error: " << errs << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			// ����Ӧ JSON ת��Ϊ�ַ���
			std::string jsonstr = root.toStyledString();
			// д����Ӧ��
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		// �ӽ������ JSON ����ȡ "email" �ֶ�
		if (!src_root.isMember("email") || !src_root["email"].isString()) {
			std::cout << "JSON does not contain a valid 'email' field!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			boost::beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		std::cout << "email is " << email << std::endl;
		// �����ɹ�����Ӧ JSON
		root["error"] = 0;
		root["email"] = email;
		// ����Ӧ JSON ת��Ϊ�ַ���
		std::string jsonstr = root.toStyledString();
		// д����Ӧ��
		boost::beast::ostream(connection->_response.body()) << jsonstr;
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