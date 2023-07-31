cmake_minimum_required(VERSION 2.8)
project(circle)

add_definitions(-std=c++11 -Wl,--no-as-needed -Wall -Werror -ggdb)

include_directories(inc/)
add_subdirectory(inc/)

add_subdirectory(src/)

add_executable(bin/circle ${circle_src})
target_link_libraries(bin/circle pthread crypt)

add_executable(bin/autowiz ${autowiz_src})
add_executable(bin/listrent ${listrent_src})
add_executable(bin/play2to3 ${play2to3_src})
add_executable(bin/shopconv ${shopconv_src})

#add_executable(bin/sign ${sign_src})

add_executable(bin/wld2html ${wld2html_src})
add_executable(bin/delobjs ${delobjs_src})
add_executable(bin/mudpasswd ${mudpasswd_src})
target_link_libraries(bin/mudpasswd crypt)
add_executable(bin/purgeplay ${purgeplay_src})
add_executable(bin/showplay ${showplay_src})
add_executable(bin/split ${split_src})
