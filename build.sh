#!/bin/bash

# 启用调试输出
set -x

# 解析命令行参数
BUILD_TYPE="Debug"
BUILD_DIR="cmake-build-debug"

if [[ "$1" == "Release" ]]; then
    BUILD_TYPE="Release"
    BUILD_DIR="cmake-build-release"
fi

# 编译 proto 文件
for SERVER in ChatServer ChatServer2 GateServer StatusServer; do
    protoc --cpp_out="$SERVER" --grpc_out="$SERVER" --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) message.proto
done

# 检查并创建构建目录
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"

# 进入构建目录
cd "$BUILD_DIR" || exit

# 根据构建类型生成 Makefile
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..

# 编译项目
make -j4

cd ..

# 拷贝配置文件（如果需要的话）
cp ChatServer/config.ini bin/ChatServer/
cp ChatServer2/config.ini bin/ChatServer2/
cp GateServer/config.ini bin/GateServer/
cp StatusServer/config.ini bin/StatusServer/
