#include "AsioServicePool.h"
#include <cstddef>
#include <iostream>
#include <memory>

AsioServicePool::AsioServicePool(std::size_t threadNum)
    : _io_service(threadNum), _works(threadNum), _next_IOService_idx(0) {
    for (std::size_t i = 0; i < threadNum; ++i) {
        _works[i] = std::unique_ptr<Work>(new Work(_io_service[i]));
    }
    for (std::size_t i = 0; i < threadNum; ++i) {
        _threads.emplace_back([this, i]() {
            _io_service[i].run();
            });
    }
}

AsioServicePool::~AsioServicePool() {
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioServicePool::GetIOService() {
    auto& service = _io_service[_next_IOService_idx];
    if (_next_IOService_idx == _io_service.size()) {
        _next_IOService_idx = 0;
    }
    return service;
}

void AsioServicePool::Stop() {
    for (auto& work : _works) {
        work.reset();
    }
    for (auto& t : _threads) {
        t.join();
    }
}

AsioServicePool& AsioServicePool::GetInstance() {
    static AsioServicePool instance;
    return instance;
}