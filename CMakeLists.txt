cmake_minimum_required(VERSION 3.10)

project(TestMMap LANGUAGES CXX)

add_executable(test_mmap test_mmap.cpp mmap.cpp)
