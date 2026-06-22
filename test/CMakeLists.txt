cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    endif()

    project(EffectiveLogTests LANGUAGES CXX)

    option(EFFECTIVE_LOG_ENABLE_ASAN "Enable address sanitizer" ON)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    find_package(fmt CONFIG REQUIRED)
    find_package(zstd CONFIG REQUIRED)
    find_package(cryptopp CONFIG REQUIRED)
    find_package(protobuf CONFIG REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(Threads REQUIRED)

    set(EFFECTIVE_LOG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

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
endif()

set(EFFECTIVE_LOG_TEST_INCLUDE_DIRS
    ${EFFECTIVE_LOG_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(test_basic_effective_log
    test_basic_effective_log.cpp
)

target_include_directories(test_basic_effective_log PRIVATE ${EFFECTIVE_LOG_TEST_INCLUDE_DIRS})
target_link_libraries(test_basic_effective_log PRIVATE effective_log::core)

add_executable(test_order_compress_crypt
    test_order_compress_crypt.cpp
)

target_include_directories(test_order_compress_crypt PRIVATE ${EFFECTIVE_LOG_TEST_INCLUDE_DIRS})
target_link_libraries(test_order_compress_crypt PRIVATE effective_log::core)

add_executable(test_effective_sink_queue_benchmark
    test_effective_sink_queue_benchmark.cpp
)

target_include_directories(test_effective_sink_queue_benchmark PRIVATE ${EFFECTIVE_LOG_TEST_INCLUDE_DIRS})
target_link_libraries(test_effective_sink_queue_benchmark PRIVATE effective_log::core)
