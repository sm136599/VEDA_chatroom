cmake_minimum_required(VERSION 3.10)

project(VEDA_chatroom)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CLIENT_SOURCES
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/src/client.c
    ${CMAKE_SOURCE_DIR}/src/connection.c
    ${CMAKE_SOURCE_DIR}/src/signal_handlers.c
    ${CMAKE_SOURCE_DIR}/src/auth.c
)
add_executable(client ${CLIENT_SOURCES})
