#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "frontend/parser.h"
#include "frontend/lexer.h"
#include "frontend/semantic.h"
#include "ir/ir_generator.h"
#ifdef _WIN32
#include <windows.h>
#else
// 类 Unix 系统使用 ANSI 转义序列
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#endif

// 重置控制台颜色的辅助函数
void resetConsoleColor()
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << ANSI_COLOR_RESET;
#endif
}

// 设置控制台文本为红色的辅助函数
void setConsoleRed()
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
    std::cout << ANSI_COLOR_RED;
#endif
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::ifstream stream("././samples/example.ss");
    if (!stream)
    {
        // 错误信息标红
        setConsoleRed();
        std::cout << "无法找到此文件！" << std::endl;
        resetConsoleColor();
        exit(-1);
    }
    std::stringstream ss;
    ss << stream.rdbuf();
    std::string source = ss.str();
    ns::Lexer lexer(source, "example.ss");
    ns::Token token;
    stream.close();
    std::vector<ns::Token> tokens;
    while ((token = lexer.scan()).getType() != ns::TokenType::END)
    {
        tokens.push_back(token);
    }
    tokens.push_back(token);
    ns::Parser parser(tokens);
    parser.setLexer(&lexer);
    std::unique_ptr<ns::Program> program = parser.parse();
    if (!program)
    {
        auto e = parser.what();
        // 错误信息标红
        setConsoleRed();
        std::cout << e->whats();
        resetConsoleColor();
        return -1;
    }
    // ns::SemanticAnalyzer *sa = new ns::SemanticAnalyzer(&lexer);
    // auto status = sa->check(program.get());
    // if (!status)
    // {
    //     auto e = sa->what();
    //     // 错误信息标红
    //     setConsoleRed();
    //     std::cout << e->whats();
    //     resetConsoleColor();
    //     return -1;
    // }

    ns::TACGenerator ig;
    ig.gen(program.release());
    ig.save("example.tac");
    return 0;
}

