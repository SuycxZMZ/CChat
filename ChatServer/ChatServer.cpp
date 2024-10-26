#include <iostream>
#include <thread>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <boost/asio.hpp>
#include "ConfigMgr.h"
#include "AsioServicePool.h"
#include "CServer.h"

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()
{
	try {
		auto& config_mgr = ConfigMgr::GetInstance();
		auto& pool = AsioServicePool::GetInstance();
		boost::asio::io_context io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, &pool](auto, auto) {
			io_context.stop();
			pool.Stop();
		});
		auto port_str = config_mgr["SelfServer"]["port"];
		std::cout << "---------------- ChatServer start at : "
			<< port_str << " ----------------" << std::endl;
		CServer s(io_context, atoi(port_str.c_str()));
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	return 0;
}

