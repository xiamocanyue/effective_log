cmake_minimum_required(VERSION 3.10)

#set(CMAKE_TOOLCHAIN_FILE "third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(FMT_TEXT LANGUAGES CXX)

find_package(fmt CONFIG REQUIRED)

### Temporarily disable fmt_test when fmt_test.cpp is missing
# add_executable(fmt_test fmt_test.cpp)

# target_link_libraries(fmt_test PRIVATE fmt::fmt)