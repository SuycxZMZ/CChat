#pragma once

#include "message.grpc.pb.h"
#include "message.pb.h"
#include <grpcpp/grpcpp.h>
#include <mutex>

// StatusServer路由到的具体聊天服务器，返回给客户端
class ChatServer {
public:
  ChatServer() : host(""), port(""), name(""), con_count(0) {}
  ChatServer(const ChatServer &cs)
      : host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count) {}
  ChatServer &operator=(const ChatServer &cs) {
    if (&cs == this) {
      return *this;
    }

    host = cs.host;
    name = cs.name;
    port = cs.port;
    con_count = cs.con_count;
    return *this;
  }
  std::string host;
  std::string port;
  std::string name;
  int con_count;
};

class StatusServiceImpl : public message::StatusService::Service {
public:
  StatusServiceImpl();
  ::grpc::Status GetChatServer(::grpc::ServerContext *context,
                               const message::GetChatServerReq *request,
                               message::GetChatServerRsp *reply) override;
  ::grpc::Status Login(::grpc::ServerContext *context,
                       const message::LoginReq *request,
                       message::LoginRsp *reply) override;

private:
  void InsertToken(int uid, std::string token);
  ChatServer GetChatServer();

private:
  std::unordered_map<std::string, ChatServer> _servers;
  std::mutex _mtx;
};

extern std::string generate_unique_string();
