cmake_minimum_required(VERSION 3.15)
project(ccask C)

add_subdirectory(vendor/log)

find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MHD REQUIRED libmicrohttpd)

include_directories(${MHD_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
link_directories(${MHD_LIBRARY_DIRS})

set(CCASK_HEADERS
    "src/ccask_core.h"
    "src/ccask_files.h"
    "src/ccask_records.h"
    "src/ccask_records_iter.h"
    "src/ccask_hintfile_gen.h"
    "src/ccask_keydir.h"
    "src/ccask_web_server.h"
    "src/ccask_signal_handler.h"
    "src/ccask_crc_check.h")
set(CCASK_SOURCES
    "src/main.c"
    "src/ccask_core.c"
    "src/ccask_files.c"
    "src/ccask_records.c"
    "src/ccask_records_iter.c"
    "src/ccask_hintfile_gen.c"
    "src/ccask_keydir.c"
    "src/ccask_web_server.c"
    "src/ccask_signal_handler.c"
    "src/ccask_crc_check.c")

add_executable(ccask ${CCASK_HEADERS} ${CCASK_SOURCES})
target_link_libraries(ccask PRIVATE log-c ${MHD_LIBRARIES} ZLIB::ZLIB)
target_include_directories(ccask PRIVATE "vendor/uthash")