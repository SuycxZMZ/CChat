#pragma once

#include "MySqlPool.h"
#include "Singleton.h"

class MySqlDao;

class MySqlMgr : public Singleton<MySqlMgr>
{
	friend class Singleton<MySqlMgr>;
public:
	~MySqlMgr();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
private:
	MySqlMgr();

private:
	MySqlDao _dao;
};

