#include "AsioIOServicePool.h"
#include <memory>

AsioIOServicePool::AsioIOServicePool(std::size_t size) :
	_ioServices(size),
	_works(size),
	_nextIOService(0)
{
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
	}

	for (std::size_t i = 0; i < size; ++i) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
		});
	}
}

AsioIOServicePool::~AsioIOServicePool() {
	Stop();
	std::cout << "---------------- AsioIOServicePool destruct ----------------" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];
	if (_nextIOService >= _ioServices.size()) {
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop() {
	for (auto& work : _works) {
		work->get_io_context().stop();
		work.reset();
	}

	for (auto& t : _threads) {
		t.join();
	}
}

