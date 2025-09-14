cmake_minimum_required(VERSION 3.16)
project(mydb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(parser)
add_subdirectory(storage)
