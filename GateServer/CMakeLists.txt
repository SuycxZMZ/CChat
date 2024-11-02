set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/GateServer)
file(GLOB GateServer_SRC "*.cpp" "*.cc")
add_executable(GateServer ${GateServer_SRC})
target_link_libraries(GateServer ${COMMON_LIBS})
