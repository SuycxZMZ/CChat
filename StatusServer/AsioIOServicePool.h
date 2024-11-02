#pragma once

#include "Singleton.h"
#include <boost/asio.hpp>
#include <vector>

class AsioIOServicePool : public Singleton<AsioIOServicePool> {
  friend class Singleton<AsioIOServicePool>;

public:
  using IOService = boost::asio::io_context;
  using Work = boost::asio::io_context::work;
  using WorkPtr = std::unique_ptr<Work>;

  ~AsioIOServicePool();
  AsioIOServicePool(const AsioIOServicePool &) = delete;
  AsioIOServicePool &operator=(const AsioIOServicePool &) = delete;
  boost::asio::io_context &GetIOService();
  void Stop();

private:
  // 没有用 hardware_concurrency 因为放到本机来跑，没那么多线程
  AsioIOServicePool(std::size_t size = 2);

private:
  std::vector<IOService> _ioServices;
  std::vector<WorkPtr> _works;
  std::vector<std::thread> _threads;
  std::size_t _nextIOService;
};
