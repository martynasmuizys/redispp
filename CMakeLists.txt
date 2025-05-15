cmake_minimum_required(VERSION 4.0)
project(redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
add_subdirectory(tests)

set(SERVER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/storage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/log.cpp
)

set(CLIENT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/parser.cpp
)

add_executable(redis ${SERVER})
add_executable(redis_client ${CLIENT})

target_compile_options(
    redis PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
    -fdiagnostics-color=always
)

target_compile_options(
    redis_client PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=
    -fdiagnostics-color=always
)
