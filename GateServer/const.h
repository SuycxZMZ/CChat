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
#include <cassert>

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
};
