#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	message::GetVarifyRsp GetVarifyCode(std::string email);
private:
	VarifyGrpcClient();

private:
	std::unique_ptr<VarifyService::Stub> _stub;
};

