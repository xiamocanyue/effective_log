#include "mmap.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main() {
    fs::path p = fs::current_path();
    p /= "test.md";
    MMapAux map(p);
    map.Push("abc", 3);

    uint8_t* cur = map.Data();
    std::cout << "····· 文件结果为 ： " << reinterpret_cast<char*>(cur) << std::endl;

    std::cout << "当前映射的大小 为 ： " << map.Size() << std::endl;
    map.Push("def", 3);

    std::string s = "hello world, mmmmmmap";
    map.Push(s.c_str(), s.size());
     std::cout << "当前映射的大小 为 ： " << map.Size() << std::endl;

    std::cout << "····· 文件结果为 ： " << reinterpret_cast<char*>(cur) << std::endl;
}