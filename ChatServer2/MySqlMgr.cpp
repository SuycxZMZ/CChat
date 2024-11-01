#include "MySqlMgr.h"

MySqlMgr::~MySqlMgr()
{
}

MySqlMgr::MySqlMgr()
{
}

int MySqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	return _dao.RegUser(name, email, pwd);
}

bool MySqlMgr::CheckEmail(const std::string& name, const std::string& email)
{
	return _dao.CheckEmail(name, email);
}

bool MySqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

bool MySqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
	return _dao.CheckPwd(email, pwd, userInfo);
}

std::shared_ptr<UserInfo> MySqlMgr::GetUser(int uid)
{
	return _dao.GetUser(uid);
}

std::shared_ptr<UserInfo> MySqlMgr::GetUser(std::string name)
{
	return _dao.GetUser(name);
}

bool MySqlMgr::AddFriendApply(const int& from, const int& to)
{
	return _dao.AddFriendApply(from, to);
}

bool MySqlMgr::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
	return _dao.GetApplyList(touid, applyList, begin, limit);
}



