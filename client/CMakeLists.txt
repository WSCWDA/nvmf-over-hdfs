cmake_minimum_required(VERSION 2.8.12.2)
project(lfs C)

set(CMAKE_C_STANDARD 99)

add_executable(lfs rewrite.c)
