cmake_minimum_required(VERSION 3.28)
project(idz3)

set(CMAKE_CXX_STANDARD 20)

add_executable(4-5_s 4-5/server.cpp lib.cpp)
add_executable(4-5_c 4-5/client.cpp lib.cpp)


add_executable(6-7-8_s 6-7-8/server.cpp lib.cpp)
add_executable(6-7-8_c 6-7-8/client.cpp lib.cpp)
add_executable(6-7-8_o 6-7-8/observer.cpp lib.cpp)
