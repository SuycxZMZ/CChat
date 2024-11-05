#include "LogicSystem.h"
#include "CSession.h"
#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
#include "MySqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "const.h"
#include <functional>
#include <iostream>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <mutex>
#include <thread>

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

LogicSystem &LogicSystem::GetInstance() {
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

void LogicSystem::RegisterCallBacks() {
  _func_callbacks[MSG_CHAT_LOGIN] =
      std::bind(&LogicSystem::LoginHandler, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);

  _func_callbacks[ID_SEARCH_USER_REQ] =
      std::bind(&LogicSystem::SearchInfo, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);

  _func_callbacks[ID_ADD_FRIEND_REQ] =
      std::bind(&LogicSystem::AddFriendApply, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);

  _func_callbacks[ID_AUTH_FRIEND_REQ] =
      std::bind(&LogicSystem::AuthFriendApply, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);

  _func_callbacks[ID_TEXT_CHAT_MSG_REQ] =
      std::bind(&LogicSystem::DealChatTextMsg, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session,
                               const short &msg_id,
                               const std::string &msg_data) {
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;

  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  bool parsingSuccessful = reader->parse(
      msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);

  if (!parsingSuccessful) {
    std::cerr << "JSON 解析失败: " << errs << std::endl;
    return;
  }

  auto uid = root["uid"].asInt();
  auto token = root["token"].asString();
  std::cout << "user login uid is  " << uid << " user token  is " << token
            << std::endl;

  Json::Value rtvalue;
  Defer defer([this, &rtvalue, session]() {
    std::string return_str = rtvalue.toStyledString();
    std::cout << "Login response : " << return_str << std::endl;
    session->Send(return_str, MSG_CHAT_LOGIN_RSP);
  });

  std::string uid_str = std::to_string(uid);
  std::string token_key = USERTOKENPREFIX + uid_str;
  std::string token_value = "";
  bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
  if (!success) {
    rtvalue["error"] = ErrorCodes::UidInvalid;
    return;
  }

  if (token_value != token) {
    rtvalue["error"] = ErrorCodes::TokenInvalid;
    return;
  }

  rtvalue["error"] = ErrorCodes::Success;

  std::string base_key = USER_BASE_INFO + uid_str;
  auto user_info = std::make_shared<UserInfo>();
  bool b_base = GetBaseInfo(base_key, uid, user_info);
  if (!b_base) {
    rtvalue["error"] = ErrorCodes::UidInvalid;
    return;
  }
  rtvalue["uid"] = uid;
  rtvalue["pwd"] = user_info->pwd;
  rtvalue["name"] = user_info->name;
  rtvalue["email"] = user_info->email;
  rtvalue["nick"] = user_info->nick;
  rtvalue["desc"] = user_info->desc;
  rtvalue["sex"] = user_info->sex;
  rtvalue["icon"] = user_info->icon;

  std::vector<std::shared_ptr<ApplyInfo>> apply_list;
  auto b_apply = GetFriendApplyInfo(uid, apply_list);
  if (b_apply) {
    for (auto &apply : apply_list) {
      Json::Value obj;
      obj["name"] = apply->_name;
      obj["uid"] = apply->_uid;
      obj["icon"] = apply->_icon;
      obj["nick"] = apply->_nick;
      obj["sex"] = apply->_sex;
      obj["desc"] = apply->_desc;
      obj["status"] = apply->_status;
      rtvalue["apply_list"].append(obj);
    }
  }

  // 获取好友列表
  std::vector<std::shared_ptr<UserInfo>> friend_list;
  bool b_friend_list = GetFriendList(uid, friend_list);
  for (auto &friend_ele : friend_list) {
    Json::Value obj;
    obj["name"] = friend_ele->name;
    obj["uid"] = friend_ele->uid;
    obj["icon"] = friend_ele->icon;
    obj["nick"] = friend_ele->nick;
    obj["sex"] = friend_ele->sex;
    obj["desc"] = friend_ele->desc;
    obj["back"] = friend_ele->back;
    rtvalue["friend_list"].append(obj);
  }

  auto server_name = ConfigMgr::GetInstance()["SelfServer"]["name"];
  auto rd_res = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
  int count = 0;
  if (!rd_res.empty()) {
    count = std::stoi(rd_res);
  }

  count++;
  auto count_str = std::to_string(count);
  RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);
  session->SetUserId(uid);
  std::string ipkey = USERIPPREFIX + uid_str;
  RedisMgr::GetInstance()->Set(ipkey, server_name);
  UserMgr::GetInstance()->SetUserSession(uid, session);

  return;
}

void LogicSystem::SearchInfo(std::shared_ptr<CSession> session,
                             const short &msg_id, const std::string &msg_data) {
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;

  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  bool parsingSuccessful = reader->parse(
      msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);

  if (!parsingSuccessful) {
    std::cerr << "JSON 解析失败: " << errs << std::endl;
    return;
  }

  auto uid_str = root["uid"].asString();
  std::cout << "user SearchInfo uid is  " << uid_str << std::endl;

  Json::Value rtvalue;

  Defer deder([this, &rtvalue, session]() {
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_SEARCH_USER_RSP);
  });

  bool b_digit = isPureDigit(uid_str);
  if (b_digit) {
    GetUserByUid(uid_str, rtvalue);
  } else {
    GetUserByName(uid_str, rtvalue);
  }
}

