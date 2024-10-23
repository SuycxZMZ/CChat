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
