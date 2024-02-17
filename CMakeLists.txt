cmake_minimum_required(VERSION 3.20)
project(Assignment2 C)

set(CMAKE_C_STANDARD 11)

add_executable(Assignment2 main.c)

target_link_libraries(Assignment2 PUBLIC m)

