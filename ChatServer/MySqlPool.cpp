#include "MySqlPool.h"
#include "ConfigMgr.h"
#include "const.h"
#include <chrono>

MySqlPool::MySqlPool(const std::string& url, const std::string& user,
	const std::string& pass, const std::string& schema, int poolSize) :
	_url(url), _user(user), _pass(pass), _schema(schema), _pool_size(poolSize), _b_stop(false)
{
	try {
		for (int i = 0; i < poolSize; ++i) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* conn = driver->connect(_url, _user, _pass);
			conn->setSchema(_schema);
			// 获取当前时间戳
			auto current_time = std::chrono::system_clock::now().time_since_epoch();
			auto time_stamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
			_sql_connection_pool.push(std::make_unique<sqlConnection>(conn, time_stamp));
		}

		_check_thread = std::thread([this]() {
			while (!_b_stop) {
				CheckConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
		});
		_check_thread.detach();
	} 
	catch (sql::SQLException& e) {
		std::cout << "mysql pool init failed" << std::endl;
	}
}

std::unique_ptr<sqlConnection> MySqlPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(_mtx);
	_cond.wait(lock, [this]() {
		if (_b_stop) {
			return true;
		}
		return !_sql_connection_pool.empty();
	});
	if (_b_stop) {
		return nullptr;
	}
	std::unique_ptr<sqlConnection> conn(std::move(_sql_connection_pool.front()));
	_sql_connection_pool.pop();
	return conn;
}

void MySqlPool::ReturnConnection(std::unique_ptr<sqlConnection> con)
{
	std::unique_lock<std::mutex> lock(_mtx);
	if (_b_stop) {
		return;
	}
	_sql_connection_pool.emplace(std::move(con));
	_cond.notify_one();
}

void MySqlPool::Close()
{
	_b_stop = true;
	_cond.notify_all();
}

void MySqlPool::CheckConnection()
{
	std::lock_guard<std::mutex> lock(_mtx);
	int pool_size = _sql_connection_pool.size();
	auto current_time = std::chrono::system_clock::now().time_since_epoch();
	auto time_stamp = std::chrono::duration_cast<std::chrono::seconds>(current_time).count();
	for (int i = 0; i < pool_size; ++i) {
		auto conn = std::move(_sql_connection_pool.front());
		_sql_connection_pool.pop();

		Defer defer([this, &conn]() {
			_sql_connection_pool.push(std::move(conn));
		});

		if (time_stamp - conn->_last_op_time < 5) {
			continue;
		}

		try {
			std::unique_ptr<sql::Statement> stmt(conn->_conn->createStatement());
			stmt->executeQuery("SELECT 1");
			conn->_last_op_time = time_stamp;

		} 
		catch (sql::SQLException& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			// 重新创建连接
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* newconn = driver->connect(_url, _user, _pass);
			newconn->setSchema(_schema);
			conn->_conn.reset(newconn);
			conn->_last_op_time = time_stamp;
		}
	}
}

MySqlPool::~MySqlPool()
{
	std::unique_lock<std::mutex> lock(_mtx);
	while (!_sql_connection_pool.empty()) {
		_sql_connection_pool.pop();
	}
}

MySqlDao::MySqlDao()
{
	auto& config_mgr = ConfigMgr::GetInstance();
	const auto& host = config_mgr["MySql"]["host"];
	const auto& port = config_mgr["MySql"]["port"];
	const auto& pwd = config_mgr["MySql"]["passwd"];
	const auto& schema = config_mgr["MySql"]["schema"];
	const auto& user = config_mgr["MySql"]["user"];
	_pool.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MySqlDao::~MySqlDao()
{
	_pool->Close();
}

int MySqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto conn = _pool->GetConnection();
	try {
		if (conn == nullptr) {
			return false;
		}
		// 准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(conn->_conn->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// 设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);
		// 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值
		// 执行存储过程
		stmt->execute();
		// 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
	    // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
		std::unique_ptr<sql::Statement> stmtResult(conn->_conn->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "result:" << result << std::endl;
			_pool->ReturnConnection(std::move(conn));
			return result;
		}
		_pool->ReturnConnection(std::move(conn));
		return -1;
	}
	catch (sql::SQLException& e) {
		_pool->ReturnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MySqlDao::CheckEmail(const std::string& name, const std::string& email)
{
	auto conn = _pool->GetConnection();
	try {
		if (conn == nullptr) {
			return false;
		}

		std::unique_ptr <sql::PreparedStatement> pstmt(conn->_conn->prepareStatement("SELECT email FROM user WHERE name = ?"));
		pstmt->setString(1, name);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				_pool->ReturnConnection(std::move(conn));
				return false;
			}
			_pool->ReturnConnection(std::move(conn));
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		_pool->ReturnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MySqlDao::UpdatePwd(const std::string& name, const std::string& newpwd)
{
	auto conn = _pool->GetConnection();
	try {
		if (conn == nullptr) {
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(conn->_conn->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		_pool->ReturnConnection(std::move(conn));
		return true;
	}
	catch (sql::SQLException& e) {
		_pool->ReturnConnection(std::move(conn));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MySqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto conn = _pool->GetConnection();
	if (conn == nullptr) {
		return false;
	}
	Defer defer([this, &conn]() {
		_pool->ReturnConnection(std::move(conn));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(conn->_conn->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); 
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

std::shared_ptr<UserInfo> MySqlDao::GetUser(int uid)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// 遍历结果集
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->uid = uid;
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

std::shared_ptr<UserInfo> MySqlDao::GetUser(std::string name)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name); 

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->uid = res->getInt("uid");
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

bool MySqlDao::AddFriendApply(const int& from, const int& to)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));
		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MySqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});


	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("select apply.from_uid, apply.status, user.name, "
			"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid); 
		pstmt->setInt(2, begin); 
		pstmt->setInt(3, limit); 

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) {
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			applyList.push_back(apply_ptr);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MySqlDao::AuthFriendApply(const int& from, const int& to)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));
		pstmt->setInt(1, to);
		pstmt->setInt(2, from);
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
	return true;
}

bool MySqlDao::AddFriend(const int& from, const int& to, std::string back_name)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		con->_conn->setAutoCommit(false);
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
		));
		pstmt->setInt(1, from); 
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			con->_conn->rollback();
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_conn->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "
		));
		pstmt2->setInt(1, to); 
		pstmt2->setInt(2, from);
		pstmt2->setString(3, "");
		int rowAffected2 = pstmt2->executeUpdate();
		if (rowAffected2 < 0) {
			con->_conn->rollback();
			return false;
		}

		con->_conn->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_conn->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
	return true;
}

bool MySqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
{
	auto con = _pool->GetConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->ReturnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_conn->prepareStatement("select * from friend where self_id = ? "));

		pstmt->setInt(1, self_id); 

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			auto friend_id = res->getInt("friend_id");
			auto back = res->getString("back");
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				continue;
			}

			user_info->back = user_info->name;
			user_info_list.push_back(user_info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}
