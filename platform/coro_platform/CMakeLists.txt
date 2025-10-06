message("Configure NetUtilsEnv Relatives")
add_library(NetUtilsEnv IOEventMonitor.cpp scheduler.cpp)
target_include_directories(
    NetUtilsEnv PUBLIC 
    . 
    ${CMAKE_SOURCE_DIR}/library
    ${CMAKE_SOURCE_DIR}/library/utils
    ${CMAKE_BINARY_DIR}/library/implement
    ${CMAKE_CURRENT_BINARY_DIR}/../implement)

