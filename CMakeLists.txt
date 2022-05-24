cmake_minimum_required(VERSION 3.21)
project(remote_plotting)
add_compile_options("-static")

set(CMAKE_CXX_STANDARD 17)

add_executable(remote_plotting main.cpp)
target_link_libraries(remote_plotting pthread)

