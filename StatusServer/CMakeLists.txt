set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/StatusServer)
file(GLOB StatusServer_SRC "*.cpp" "*.cc")
add_executable(StatusServer ${StatusServer_SRC})
target_link_libraries(StatusServer ${COMMON_LIBS})
