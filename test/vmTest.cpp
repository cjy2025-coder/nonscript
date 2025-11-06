#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "vm/vm.h"

int main(int argc, char* argv[])
{
    std::string path="samples/example.ss";
    
    // 处理命令行参数
    if (argc == 2) {
        // 用法: ss code.ss
        path = argv[1];
    } 
    // else if (argc == 3 && std::string(argv[1]) == "ss") {
    //     // 用法: ss code.ss
    //     path = argv[2];
    // } 
    // else {
    //     // 显示用法信息
    //     std::cerr << "用法: " << argv[0] << " <filename.ss>" << std::endl;
    //     std::cerr << "或者: ss <filename.ss>" << std::endl;
    //     std::cerr << "示例:" << std::endl;
    //     std::cerr << "  " << argv[0] << " samples/example.ss" << std::endl;
    //     std::cerr << "  ss samples/example.ss" << std::endl;
    //     return 1;
    // }
    
    // 确保文件以.ss结尾（可选）
    if (path.size() < 3 || path.substr(path.size() - 3) != ".ss") {
        std::cerr << "\033[33m" << "警告: 文件 '" << path << "' 可能不是SpringScript文件(.ss)" << "\033[0m" << std::endl;
    }
    
    std::ifstream is(path);
    if (!is.is_open())
    {
        std::cerr << "\033[31m" << "错误: 无法打开文件 '" << path << "'，请确认文件存在且路径正确!" << "\033[0m" << std::endl;
        return -1;
    }
    
    std::stringstream ss;
    ss << is.rdbuf();
    is.close();
    std::string source = ss.str();
    
    ns::VM vm;
    auto errors = vm.eval(source);
    if(errors == nullptr) return -1;
    if (typeid(*(errors.get())) == typeid(ns::Error))
    {
        std::cerr << "\033[31m" << errors->toBuf() << "\033[0m" << std::endl;
        return -1;
    }
    
    return 0;
}