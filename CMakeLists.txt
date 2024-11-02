cmake_minimum_required(VERSION 3.14)

project(CChat)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64 -std=c++20")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# find_package(hiredis REQUIRED) # 查找 hiredis
# find_package(MySQLConnector REQUIRED) # 查找 MySQL Connector/C++
set(COMMON_LIBS
  Boost::system
  Boost::filesystem
  pthread
  gRPC::grpc++
  jsoncpp
  hiredis
  mysqlcppconn
)

add_subdirectory(ChatServer)
add_subdirectory(ChatServer2)
add_subdirectory(GateServer)
add_subdirectory(StatusServer)