void LogicSystem::AddFriendApply(std::shared_ptr<CSession> session,
                                 const short &msg_id,
                                 const std::string &msg_data) {
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;

  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  bool parsingSuccessful = reader->parse(
      msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);

  if (!parsingSuccessful) {
    std::cerr << "JSON 解析失败: " << errs << std::endl;
    return;
  }

  auto uid = root["uid"].asInt();
  auto applyname = root["applyname"].asString();
  auto bakname = root["bakname"].asString();
  auto touid = root["touid"].asInt();
  std::cout << "Add friend apply is  " << uid << " applyname  is " << applyname
            << " bakname is " << bakname << " touid is " << touid << std::endl;

  Json::Value rtvalue;
  rtvalue["error"] = ErrorCodes::Success;
  Defer defer([this, &rtvalue, session]() {
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_ADD_FRIEND_RSP);
  });

  // 这里直接添加到mysql就行，因为添加好友之前肯定已经正确搜索到了
  MySqlMgr::GetInstance()->AddFriendApply(uid, touid);

  auto to_str = std::to_string(touid);
  auto to_ip_key = USERIPPREFIX + to_str;
  std::string to_ip_value = "";

  // 获取目标用户所在的服务器，放入 to_ip_value
  bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
  if (!b_ip) {
    return;
  }

  auto &cfg = ConfigMgr::GetInstance();
  auto self_name = cfg["SelfServer"]["name"];

  std::string base_key = USER_BASE_INFO + std::to_string(uid);
  auto apply_info = std::make_shared<UserInfo>();
  bool b_info = GetBaseInfo(base_key, uid, apply_info);

  // 两个用户连接的是一个服务器
  if (to_ip_value == self_name) {
    auto session = UserMgr::GetInstance()->GetSession(touid);
    if (session) {
      Json::Value notify;
      notify["error"] = ErrorCodes::Success;
      notify["applyuid"] = uid;
      notify["name"] = applyname;
      notify["desc"] = "";
      if (b_info) {
        notify["icon"] = apply_info->icon;
        notify["sex"] = apply_info->sex;
        notify["nick"] = apply_info->nick;
      }
      std::string return_str = notify.toStyledString();
      // std::cout << "----------- ID_NOTIFY_ADD_FRIEND_REQ:" << return_str
      //           << " -----------" << std::endl;
      session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
    }

    return;
  }

  // 不在同一个服务器，调用rpc远程方法
  message::AddFriendReq add_req;
  add_req.set_applyuid(uid);
  add_req.set_touid(touid);
  add_req.set_name(applyname);
  add_req.set_desc("");
  if (b_info) {
    add_req.set_icon(apply_info->icon);
    add_req.set_sex(apply_info->sex);
    add_req.set_nick(apply_info->nick);
  }

  ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
}

