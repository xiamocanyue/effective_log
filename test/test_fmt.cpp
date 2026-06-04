#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <fmt/core.h>
#include <fmt/color.h>
#include <vector>
#include <chrono>

int main() {
    fmt::print(fg(fmt::color::red), "hello world?\n");
    auto s = fmt::format("hello : {}", "test555\n");
    fmt::print(fg(fmt::color::pink), "{}", s);

    auto now = std::chrono::system_clock::now();
    fmt::print(fg(fmt::color::blue), "Time : {:%Y-%m-%d %H:%M:%S} \n", now);
    std::vector<int> v = {1, 2, 3};
    fmt::print("{}\n", fmt::join(v, ", "));
    fmt::print("{}\n", v);
}

