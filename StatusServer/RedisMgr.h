#pragma once

#include "const.h"
#include "Singleton.h"

class RedisConnectionPool;

class RedisMgr : public Singleton<RedisMgr>, public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Auth(const std::string& passwd);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistKey(const std::string& key);
	void Close();
private:
	RedisMgr();
private:
	std::unique_ptr<RedisConnectionPool> _redis_connection_pool;
};

extern void TestRedisMgr();

class RedisConnectionPool {

public:
	RedisConnectionPool(std::size_t pool_size, const char* host, int port, const char* pwd);
	~RedisConnectionPool();
	redisContext* GetConnection();
	void ReturnConnection(redisContext* context);
	void Close();
private:
	std::atomic<bool> _b_stop;
	std::size_t _pool_size;
	const char* _host;
	int _port;
	std::queue<redisContext*> _connections;
	std::mutex _mtx;
	std::condition_variable _cond;
};

