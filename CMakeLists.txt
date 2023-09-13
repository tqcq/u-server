cmake_minimum_required(VERSION 3.5)
project(http_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("u-toolbox")

add_executable(http_server main.cpp)
add_subdirectory("third_party/marl")

target_link_libraries(http_server PRIVATE
        u-toolbox
        marl)

install(FILES "index.html" DESTINATION .)