void LogicSystem::AuthFriendApply(std::shared_ptr<CSession> session,
                                  const short &msg_id,
                                  const std::string &msg_data) {
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;
  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  bool parsingSuccessful = reader->parse(
      msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);
  if (!parsingSuccessful) {
    std::cerr << "JSON 解析失败: " << errs << std::endl;
    return;
  }

  auto uid = root["fromuid"].asInt();
  auto touid = root["touid"].asInt();
  auto back_name = root["back"].asString();
  std::cout << "from " << uid << " auth friend to " << touid << std::endl;

  Json::Value rtvalue;
  rtvalue["error"] = ErrorCodes::Success;
  auto user_info = std::make_shared<UserInfo>();

  std::string base_key = USER_BASE_INFO + std::to_string(touid);
  bool b_info = GetBaseInfo(base_key, touid, user_info);
  if (b_info) {
    rtvalue["name"] = user_info->name;
    rtvalue["nick"] = user_info->nick;
    rtvalue["icon"] = user_info->icon;
    rtvalue["sex"] = user_info->sex;
    rtvalue["uid"] = touid;
  } else {
    rtvalue["error"] = ErrorCodes::UidInvalid;
  }

  Defer defer([this, &rtvalue, session]() {
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_AUTH_FRIEND_RSP);
  });

  // //先更新数据库
  MySqlMgr::GetInstance()->AuthFriendApply(uid, touid);
  // 更新数据库添加好友
  MySqlMgr::GetInstance()->AddFriend(uid, touid, back_name);

  // 查询redis 查找touid对应的server ip
  auto to_str = std::to_string(touid);
  auto to_ip_key = USERIPPREFIX + to_str;
  std::string to_ip_value = "";
  bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
  if (!b_ip) {
    return;
  }

  auto &cfg = ConfigMgr::GetInstance();
  auto self_name = cfg["SelfServer"]["name"];
  if (to_ip_value == self_name) {
    auto session = UserMgr::GetInstance()->GetSession(touid);
    if (session) {
      Json::Value notify;
      notify["error"] = ErrorCodes::Success;
      notify["fromuid"] = uid;
      notify["touid"] = touid;
      std::string base_key = USER_BASE_INFO + std::to_string(uid);
      auto user_info = std::make_shared<UserInfo>();
      bool b_info = GetBaseInfo(base_key, uid, user_info);
      if (b_info) {
        notify["name"] = user_info->name;
        notify["nick"] = user_info->nick;
        notify["icon"] = user_info->icon;
        notify["sex"] = user_info->sex;
      } else {
        notify["error"] = ErrorCodes::UidInvalid;
      }

      std::string return_str = notify.toStyledString();
      session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
    }
    return;
  }

  AuthFriendReq auth_req;
  auth_req.set_fromuid(uid);
  auth_req.set_touid(touid);
  ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
}

