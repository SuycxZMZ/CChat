#ifndef ASIOSERVICEPOOL_H
#define ASIOSERVICEPOOL_H

#include <boost/asio/io_context.hpp>
#include <memory>
#include <thread>
#include <vector>

class AsioServicePool {
public:
  using IOService = boost::asio::io_context;
  using Work = boost::asio::io_context::work;
  using WorkPtr = std::shared_ptr<Work>;

  ~AsioServicePool();
  AsioServicePool(const AsioServicePool &) = delete;
  AsioServicePool &operator=(const AsioServicePool &) = delete;

  boost::asio::io_context &GetIOService();
  void Stop();
  static AsioServicePool &GetInstance();

private:
  AsioServicePool(std::size_t threadNum = std::thread::hardware_concurrency());
  std::vector<IOService> _io_service;
  std::vector<WorkPtr> _works;
  std::vector<std::thread> _threads;
  std::size_t _next_IOService_idx;
};

#endif