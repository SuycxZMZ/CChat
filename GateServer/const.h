#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <json/json.h>

#include <memory>
#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <queue>
#include <cassert>
#include <condition_variable>
#include <mutex>

#include <hiredis.h>

#define CODEPREFIX "code_"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// ------------------ urlת������ ------------------ //
extern unsigned char ToHex(unsigned char x);
extern unsigned char FromHex(unsigned char x);
extern std::string UrlEncode(const std::string& str);
extern std::string UrlDecode(const std::string& str);
// ------------------ urlת������ ------------------ //


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //Json��������
	RPCFailed = 1002,  //RPC�������
	VarifyExpired = 1003, //��֤�����
	VarifyCodeErr = 1004, //��֤�����
	UserExist = 1005,       //�û��Ѿ�����
	PasswdErr = 1006,    //�������
	EmailNotMatch = 1007,  //���䲻ƥ��
	PasswdUpFailed = 1008,  //��������ʧ��
	PasswdInvalid = 1009,   //�������ʧ��
	TokenInvalid = 1010,   //TokenʧЧ
	UidInvalid = 1011,  //uid��Ч
};

class Defer {
public:
	// ����һ��lambda���ߺ���ָ��
	Defer(std::function<void()> func);
	~Defer();
private:
	std::function<void()> _func;
};
