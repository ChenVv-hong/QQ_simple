cmake_minimum_required(VERSION 3.20)
project(QQ_simple)

set(CMAKE_CXX_STANDARD 14)

add_executable(QQ_simple main.cpp)
add_executable(server ./server/server.cpp)
add_executable(client ./client/client.cpp)