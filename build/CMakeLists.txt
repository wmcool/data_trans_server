cmake_minimum_required(VERSION 3.20)
project(server)

set(CMAKE_CXX_STANDARD 14)

add_executable(server ../main.cpp ../utils.cpp ../include/utils.h)
