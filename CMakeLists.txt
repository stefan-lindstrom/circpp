cmake_minimum_required(VERSION 2.8)
project(circle)

add_definitions(-std=c++11 -Wl,--no-as-needed)

include_directories(inc/)
add_subdirectory(inc/)

file(GLOB circle_src "src/*.cpp")

add_executable(bin/circle ${circle_src})
target_link_libraries(bin/circle pthread crypt)
