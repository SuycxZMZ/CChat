#pragma once

#include "MySqlPool.h"
#include "Singleton.h"
#include <memory>

class MySqlDao;

class MySqlMgr : public Singleton<MySqlMgr>
{
	friend class Singleton<MySqlMgr>;
public:
	~MySqlMgr();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::shared_ptr<UserInfo> GetUser(int uid);
	std::shared_ptr<UserInfo> GetUser(std::string name);
	bool AddFriendApply(const int& from, const int& to);
	bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit = 10);
	bool AuthFriendApply(const int& from, const int& to);
	bool AddFriend(const int& from, const int& to, std::string back_name);
	bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);
private:
	MySqlMgr();

private:
	MySqlDao _dao;
};

