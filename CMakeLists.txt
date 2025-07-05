cmake_minimum_required(VERSION 3.15)
project(ccask VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD        11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build ccask as a shared library" ON)
option(CCASK_ENABLE_LOGGING "Enable logging support via spdlog" ON)

add_subdirectory("vendor/spdlog" EXCLUDE_FROM_ALL)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GTest REQUIRED)

set(CCASK_PUBLIC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(CCASK_SOURCES
    "src/core.c"
    "src/files.c"
    "src/keydir.c"
    "src/reader.c"
    "src/writer.c"
    "src/writer_ringbuf.c"
    "src/hint.c"
    "src/compactor.c"
    "src/records.c"
    "src/iterator.c"
    "src/errors.c"
    "src/utils.c"
    "src/log.cc")

add_library(ccask ${CCASK_SOURCES})

target_compile_features(ccask PUBLIC c_std_11)

target_include_directories(ccask
    PUBLIC
        $<BUILD_INTERFACE:${CCASK_PUBLIC_INCLUDES}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src/include
        ${ZLIB_INCLUDE_DIRS}
        vendor/uthash
        vendor/spdlog/include
)

target_link_libraries(ccask
    PRIVATE
        Threads::Threads
        ZLIB::ZLIB
)

if(CCASK_ENABLE_LOGGING)
    target_compile_definitions(ccask PRIVATE CCASK_ENABLE_LOGGING=1)
    target_link_libraries(ccask PRIVATE spdlog::spdlog)
endif()
