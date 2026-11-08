cmake_minimum_required(VERSION 3.20)
project(pwlck LANGUAGES CXX)

add_executable(pwlck src/main.cpp)

target_compile_options(pwlck PRIVATE -Wextra -Wall)
target_compile_features(pwlck PRIVATE cxx_std_20)
