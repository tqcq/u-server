cmake_minimum_required(VERSION 3.8)

# C++14 support struct Aggregate initialization
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(u-toolbox LANGUAGES C CXX VERSION 0.1.0)

find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

add_library(u-toolbox STATIC ""
        src/u-toolbox/net/dispatchers/socket_dispatcher.cpp
        src/u-toolbox/net/sockets/physical_socket.cpp
        src/u-toolbox/net/socket_address.cpp
        src/u-toolbox/net/dispatchers/signaler.cpp
        src/u-toolbox/concurrency/recursive_mutex.cpp)
target_sources(u-toolbox PRIVATE
        "src/u-toolbox/api/task_queue_base.cpp"
        "src/u-toolbox/concurrency/condition_variable.cpp"
        "src/u-toolbox/concurrency/event.cpp"
        "src/u-toolbox/concurrency/mutex.cpp"
        "src/u-toolbox/concurrency/thread.cpp"
        "src/u-toolbox/concurrency/thread_local.cpp"
        "src/u-toolbox/log/u_log.cpp"
        "src/u-toolbox/memory/memory.cpp"
        "src/u-toolbox/net/ip_address.cpp"
        "src/u-toolbox/net/socket_server.cpp"
        "src/u-toolbox/net/socket.cpp"
        "src/u-toolbox/net/physical_socket_server.cpp"
        "src/u-toolbox/net/null_socket_server.cpp"
        "src/u-toolbox/system/time_utils.cpp"
        "src/u-toolbox/third_party/sigslot/sigslot.cpp"
        )

target_include_directories(u-toolbox PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/")

target_link_libraries(u-toolbox
        PRIVATE Threads::Threads
        PUBLIC spdlog::spdlog_header_only)

set_target_properties(u-toolbox PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src/")

install(TARGETS u-toolbox
        EXPORT u-toolbox
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        PUBLIC_HEADER DESTINATION include)