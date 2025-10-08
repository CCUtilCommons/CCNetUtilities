message("Configure CoroHttp")
add_library(CoroHttp 
            coro_http_reader.cpp
            coro_http_writer.cpp)

target_include_directories(
    CoroHttp PUBLIC 
    . 
    ..)

target_link_libraries(CoroHttp PUBLIC HttpBase CoroSysSocket)



