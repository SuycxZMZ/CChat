#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include "const.h"
#include "const.h"

class CSession;
class LogicNode;

class LogicSystem {
public:
    using FuncCallBack = std::function<void(std::shared_ptr<CSession>,
        const short&, const std::string&)>;
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
    static LogicSystem& GetInstance();

private:
    LogicSystem();
    void DealMsg();
    void RegisterCallBacks();
    void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    //void SearchInfo(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    //void AddFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    //void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    //void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    //bool isPureDigit(const std::string& str);
    //void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
    //void GetUserByName(std::string name, Json::Value& rtvalue);
    //bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    //bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
    //bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list);

private:
    std::thread _work_thread;
    std::mutex _mutex;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::condition_variable _consume;
    bool _b_stop;
    std::map<short, FuncCallBack> _func_callbacks;
    std::unordered_map<int, std::shared_ptr<UserInfo>> _users;
};

#endif
