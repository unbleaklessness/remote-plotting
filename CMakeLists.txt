cmake_minimum_required(VERSION 3.21)
project(remote_plotting)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}_executable main.cpp)
target_link_libraries(${PROJECT_NAME}_executable pthread)

add_library(${PROJECT_NAME}_library main.cpp)
target_link_libraries(${PROJECT_NAME}_library pthread)
