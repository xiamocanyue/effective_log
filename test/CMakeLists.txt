cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(TEST LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(context_text
	test_context.cpp
	../context/executor.cpp
	../context/thread_pool.cpp
)

target_include_directories(context_text PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
)


