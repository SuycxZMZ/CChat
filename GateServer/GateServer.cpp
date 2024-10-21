#include "const.h"
#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
    // ---------------- 加载配置文件 ---------------- //
    auto & gate_all_config_mgr = ConfigMgr::GetInstance();
    std::string gate_port_str = gate_all_config_mgr["GateServer"]["port"];
    unsigned short gate_port = std::stoi(gate_port_str);

    try {
        unsigned short port = gate_port;
        net::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
        });

        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "---------------- GateServer start at port:" 
            << port << " ----------------" << std::endl;
        ioc.run();
    }
    catch (std::exception& exp) {
        std::cerr << "Error:" << exp.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
};