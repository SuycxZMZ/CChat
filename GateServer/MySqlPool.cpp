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
