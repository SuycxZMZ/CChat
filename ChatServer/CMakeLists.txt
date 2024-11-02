set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/ChatServer)
file(GLOB ChatServer_SRC "*.cpp" "*.cc")
add_executable(ChatServer ${ChatServer_SRC})
target_link_libraries(ChatServer ${COMMON_LIBS})
