#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "ir/ir_generator.h"
#include "frontend/lexer.h"
#include "frontend/parser.h"
#include "backend/x86.h"
#ifdef _WIN32
#include <windows.h>
#endif
int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::ifstream stream("././samples/example.ss");
    if (!stream)
    {
        std::cout << "无法找到此文件！" << std::endl;
        exit(-1);
    }
    std::stringstream ss;
    ss << stream.rdbuf();
    std::string source = ss.str();
    ns::Lexer lexer(source, "example.ss");
    ns::Token token;

    std::vector<ns::Token> tokens;
    while ((token = lexer.scan()).getType() != ns::TokenType::END)
    {
        tokens.push_back(token);
    }
    tokens.push_back(token);
    ns::Parser parser(tokens);
    std::unique_ptr<ns::Program> program = parser.parse();
    ns::IrGenerator ig;
    auto irs=ig.generate(*(program.release()));
    ns::x86Generator x86;
    auto asm_code=x86.generate(irs);
    std::ofstream of;
    of.open("example.s",std::ios::out);
    of<<asm_code;
    of.close();
    stream.close();
    return 0;
}