bool LogicSystem::isPureDigit(const std::string &str) {
  for (char c : str) {
    if (!std::isdigit(c)) {
      return false;
    }
  }
  return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, Json::Value &rtvalue) {
  rtvalue["error"] = ErrorCodes::Success;

  std::string base_key = USER_BASE_INFO + uid_str;
  std::string info_str = "";
  bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
  if (b_base) {
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    bool parsingSuccessful = reader->parse(
        info_str.c_str(), info_str.c_str() + info_str.size(), &root, &errs);
    if (!parsingSuccessful) {
      std::cerr << "JSON 解析失败: " << errs << std::endl;
      return;
    }
    // Json::Reader reader;
    // Json::Value root;
    // reader.parse(info_str, root);
    auto uid = root["uid"].asInt();
    auto name = root["name"].asString();
    auto pwd = root["pwd"].asString();
    auto email = root["email"].asString();
    auto nick = root["nick"].asString();
    auto desc = root["desc"].asString();
    auto sex = root["sex"].asInt();
    auto icon = root["icon"].asString();
    std::cout << "user  uid is  " << uid << " name  is " << name << " pwd is "
              << pwd << " email is " << email << " icon is " << icon
              << std::endl;

    rtvalue["uid"] = uid;
    rtvalue["pwd"] = pwd;
    rtvalue["name"] = name;
    rtvalue["email"] = email;
    rtvalue["nick"] = nick;
    rtvalue["desc"] = desc;
    rtvalue["sex"] = sex;
    rtvalue["icon"] = icon;
    return;
  }

  auto uid = std::stoi(uid_str);
  std::shared_ptr<UserInfo> user_info = nullptr;
  user_info = MySqlMgr::GetInstance()->GetUser(uid);
  if (user_info == nullptr) {
    rtvalue["error"] = ErrorCodes::UidInvalid;
    return;
  }

  Json::Value redis_root;
  redis_root["uid"] = user_info->uid;
  redis_root["pwd"] = user_info->pwd;
  redis_root["name"] = user_info->name;
  redis_root["email"] = user_info->email;
  redis_root["nick"] = user_info->nick;
  redis_root["desc"] = user_info->desc;
  redis_root["sex"] = user_info->sex;
  redis_root["icon"] = user_info->icon;

  RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

  rtvalue["uid"] = user_info->uid;
  rtvalue["pwd"] = user_info->pwd;
  rtvalue["name"] = user_info->name;
  rtvalue["email"] = user_info->email;
  rtvalue["nick"] = user_info->nick;
  rtvalue["desc"] = user_info->desc;
  rtvalue["sex"] = user_info->sex;
  rtvalue["icon"] = user_info->icon;
}

void LogicSystem::GetUserByName(std::string name, Json::Value &rtvalue) {
  rtvalue["error"] = ErrorCodes::Success;
  std::string base_key = NAME_INFO + name;
  std::string info_str = "";
  bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
  if (b_base) {
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    bool parsingSuccessful = reader->parse(
        info_str.c_str(), info_str.c_str() + info_str.size(), &root, &errs);
    if (!parsingSuccessful) {
      std::cerr << "JSON 解析失败: " << errs << std::endl;
      return;
    }
    auto uid = root["uid"].asInt();
    auto name = root["name"].asString();
    auto pwd = root["pwd"].asString();
    auto email = root["email"].asString();
    auto nick = root["nick"].asString();
    auto desc = root["desc"].asString();
    auto sex = root["sex"].asInt();
    std::cout << "user  uid is  " << uid << " name  is " << name << " pwd is "
              << pwd << " email is " << email << std::endl;

    rtvalue["uid"] = uid;
    rtvalue["pwd"] = pwd;
    rtvalue["name"] = name;
    rtvalue["email"] = email;
    rtvalue["nick"] = nick;
    rtvalue["desc"] = desc;
    rtvalue["sex"] = sex;
    return;
  }

  std::shared_ptr<UserInfo> user_info = nullptr;
  user_info = MySqlMgr::GetInstance()->GetUser(name);
  if (user_info == nullptr) {
    rtvalue["error"] = ErrorCodes::UidInvalid;
    return;
  }

  Json::Value redis_root;
  redis_root["uid"] = user_info->uid;
  redis_root["pwd"] = user_info->pwd;
  redis_root["name"] = user_info->name;
  redis_root["email"] = user_info->email;
  redis_root["nick"] = user_info->nick;
  redis_root["desc"] = user_info->desc;
  redis_root["sex"] = user_info->sex;

  RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
  rtvalue["uid"] = user_info->uid;
  rtvalue["pwd"] = user_info->pwd;
  rtvalue["name"] = user_info->name;
  rtvalue["email"] = user_info->email;
  rtvalue["nick"] = user_info->nick;
  rtvalue["desc"] = user_info->desc;
  rtvalue["sex"] = user_info->sex;
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid,
                              std::shared_ptr<UserInfo> &userinfo) {
  std::string info_str = "";
  bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
  if (b_base) {
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    bool parsingSuccessful = reader->parse(
        info_str.c_str(), info_str.c_str() + info_str.size(), &root, &errs);
    if (!parsingSuccessful) {
      std::cerr << "JSON 解析失败: " << errs << std::endl;
      return false;
    }

    userinfo->uid = root["uid"].asInt();
    userinfo->name = root["name"].asString();
    userinfo->pwd = root["pwd"].asString();
    userinfo->email = root["email"].asString();
    userinfo->nick = root["nick"].asString();
    userinfo->desc = root["desc"].asString();
    userinfo->sex = root["sex"].asInt();
    userinfo->icon = root["icon"].asString();
    std::cout << "user login uid is  " << userinfo->uid << " name  is "
              << userinfo->name << " pwd is " << userinfo->pwd << " email is "
              << userinfo->email << std::endl;
  } else {
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MySqlMgr::GetInstance()->GetUser(uid);
    if (user_info == nullptr) {
      return false;
    }

    userinfo = user_info;
    Json::Value redis_root;
    redis_root["uid"] = uid;
    redis_root["pwd"] = userinfo->pwd;
    redis_root["name"] = userinfo->name;
    redis_root["email"] = userinfo->email;
    redis_root["nick"] = userinfo->nick;
    redis_root["desc"] = userinfo->desc;
    redis_root["sex"] = userinfo->sex;
    redis_root["icon"] = userinfo->icon;
    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
  }

  return true;
}

