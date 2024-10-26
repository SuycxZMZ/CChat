#include "LogicSystem.h"
#include "CSession.h"
#include "const.h"
#include "RedisMgr.h"
#include "MySqlMgr.h"
#include "StatusGrpcClient.h"
#include <functional>
#include <iostream>
#include <json/reader.h>
#include <json/value.h>
#include <mutex>
#include <thread>
#include <json/json.h>

LogicSystem::LogicSystem() : _b_stop(false) {
    RegisterCallBacks();
    _work_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
    _b_stop = true;
    _consume.notify_one();
    _work_thread.join();
}
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> lock(_mutex);
    _msg_que.emplace(msg);
    if (_msg_que.size() == 1) {
        lock.unlock();
        _consume.notify_one();
    }
}

LogicSystem& LogicSystem::GetInstance() {
    static LogicSystem instance;
    return instance;
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> lock(_mutex);
        _consume.wait(lock,
            [this]() -> bool { return !_msg_que.empty() || _b_stop; });

        // 如果是关闭状态，则要把所有逻辑执行完再退出
        if (_b_stop) {
            while (!_msg_que.empty()) {
                auto msg_node = _msg_que.front();
                std::cout << "recv_msg id is : " << msg_node->_recv_node->_msg_id
                    << std::endl;
                auto callback_it = _func_callbacks.find(msg_node->_recv_node->_msg_id);
                if (_func_callbacks.end() == callback_it) {
                    _msg_que.pop();
                    continue;
                }
                callback_it->second(msg_node->_session, msg_node->_recv_node->_msg_id,
                    std::string(msg_node->_recv_node->_data,
                        msg_node->_recv_node->_total_len));
                _msg_que.pop();
            }
            break;
        }

        // 正常运行状态
        auto msg_node = _msg_que.front();
        std::cout << "recv_msg id is : " << msg_node->_recv_node->_msg_id
            << std::endl;
        auto callback_it = _func_callbacks.find(msg_node->_recv_node->_msg_id);
        if (_func_callbacks.end() == callback_it) {
            _msg_que.pop();
            continue;
        }
        callback_it->second(msg_node->_session, msg_node->_recv_node->_msg_id,
            std::string(msg_node->_recv_node->_data,
                msg_node->_recv_node->_total_len));
        _msg_que.pop();
    }
}

void LogicSystem::RegisterCallBacks()
{
    _func_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    //_func_callbacks[ID_SEARCH_USER_REQ] = std::bind(&LogicSystem::SearchInfo, this,
    //    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    //_func_callbacks[ID_ADD_FRIEND_REQ] = std::bind(&LogicSystem::AddFriendApply, this,
    //    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    //_func_callbacks[ID_AUTH_FRIEND_REQ] = std::bind(&LogicSystem::AuthFriendApply, this,
    //    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    //_func_callbacks[ID_TEXT_CHAT_MSG_REQ] = std::bind(&LogicSystem::DealChatTextMsg, this,
    //    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    bool parsingSuccessful = reader->parse(msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);

    if (!parsingSuccessful) {
        std::cerr << "JSON 解析失败: " << errs << std::endl;
        return;
    }
    auto uid = root["uid"].asInt();
    std::cout << "user login uid is  " << uid << " user token  is "
        << root["token"].asString() << std::endl;

    //从状态服务器获取token匹配是否准确
    auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
    Json::Value  rtvalue;
    Defer defer([this, &rtvalue, session]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, MSG_CHAT_LOGIN_RSP);
    });

    rtvalue["error"] = rsp.error();
    if (rsp.error() != ErrorCodes::Success) {
        return;
    }

    //内存中查询用户信息
    auto find_iter = _users.find(uid);
    std::shared_ptr<UserInfo> user_info = nullptr;
    if (find_iter == _users.end()) {
        //查询数据库
        user_info = MySqlMgr::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            rtvalue["error"] = ErrorCodes::UidInvalid;
            return;
        }

        _users[uid] = user_info;
    }
    else {
        user_info = find_iter->second;
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = rsp.token();
    rtvalue["name"] = user_info->name;
}

