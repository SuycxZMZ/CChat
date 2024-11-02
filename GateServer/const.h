#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <json/json.h>
#include <string>
#include <cassert>


#define CODEPREFIX "code_"

using tcp = boost::asio::ip::tcp;

// ------------------ url转换代码 ------------------ //
extern unsigned char ToHex(unsigned char x);
extern unsigned char FromHex(unsigned char x);
extern std::string UrlEncode(const std::string& str);
extern std::string UrlDecode(const std::string& str);
// ------------------ url转换代码 ------------------ //


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //Json解析错误
	RPCFailed = 1002,  //RPC请求错误
	VarifyExpired = 1003, //验证码过期
	VarifyCodeErr = 1004, //验证码错误
	UserExist = 1005,       //用户已经存在
	PasswdErr = 1006,    //密码错误
	EmailNotMatch = 1007,  //邮箱不匹配
	PasswdUpFailed = 1008,  //更新密码失败
	PasswdInvalid = 1009,   //密码更新失败
	TokenInvalid = 1010,   //Token失效
	UidInvalid = 1011,  //uid无效
};

class Defer {
public:
	// 接收一个lambda或者函数指针
	Defer(std::function<void()> func);
	~Defer();
private:
	std::function<void()> _func;
};


