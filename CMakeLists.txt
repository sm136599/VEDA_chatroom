cmake_minimum_required(VERSION 3.10)

project(VEDA_chatroom)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/client/include)

include_directories(${CMAKE_SOURCE_DIR}/server/include)

set(CLIENT_SOURCES
    ${CMAKE_SOURCE_DIR}/client/client.c    
)

set(SERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/server/main.c
    ${CMAKE_SOURCE_DIR}/server/src/server.c
    ${CMAKE_SOURCE_DIR}/server/src/connection.c
    ${CMAKE_SOURCE_DIR}/server/src/signal_handlers.c
    ${CMAKE_SOURCE_DIR}/server/src/utils.c
)

add_executable(client ${CLIENT_SOURCES})

add_executable(server ${SERVER_SOURCES})

