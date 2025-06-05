cmake_minimum_required(VERSION 3.10)
project(log-c)

add_library(log-c "log/src/log.c" "log/src/log.h")
target_include_directories(log-c PUBLIC "log/src")
target_compile_features(log-c PUBLIC c_std_99)
