set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ChatServer2)
file(GLOB ChatServer_SRC "*.cpp" "*.cc")
add_executable(ChatServer2 ${ChatServer_SRC})
target_link_libraries(ChatServer2 ${COMMON_LIBS})
