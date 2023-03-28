cmake_minimum_required(VERSION 3.20)
project(examples)

# common includes & library
include_directories(../)
link_libraries(usefull_macros improc m)

# exe list
add_executable(equalize equalize.c)
add_executable(generate generate.c)
add_executable(genu16 genu16.c)
