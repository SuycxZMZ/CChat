#include "VarifyGrpcClient.h"

message::GetVarifyRsp VarifyGrpcClient::GetVarifyCode(std::string email) {
	::grpc::ClientContext context;
	GetVarifyRsp reply;
	GetVarifyReq request;
	request.set_email(email);
	::grpc::Status status = _stub->GetVarifyCode(&context, request, &reply);
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

VarifyGrpcClient::VarifyGrpcClient() {
	std::shared_ptr<::grpc::Channel> channel =
		::grpc::CreateChannel("127.0.0.1:50051", ::grpc::InsecureChannelCredentials());
	_stub = VarifyService::NewStub(channel);
}