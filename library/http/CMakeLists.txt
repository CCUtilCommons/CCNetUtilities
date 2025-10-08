message("Configure HttpBase")

print_banner("Installing the must libraies for http base")

message("Including the nlohmann::json for the http base json parser")

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)


add_library(HttpBase 
            methods.cpp 
            http_response.cpp
            http_request.cpp
            http_status_code.cpp
            json_helper/json_to_http.cpp)
target_include_directories(
    HttpBase PUBLIC 
    . 
    ..
    ../utils)
target_link_libraries(HttpBase PRIVATE nlohmann_json::nlohmann_json)

