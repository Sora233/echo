cmake_minimum_required(VERSION 3.10)
project(echo_select C)

set(CMAKE_C_STANDARD 11)

add_executable(echo_select_server select_server.c def.h)
add_executable(echo_select_client select_client.c def.h)
add_executable(echo_epoll_server epoll_server.c def.h)
add_executable(echo_epoll_client epoll_client.c def.h)
add_executable(echo_poll_client poll_client.c def.h)
add_executable(echo_poll_server poll_server.c def.h)