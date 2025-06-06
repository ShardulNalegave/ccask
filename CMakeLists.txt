cmake_minimum_required(VERSION 3.15)
project(ccask C)

add_subdirectory(vendor/log)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MHD REQUIRED libmicrohttpd)

include_directories(${MHD_INCLUDE_DIRS})
link_directories(${MHD_LIBRARY_DIRS})

set(CCASK_HEADERS
    "src/ccask_core.h"
    "src/ccask_records.h"
    "src/ccask_keydir.h"
    "src/ccask_web_server.h"
    "src/ccask_signal_handler.h")
set(CCASK_SOURCES
    "src/main.c"
    "src/ccask_core.c"
    "src/ccask_keydir.c"
    "src/ccask_web_server.c"
    "src/ccask_signal_handler.c")

add_executable(ccask ${CCASK_HEADERS} ${CCASK_SOURCES})
target_link_libraries(ccask ${MHD_LIBRARIES})
target_link_libraries(ccask log-c)
target_include_directories(ccask PRIVATE "vendor/uthash")