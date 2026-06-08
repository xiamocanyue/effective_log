cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(TEST LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(fmt CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(basic_stream_text
	test_basic_stream.cpp
	../log_handle.cpp
	../log_msg.cpp
	../formatter/default_formatter.cpp
	../sinks/console_sink.cpp
)

target_include_directories(basic_stream_text PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(basic_stream_text PRIVATE fmt::fmt Threads::Threads)


