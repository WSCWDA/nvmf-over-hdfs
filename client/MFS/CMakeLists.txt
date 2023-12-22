cmake_minimum_required(VERSION 2.8.12.2)
project(MFS)

set(CMAKE_CXX_STANDARD 17)

add_executable(MFS main.cpp common.h bitmap.cpp init.cpp)