bool LogicSystem::GetFriendApplyInfo(
    int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &list) {
  return MySqlMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool LogicSystem::GetFriendList(
    int self_id, std::vector<std::shared_ptr<UserInfo>> &user_list) {
  return MySqlMgr::GetInstance()->GetFriendList(self_id, user_list);
}

void LogicSystem::DealChatTextMsg(std::shared_ptr<CSession> session,
                                  const short &msg_id,
                                  const std::string &msg_data) {
  Json::Value root;
  Json::CharReaderBuilder readerBuilder;
  std::string errs;
  std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
  bool parsingSuccessful = reader->parse(
      msg_data.c_str(), msg_data.c_str() + msg_data.size(), &root, &errs);
  if (!parsingSuccessful) {
    std::cerr << "JSON 解析失败: " << errs << std::endl;
    return;
  }

  auto uid = root["fromuid"].asInt();
  auto touid = root["touid"].asInt();

  const Json::Value arrays = root["text_array"];

  Json::Value rtvalue;
  rtvalue["error"] = ErrorCodes::Success;
  rtvalue["text_array"] = arrays;
  rtvalue["fromuid"] = uid;
  rtvalue["touid"] = touid;

  Defer defer([this, &rtvalue, session]() {
    std::string return_str = rtvalue.toStyledString();
    session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
  });

  // 查询redis 查找touid对应的server ip
  auto to_str = std::to_string(touid);
  auto to_ip_key = USERIPPREFIX + to_str;
  std::string to_ip_value = "";
  bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
  if (!b_ip) {
    return;
  }

  auto &cfg = ConfigMgr::GetInstance();
  auto self_name = cfg["SelfServer"]["name"];

  // 二者在同一个服务器上，直接通知对方有消息
  if (to_ip_value == self_name) {
    auto session = UserMgr::GetInstance()->GetSession(touid);
    if (session) {
      // 则直接发送通知对方
      std::string return_str = rtvalue.toStyledString();
      session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    }
    return;
  }

  TextChatMsgReq text_msg_req;
  text_msg_req.set_fromuid(uid);
  text_msg_req.set_touid(touid);
  for (const auto &txt_obj : arrays) {
    auto content = txt_obj["content"].asString();
    auto msgid = txt_obj["msgid"].asString();
    std::cout << "content is " << content << std::endl;
    std::cout << "msgid is " << msgid << std::endl;
    auto *text_msg = text_msg_req.add_textmsgs();
    text_msg->set_msgid(msgid);
    text_msg->set_msgcontent(content);
  }

  // [TODO]: 发送通知
  ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req,
                                                   rtvalue);
}
