#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "frontend/parser.h"
#include "frontend/lexer.h"
#include "frontend/semantic.h"
#include "ir/ir_generator.h"
#include "optimization/optimizer.h"
#include "backend/x86.h"
#ifdef _WIN32
#include <windows.h>
#else
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"
#endif

void resetConsoleColor()
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << ANSI_COLOR_RESET;
#endif
}

void setConsoleRed()
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
    std::cout << ANSI_COLOR_RED;
#endif
}

static const char *op_to_str(ns::Op op)
{
    switch (op)
    {
    case ns::Op::LDC:   return "LDC";
    case ns::Op::MOV:   return "MOV";
    case ns::Op::ADD:   return "ADD";
    case ns::Op::SUB:   return "SUB";
    case ns::Op::MUL:   return "MUL";
    case ns::Op::DIV:   return "DIV";
    case ns::Op::MOD:   return "MOD";
    case ns::Op::NEG:   return "NEG";
    case ns::Op::NOT:   return "NOT";
    case ns::Op::AND:   return "AND";
    case ns::Op::OR:    return "OR";
    case ns::Op::CMP_EQ: return "CMP_EQ";
    case ns::Op::CMP_NE: return "CMP_NE";
    case ns::Op::CMP_LT: return "CMP_LT";
    case ns::Op::CMP_GT: return "CMP_GT";
    case ns::Op::CMP_LE: return "CMP_LE";
    case ns::Op::CMP_GE: return "CMP_GE";
    case ns::Op::JMP:   return "JMP";
    case ns::Op::JE:    return "JE";
    case ns::Op::JNE:   return "JNE";
    case ns::Op::LABEL: return "LABEL";
    case ns::Op::CALL:  return "CALL";
    case ns::Op::RET:   return "RET";
    case ns::Op::PARAM: return "PARAM";
    case ns::Op::ALLOC: return "ALLOC";
    case ns::Op::INDEX: return "INDEX";
    case ns::Op::PRINT: return "PRINT";
    case ns::Op::SCAN:  return "SCAN";
    case ns::Op::NOP:   return "NOP";
    }
    return "???";
}

void dump_ir(const ns::ProgramIR &pir)
{
    for (auto &func : pir.functions)
    {
        std::cout << "\n===== Function: " << func.name << " =====\n";
        for (auto &tac : func.instructions)
        {
            if (tac.op == ns::Op::LABEL)
            {
                std::cout << tac.label << ":\n";
                continue;
            }
            std::cout << "  " << op_to_str(tac.op);
            if (!tac.result.empty()) std::cout << " " << tac.result;
            if (!tac.arg1.empty())   std::cout << ", " << tac.arg1;
            if (!tac.arg2.empty())   std::cout << ", " << tac.arg2;
            std::cout << "\n";
        }
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::ifstream stream("././test.ns");
    if (!stream)
    {
        setConsoleRed();
        std::cout << "无法找到此文件！" << std::endl;
        resetConsoleColor();
        exit(-1);
    }
    std::stringstream ss;
    ss << stream.rdbuf();
    std::string source = ss.str();
    ns::Lexer lexer(source, "./test.ns");
    ns::typeManager::init();
    ns::Parser parser(&lexer);
    std::unique_ptr<ns::Program> program = parser.parse();
    if (program)
    {
        ns::SemanticAnalyzer *sa = new ns::SemanticAnalyzer(&lexer);
        auto status = sa->check(program.get());
        if (!status)
        {
            auto e = sa->what();
            setConsoleRed();
            std::cout << e->whats();
            resetConsoleColor();
            return -1;
        }

        // 1. 生成中间代码 (IR)
        ns::IrGenerator ir_gen;
        auto pir = ir_gen.generate(program.get());

        std::cout << "=== 优化前 IR ===";
        dump_ir(pir);

        // 2. 优化
        ns::Optimizer opt;
        opt.optimize(pir);

        std::cout << "\n=== 优化后 IR ===";
        dump_ir(pir);

        // 3. 生成 x86 汇编
        ns::x86Generator x86_gen;
        std::string asm_code = x86_gen.generate(pir);

        std::cout << "\n=== 生成 x86 汇编 ===\n" << asm_code << std::endl;

        // 4. 保存为 .asm 文件
        std::ofstream asm_file("output.asm");
        asm_file << asm_code;
        asm_file.close();
        std::cout << "已保存到 output.asm" << std::endl;
    }
    else
    {
        auto e = parser.what();
        setConsoleRed();
        std::cout << e->whats();
        resetConsoleColor();
    }
    return 0;
}