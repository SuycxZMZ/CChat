#pragma once

#include <grpcpp/grpcpp.h>
#include <atomic>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPCConnectionPool {
public:
	RPCConnectionPool(std::size_t poolsize, std::string host, std::string port);
	~RPCConnectionPool();
	std::unique_ptr<VarifyService::Stub> GetConnection();

	// 回收一个连接到pool里
	void ReturnConnection(std::unique_ptr<VarifyService::Stub> context);
	void Close();

private:
	std::atomic<bool> _b_stop;
	std::size_t _pool_size;
	std::string _host;
	std::string _port;
	std::mutex _mtx;
	std::condition_variable _cond;
	std::queue<std::unique_ptr<VarifyService::Stub>> _connections;
};

class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	message::GetVarifyRsp GetVarifyCode(std::string email);
private:
	VarifyGrpcClient();

private:
	std::unique_ptr<RPCConnectionPool> _rpc_connection_pool;
};

