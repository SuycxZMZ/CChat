#include "VarifyGrpcClient.h"
#include "ConfigMgr.h"

message::GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
	::grpc::ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);

	auto stub = _rpc_connection_pool->GetConnection();
	::grpc::Status status = stub->GetVarifyCode(&context, request, &reply);

	if (status.ok()) {
		_rpc_connection_pool->ReturnConnection(std::move(stub));
		return reply;
	}
	else {
		_rpc_connection_pool->ReturnConnection(std::move(stub));
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VarifyGrpcClient::VarifyGrpcClient() {
	auto& config_mgr = ConfigMgr::GetInstance();
	std::string host = config_mgr["VarifyServer"]["host"];
	std::string port = config_mgr["VarifyServer"]["port"];
	_rpc_connection_pool.reset(new RPCConnectionPool(5, host, port));
}

RPCConnectionPool::RPCConnectionPool(std::size_t poolsize, std::string host, std::string port) :
	_pool_size(poolsize),
	_host(host),
	_port(port),
	_b_stop(false)
{
	for (std::size_t i = 0; i < _pool_size; ++i) {
		std::shared_ptr<::grpc::Channel> channel = ::grpc::CreateChannel(_host + ":" + port,
			::grpc::InsecureChannelCredentials());
		_connections.push(VarifyService::NewStub(channel));
	}
}

RPCConnectionPool::~RPCConnectionPool()
{
	std::lock_guard<std::mutex> lock(_mtx);
	Close();
	while (!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<VarifyService::Stub> RPCConnectionPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(_mtx);
	_cond.wait(lock, [this] {
		if (_b_stop) {
			return true;
		}
		return !_connections.empty();
		});
	if (_b_stop) {
		return nullptr;
	}
	auto context = std::move(_connections.front());
	_connections.pop();
	return context;
}

void RPCConnectionPool::ReturnConnection(std::unique_ptr<VarifyService::Stub> context)
{
	std::lock_guard<std::mutex> lock(_mtx);
	if (_b_stop) {
		return;
	}
	_connections.emplace(std::move(context));
	_cond.notify_one();
}

void RPCConnectionPool::Close()
{
	_b_stop = true;
	_cond.notify_all();
}
