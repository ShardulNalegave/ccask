cmake_minimum_required(VERSION 3.15)
project(ccask-bin C)

add_subdirectory(${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/ccask)

set(CCASK_BIN_SOURCES
    src/main.c)

add_executable(ccask-bin ${CCASK_BIN_SOURCES})

target_link_libraries(ccask-bin PRIVATE ccask)
target_include_directories(ccask-bin PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
)
