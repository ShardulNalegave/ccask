cmake_minimum_required(VERSION 3.15)
project(ccask VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CCASK_ENABLE_LOGGING "Enable logging support via spdlog" ON)
option(CCASK_BUILD_SHARED "Build shared library" OFF)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

if(CCASK_ENABLE_LOGGING)
    find_package(spdlog REQUIRED)
endif()

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
    "src/utils.c"
    "src/status.c"
    "src/log.cc"
)

if(CCASK_BUILD_SHARED)
    add_library(ccask SHARED ${CCASK_SOURCES})
else()
    add_library(ccask STATIC ${CCASK_SOURCES})
endif()

add_library(ccask::ccask ALIAS ccask)

set_target_properties(ccask PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(ccask
    PUBLIC
        $<BUILD_INTERFACE:${CCASK_PUBLIC_INCLUDES}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/uthash
)

target_link_libraries(ccask
    PUBLIC
        Threads::Threads
        ZLIB::ZLIB
)

if(CCASK_ENABLE_LOGGING)
    target_compile_definitions(ccask PRIVATE CCASK_ENABLE_LOGGING=1)
    target_link_libraries(ccask PRIVATE spdlog::spdlog)
endif()

# Compiler-specific options
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ccask PRIVATE 
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
endif()

install(TARGETS ccask
    EXPORT ccaskTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT ccaskTargets
    FILE ccaskTargets.cmake
    NAMESPACE ccask::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccask
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ccaskConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ccaskConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccask
    NO_SET_AND_CHECK_MACRO
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ccaskConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ccaskConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ccaskConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccask
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
    "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

message(STATUS "")
message(STATUS "ccask Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
message(STATUS "  Enable logging: ${CCASK_ENABLE_LOGGING}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
