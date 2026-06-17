cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(TEST LANGUAGES CXX)

find_package(fmt CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)


# ---- test_basic_effective_log ----
set(ROOT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(test_basic_effective_log
    test_basic_effective_log.cpp

    # sinks
    ${ROOT_SRC}/sinks/effective_sink.cpp

    # formatter
    ${ROOT_SRC}/formatter/effective_formatter.cpp
    ${ROOT_SRC}/formatter/default_formatter.cpp

    # crypt
    ${ROOT_SRC}/crypt/crypt.cpp
    ${ROOT_SRC}/crypt/aes_crypt.cpp

    # compress
    ${ROOT_SRC}/compress/zstd_compress.cpp

    # mmap
    ${ROOT_SRC}/mmap.cpp

    # context runtime
    ${ROOT_SRC}/context/executor.cpp
    ${ROOT_SRC}/context/thread_pool.cpp

    # log core
    ${ROOT_SRC}/log_msg.cpp
    ${ROOT_SRC}/log_handle.cpp

    # protobuf generated
    ${ROOT_SRC}/msg_protobuf/effective_msg.pb.cc
)

target_include_directories(test_basic_effective_log PRIVATE
    ${ROOT_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(test_basic_effective_log PRIVATE
    fmt::fmt
    zstd::libzstd_static
    cryptopp::cryptopp
    protobuf::libprotobuf
)
