message("Configure HttpBase")
add_library(HttpBase 
            methods.cpp 
            http_response.cpp
            http_request.cpp
            http_status_code.cpp)
target_include_directories(
    HttpBase PUBLIC 
    . 
    ..
    ../utils)


