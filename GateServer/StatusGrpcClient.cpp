#include "StatusGrpcClient.h"
#include "ConfigMgr.h"
#include "const.h"

StatusConnPool::StatusConnPool(size_t pool_size, std::string host,
                               std::string port)
    : _pool_size(pool_size), _host(host), _port(port), _b_stop(false) {
  for (size_t i = 0; i < pool_size; ++i) {
    std::shared_ptr<::grpc::Channel> channel = ::grpc::CreateChannel(
        host + ":" + port, ::grpc::InsecureChannelCredentials());
    _grpc_conn_pool.push(message::StatusService::NewStub(channel));
  }
}

StatusConnPool::~StatusConnPool() {
  std::lock_guard<std::mutex> lock(_mtx);
  Close();
  while (!_grpc_conn_pool.empty()) {
    _grpc_conn_pool.pop();
  }
}

std::unique_ptr<message::StatusService::Stub> StatusConnPool::GetConnection() {
  std::unique_lock<std::mutex> lock(_mtx);
  _cond.wait(lock, [this]() {
    if (_b_stop) {
      return true;
    }
    return !_grpc_conn_pool.empty();
  });
  if (_b_stop) {
    return nullptr;
  }
  auto context = std::move(_grpc_conn_pool.front());
  _grpc_conn_pool.pop();
  return context;
}

void StatusConnPool::ReturnConnection(
    std::unique_ptr<message::StatusService::Stub> context) {
  std::lock_guard<std::mutex> lock(_mtx);
  if (_b_stop) {
    return;
  }
  _grpc_conn_pool.push(std::move(context));
  _cond.notify_one();
}

void StatusConnPool::Close() {
  _b_stop = true;
  _cond.notify_all();
}

message::GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
  ::grpc::ClientContext context;
  message::GetChatServerReq request;
  message::GetChatServerRsp reply;
  request.set_uid(uid);
  auto stub = _pool->GetConnection();
  ::grpc::Status status = stub->GetChatServer(&context, request, &reply);
  Defer defer([&stub, this] { _pool->ReturnConnection(std::move(stub)); });

  if (status.ok()) {
    return reply;
  } else {
    reply.set_error(ErrorCodes::RPCFailed);
    return reply;
  }
}

StatusGrpcClient::StatusGrpcClient() {
  auto &config_mgr = ConfigMgr::GetInstance();
  auto host = config_mgr["StatusServer"]["host"];
  auto port = config_mgr["StatusServer"]["port"];
  _pool.reset(new StatusConnPool(5, host, port));
}