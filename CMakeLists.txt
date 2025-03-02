cmake_minimum_required(VERSION 3.10)

project(serializer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

file(GLOB_RECURSE HEADERS ./*.h)

add_library(serializer INTERFACE)
target_include_directories(serializer INTERFACE .)

set_target_properties(serializer PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(serializer PROPERTIES PUBLIC_HEADER "${HEADERS}")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_link_libraries(serializer INTERFACE nlohmann_json::nlohmann_json)