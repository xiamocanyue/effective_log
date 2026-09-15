cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(EffectiveLog LANGUAGES CXX)

option(EFFECTIVE_LOG_ENABLE_ASAN "Enable address sanitizer" OFF)
option(EFFECTIVE_LOG_BUILD_TESTS "Build and register tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

set(EFFECTIVE_LOG_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(EFFECTIVE_LOG_CORE_SOURCES
    ${EFFECTIVE_LOG_ROOT}/compress/zlib_compress.cpp
    ${EFFECTIVE_LOG_ROOT}/compress/zstd_compress.cpp
    ${EFFECTIVE_LOG_ROOT}/context/executor.cpp
    ${EFFECTIVE_LOG_ROOT}/context/thread_pool.cpp
    ${EFFECTIVE_LOG_ROOT}/crypt/aes_crypt.cpp
    ${EFFECTIVE_LOG_ROOT}/crypt/crypt.cpp
    ${EFFECTIVE_LOG_ROOT}/formatter/default_formatter.cpp
    ${EFFECTIVE_LOG_ROOT}/formatter/effective_formatter.cpp
    ${EFFECTIVE_LOG_ROOT}/log_handle.cpp
    ${EFFECTIVE_LOG_ROOT}/log_msg.cpp
    ${EFFECTIVE_LOG_ROOT}/mmap.cpp
    ${EFFECTIVE_LOG_ROOT}/msg_protobuf/effective_msg.pb.cc
    ${EFFECTIVE_LOG_ROOT}/sinks/console_sink.cpp
    ${EFFECTIVE_LOG_ROOT}/sinks/effective_sink.cpp
)

add_library(effective_log_core STATIC ${EFFECTIVE_LOG_CORE_SOURCES})
add_library(effective_log::core ALIAS effective_log_core)

target_include_directories(effective_log_core
    PUBLIC
        ${EFFECTIVE_LOG_ROOT}
)

target_link_libraries(effective_log_core
    PUBLIC
        fmt::fmt
        zstd::libzstd_static
        cryptopp::cryptopp
        protobuf::libprotobuf
        ZLIB::ZLIB
        Threads::Threads

)

target_compile_features(effective_log_core PUBLIC cxx_std_17)

if(EFFECTIVE_LOG_ENABLE_ASAN)
    target_compile_options(effective_log_core PUBLIC -fsanitize=address)
    target_link_options(effective_log_core PUBLIC -fsanitize=address)
endif()

if(EFFECTIVE_LOG_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
