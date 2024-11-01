#include <iostream>
#include <thread>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <boost/asio.hpp>
#include "ConfigMgr.h"
#include "AsioServicePool.h"
#include "CServer.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "const.h"

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
	auto& cfg = ConfigMgr::GetInstance();
	auto server_name = cfg["SelfServer"]["name"];

	try {
		auto & pool = AsioServicePool::GetInstance();
		//将登录数设置为0
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
		std::cout << "----------------- HSet LOGIN_COUNT -----------------" << std::endl;
		//定义一个GrpcServer
		std::string server_address(cfg["SelfServer"]["host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// 监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(std::move(builder.BuildAndStart()));
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
		});

		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, &pool, &server](auto, auto) {
			io_context.stop();
			pool.Stop();
			server->Shutdown();
		});

		auto port_str = cfg["SelfServer"]["port"];
		std::cout << "----------------" << "ChatServer2 start at port:"
			<< port_str << "----------------" << std::endl;
		CServer s(io_context, atoi(port_str.c_str()));
		io_context.run();

		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}
}

