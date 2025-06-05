cmake_minimum_required(VERSION 3.15)
project(ccask C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MHD REQUIRED libmicrohttpd)

include_directories(${MHD_INCLUDE_DIRS})
link_directories(${MHD_LIBRARY_DIRS})

set(CCASK_HEADERS
    "src/ccask_records.h")
set(CCASK_SOURCES
    "src/main.c")

add_executable(ccask ${CCASK_HEADERS} ${CCASK_SOURCES})
target_link_libraries(ccask ${MHD_LIBRARIES})
