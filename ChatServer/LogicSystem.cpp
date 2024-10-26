#include "LogicSystem.h"
#include "CSession.h"
#include "const.h"
#include "RedisMgr.h"
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
    
    std::cout << "user login uid is " << root["uid"].asInt()
        << " user token is " << root["token"].asString() << std::endl;
    std::string return_str = root.toStyledString();
    session->Send(return_str, msg_id);
    
    //auto uid = root["uid"].asInt();
    //auto token = root["token"].asString();
    //std::cout << "user login uid is:" << uid << " user token is:"
    //    << token << std::endl;

    //Json::Value rtvalue;
    //Defer defer([this, &rtvalue, session]() {
    //    std::string return_str = rtvalue.toStyledString();
    //    session->Send(return_str, MSG_CHAT_LOGIN_RSP);
    //});

    //// 去redis中拿token
    //std::string uid_str = std::to_string(uid);
    //std::string token_key = USERTOKENPREFIX + uid_str;
    //std::string token_value = "";
    //bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    //if (!success) {
    //    rtvalue["error"] = ErrorCodes::UidInvalid;
    //    return;
    //}
    //if (token_value != token) {
    //    rtvalue["error"] = ErrorCodes::TokenInvalid;
    //    return;
    //}

    //rtvalue["error"] = ErrorCodes::Success;
    //std::string base_key = USER_BASE_INFO + uid_str;
    //auto user_info = std::make_shared<UserInfo>();
    //bool b_base = GetBaseInfo(base_key, uid, user_info);
    //if (!b_base) {
    //    rtvalue["error"] = ErrorCodes::UidInvalid;
    //    return;
    //}

    //rtvalue["uid"] = uid;
    //rtvalue["pwd"] = user_info->pwd;
    //rtvalue["name"] = user_info->name;
    //rtvalue["email"] = user_info->email;
    //rtvalue["nick"] = user_info->nick;
    //rtvalue["desc"] = user_info->desc;
    //rtvalue["sex"] = user_info->sex;
    //rtvalue["icon"] = user_info->icon;

    //std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    //auto b_apply = GetFriendApplyInfo(uid, apply_list);

}

