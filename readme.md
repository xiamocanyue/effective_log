# Effective Log

## 项目介绍

`Effective Log` 是一个基于 `C++17` 的日志组件，目标是在日志写入前完成结构化、压缩、加密，并通过异步方式落盘。

项目当前主要包含以下能力：

- 使用 `protobuf` 对日志进行结构化序列化
- 使用 `zstd` 和 `zlib` 提供压缩能力
- 使用 `Crypto++` 提供 ECDH 密钥协商和 AES 加密
- 使用 `EffectiveSink` 作为高效日志输出目标
- 使用任务线程和消息队列将日志写入与业务线程解耦

项目主要目录说明：

- `compress/`：压缩实现
- `crypt/`：加密实现
- `formatter/`：日志格式化器
- `sinks/`：日志输出目标，核心是 `EffectiveSink`
- `context/`：任务调度和线程池
- `msg_protobuf/`：`protobuf` 消息定义及生成代码
- `read_log/`：日志解码辅助代码
- `test/`：独立测试工程

## 环境配置

### 基础环境

需要以下基础工具：

- `CMake >= 3.16`
- 支持 `C++17` 的编译器，例如 `g++` 或 `clang++`
- `make` 或 `ninja`

Ubuntu/Debian 下可使用：

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config
```

### 依赖库

项目依赖定义在 `vcpkg.json:1`，当前包含：

- `fmt`
- `zlib`
- `zstd`
- `protobuf`
- `cryptopp`


### 使用 vcpkg 安装依赖

如果你使用 `vcpkg`，可以安装依赖：

```bash
./vcpkg install fmt zlib zstd protobuf cryptopp 
```

如果仓库内已经准备好 `third_party/vcpkg`，则可以直接通过 `CMake` 配置工程，项目会自动使用仓库里的 `vcpkg` toolchain。

## 项目编译

### 编译核心库

根目录 `CMakeLists.txt:1` 当前负责构建核心静态库 `effective_log_core`。

配置并编译：

```bash
cmake -S . -B build
cmake --build build
```


## 项目使用

### 基本使用流程

项目对外推荐使用 `logger.h:7` 中提供的宏接口。

对外宏主要包括：

- `EXT_LOGGER_INIT(handle)`
- `LOG_LOGGER_TRACE(handle, ...)`
- `LOG_LOGGER_DEBUG(handle, ...)`
- `LOG_LOGGER_INFO(handle, ...)`
- `LOG_LOGGER_WARN(handle, ...)`
- `LOG_LOGGER_ERROR(handle, ...)`
- `LOG_LOGGER_CRITICAL(handle, ...)`
- `EXT_LOG_INFO(...)`
- `EXT_LOG_ERROR(...)`

其中：

- `LOG_LOGGER_ERROR(handle, ...)` 这类宏用于显式指定 `handle`
- `EXT_LOG_ERROR(...)` 这类宏使用 `LogFactory` 中注册的全局 `handle`
- `internal_log.h:1` 中的 `LOG_INFO` / `LOG_ERROR` 仅用于库内部调试，不是对外业务日志接口

### 典型使用方式

1. 先准备 `Logger::EffectiveSink::Conf`
2. 创建 `Logger::EffectiveSink`
3. 创建 `Logger::VariadicLogHandle` 或其他日志句柄
4. 通过 `EXT_LOGGER_INIT(handle)` 注册全局句柄
5. 使用 `LOG_LOGGER_ERROR(handle, ...)` 或 `EXT_LOG_ERROR(...)` 输出日志

示例：

```cpp
#include "logger.h"
#include "sinks/effective_sink.h"
#include "crypt/crypt.h"
#include "log_variadic_handle.h"

int main() {
    auto [peer_private_key, peer_public_key_bin] = Logger::Crypt::GenECDHKey();
    std::string peer_public_key_hex = Logger::Crypt::BinaryToHex(peer_public_key_bin);

    Logger::EffectiveSink::Conf conf;
    conf.other_public_key = peer_public_key_hex;
    conf.dir = "logs";
    conf.file_name = "effective";
    conf.passed_time_interval = std::chrono::minutes(1);

    auto sink = std::make_shared<Logger::EffectiveSink>(conf);
    auto handle = std::make_shared<Logger::VariadicLogHandle>(sink);

    EXT_LOGGER_INIT(handle);

    LOG_LOGGER_ERROR(handle, "log with explicit handle: {}", 100);
    EXT_LOG_ERROR("log with global handle: {}", 200);

    sink->Flush();
    return 0;
}
```

### 日志写入链路

日志从外部接口到落盘的大致流程如下：

1. 通过 `logger.h:7` 中的宏发起日志调用
2. `LogHandle` 把日志分发到 `sink`
3. `EffectiveSink::Log()` 在 `sinks/effective_sink.cpp:56` 将日志放入队列
4. 后台工作线程在 `sinks/effective_sink.cpp:129` 对日志执行格式化、压缩、加密和缓存写入
5. 最终由 `EffectiveSink` 完成文件落盘

### 使用提示

- 如果程序结束前需要确保日志落盘，主动调用 `sink->Flush()`
- 如果需要自定义日志输出位置、文件名前缀或定时整理间隔，修改 `sinks/effective_sink.h:39` 中的 `Logger::EffectiveSink::Conf`
- 如果只是做功能验证或性能对比，直接使用 `test/` 下的独立测试工程即可
