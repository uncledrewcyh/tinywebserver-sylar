Cmake_minimum_required(VERSION 2.8)
project(pudge)
include (cmake/utils.cmake) #cmkae子函数封装库

set(CMAKE_VERBOSE_MAKEFILE ON) # make文件显示详细信息
# $ENV{VAR}读取 VAR环境变量
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-builtin-macro-redefined -Wno-deprecated-declarations")


include_directories(.)

find_package(yaml-cpp REQUIRED)
message(STATUS "${YAML_CPP_LIBRARIES}")

set(LIB_SRC
    src/log.cc
    src/config.cc
    src/mutex.cc
    src/env.cc
    src/util.cc
    src/thread.cc
    src/fiber.cc
    src/scheduler.cc
    src/iomanager.cc
    src/timer.cc
    src/hook.cc
    src/fd_manager.cc
    src/address.cc
    src/socket.cc
    src/bytearray.cc
    src/stream.cc
    src/stream/socket_stream.cc
    src/http/http.cc
    src/http/http11_parser.cc
    src/http/httpclient_parser.cc
    src/http/http_parser.cc
    src/http/servlet.cc
    src/http/http_server.cc
    src/http/http_session.cc
    src/tcp_server.cc
    src/db/mysql_pool.cc
)

# 生成网络库
add_library(pudge SHARED ${LIB_SRC})
redefine_file_macro(pudge) # 改变__FILE__为相对路径

# 设置库文件,类似-lpthread
set(LIBS
    pudge
    pthread
    dl
    mysqlclient
    ${YAML_CPP_LIBRARIES}
)



my_add_executable(tiny_webserver "example/tiny_webserver.cc" pudge "${LIBS}")
my_add_executable(test_mysql_pool "tests/test_mysql_pool.cc" pudge "${LIBS}")
# my_add_executable(test_log "tests/test_log.cc" pudge "${LIBS}")
# my_add_executable(test_foo "tests/test_foo.cc" pudge "${LIBS}")
# my_add_executable(test_config "tests/test_config.cc" pudge "${LIBS}")
# my_add_executable(test_env "tests/test_env.cc" pudge "${LIBS}")
# my_add_executable(test_config_log "tests/test_config_log.cc" pudge "${LIBS}")
# my_add_executable(test_thread "tests/test_thread.cc" pudge "${LIBS}")
# my_add_executable(test_assert "tests/test_assert.cc" pudge "${LIBS}")
# my_add_executable(test_ucontext "tests/test_ucontext.cc" pudge "${LIBS}")
# my_add_executable(test_fiber "tests/test_fiber.cc" pudge "${LIBS}")
# my_add_executable(test_thread_fiber "tests/test_thread_fiber.cc" pudge "${LIBS}")
# my_add_executable(test_scheduler "tests/test_scheduler.cc" pudge "${LIBS}")
# my_add_executable(test_iomanager "tests/test_iomanager.cc" pudge "${LIBS}")
# my_add_executable(test_mutex "tests/test_mutex.cc" pudge "${LIBS}")
# my_add_executable(test_getTime "tests/test_getTime.cc" pudge "${LIBS}")
# my_add_executable(test_timer "tests/test_timer.cc" pudge "${LIBS}")
# my_add_executable(test_hook "tests/test_hook.cc" pudge "${LIBS}")
# my_add_executable(test_bool_bit "tests/test_bool_bit.cc" pudge "${LIBS}")
# my_add_executable(test_hook_socket "tests/test_hook_socket.cc" pudge "${LIBS}")
# my_add_executable(test_variadic_template "tests/test_variadic_template.cc" pudge "${LIBS}")
# my_add_executable(test_endian "tests/test_endian.cc" pudge "${LIBS}")
# my_add_executable(test_address "tests/test_address.cc" pudge "${LIBS}")
# my_add_executable(test_socket "tests/test_socket.cc" pudge "${LIBS}")
# my_add_executable(test_bytearray "tests/test_bytearray.cc" pudge "${LIBS}")
# my_add_executable(test_uricode "tests/test_uricode.cc" pudge "${LIBS}")
# my_add_executable(test_http_parser "tests/test_http_parser.cc" pudge "${LIBS}")
# my_add_executable(test_tcp_server "tests/test_tcp_server.cc" pudge "${LIBS}")
# my_add_executable(echo_server "tests/echo_server.cc" pudge "${LIBS}")
# my_add_executable(test_http_server "tests/test_http_server.cc" pudge "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)