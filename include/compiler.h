#pragma once
#include<string>
#include<fstream>
#include<iostream>
#include<filesystem>
#include<stdlib.h>
#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "frontend/semantic.h"
#include "cplusplus/generator.h"
#ifdef _WIN32
#include <windows.h>
#else
#define ANSI_COLOR_RED   "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#endif
#define NS_VERSION 20260423L

namespace ns{
    // ------------------------------
    // 控制台颜色工具
    // ------------------------------
    static void reset_console_color() {
    #ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    #else
        std::cout << ANSI_COLOR_RESET;
    #endif
    }
    static void set_console_red() {
    #ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY);
    #else
        std::cout << ANSI_COLOR_RED;
    #endif
    }
    // ------------------------------
    // 错误输出
    // ------------------------------
    static void error(const std::string& msg) {
        set_console_red();
        std::cerr << "nsc: " << msg << std::endl;
        reset_console_color();
        std::exit(EXIT_FAILURE);
    }
    // 获取当前工作目录
    inline std::filesystem::path get_current_directory(){
        return std::filesystem::current_path();
    }
    // 获取当前相对路径对应的绝对路径
    inline std::filesystem::path get_absolute_path(const std::string & relative_path){
        return std::filesystem::absolute(relative_path).string();
    }
    inline std::filesystem::path get_compiler_path(){
        std::filesystem::path compiler_path = std::filesystem::current_path();
        return compiler_path.parent_path().string();
    }
    // ------------------------------
    // 读取源文件
    // ------------------------------
    static std::string read_file(const std::string& path) {
        std::ifstream f(path);
        if (!f) error("无法打开文件: " + path);
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
    // 直接通过 gcc 编译生成在内存中的 cpp 源码
    bool compile_with_gcc(const std::string& cpp_code, const std::string& output_exe)
    {
        // 构建 g++ 命令（从 stdin 读取 C++ 代码！）
        std::string cmd = "g++ -x c++ -o \"" + output_exe + "\" -";

        // 打开管道，写入模式 → 写给 g++ stdin
        FILE* fp = popen(cmd.c_str(), "w");
        if (!fp) {
            return false;
        }

        // 把内存里的 C++ 代码直接写给 g++
        fwrite(cpp_code.data(), 1, cpp_code.size(), fp);

        // 关闭管道 → g++ 开始编译
        int ret = pclose(fp);

        return ret == 0;
    }
}