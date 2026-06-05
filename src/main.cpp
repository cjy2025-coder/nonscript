#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "frontend/parser.h"
#include "frontend/lexer.h"
#include "frontend/semantic.h"
#include "ir/ir_generator.h"
#include "optimization/optimizer.h"
#include "backend/x86.h"
#include "compiler.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static void reset_color()
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    std::cout << "\x1b[0m";
#endif
}

static void set_red()
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
    std::cout << "\x1b[31m";
#endif
}

std::string  op_to_string(ns::Op op) {
    switch (op) {
        case ns::Op::LDC: return "LDC";
        case ns::Op::MOV: return "MOV";
        case ns::Op::ADD: return "ADD";
        case ns::Op::SUB: return "SUB";
        case ns::Op::MUL: return "MUL";
        case ns::Op::DIV: return "DIV";
        case ns::Op::MOD: return "MOD";
        case ns::Op::NEG: return "NEG";
        case ns::Op::NOT: return "NOT";
        case ns::Op::AND: return "AND";
        case ns::Op::OR: return "OR";
        case ns::Op::CMP_EQ: return "CMP_EQ";
        case ns::Op::CMP_NE: return "CMP_NE";
        case ns::Op::CMP_LT: return "CMP_LT";
        case ns::Op::CMP_GT: return "CMP_GT";
        case ns::Op::CMP_LE: return "CMP_LE";
        case ns::Op::CMP_GE: return "CMP_GE";
        case ns::Op::JMP: return "JMP";
        case ns::Op::JE: return "JE";
        case ns::Op::JNE: return "JNE";
        case ns::Op::LABEL: return "LABEL";
        case ns::Op::CALL: return "CALL";
        case ns::Op::RET: return "RET";
        case ns::Op::PARAM: return "PARAM";
        case ns::Op::ALLOC: return "ALLOC";
        case ns::Op::INDEX: return "INDEX";
        case ns::Op::STORE_INDEX: return "STORE_INDEX";
        case ns::Op::PRINT: return "PRINT";
        case ns::Op::SCAN: return "SCAN";
        case ns::Op::NOP: return "NOP";
        default: return "UNKNOWN";
    }
}

static void error(const std::string &msg)
{
    set_red();
    std::cerr << "nsc: " << msg << std::endl;
    reset_color();
    exit(EXIT_FAILURE);
}

static std::string read_source(const std::string &path)
{
    std::ifstream f(path);
    if (!f) error("cannot open file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// 获取文件不含扩展名的基名
static std::string stem(const std::string &path)
{
    size_t sep = path.find_last_of("/\\");
    std::string name = (sep == std::string::npos) ? path : path.substr(sep + 1);
    size_t dot = name.find_last_of('.');
    return (dot == std::string::npos) ? name : name.substr(0, dot);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::vector<std::string> args(argv + 1, argv + argc);
    std::string input_file;
    std::string output_file;
    bool save_temps = false;
    args = {"test.ns","-save-temps"};
    for (size_t i = 0; i < args.size(); ++i)
    {
        const auto &arg = args[i];

        if (arg == "-o")
        {
            if (i + 1 >= args.size()) error("-o requires an output file");
            output_file = args[++i];
        }
        else if (arg == "-v" || arg == "--version")
        {
            std::cout << NS_VERSION << std::endl;
            return EXIT_SUCCESS;
        }
        else if (arg == "-h" || arg == "--help")
        {
            std::cout << "NonScript Compiler (nsc)\n";
            std::cout << "Usage: nsc [options] file.ns\n";
            std::cout << "Options:\n";
            std::cout << "  -o file      指定输出文件\n";
            std::cout << "  -save-temps  保留中间文件(.ir, .asm)\n";
            std::cout << "  -v           版本\n";
            std::cout << "  -h           帮助\n";
            return EXIT_SUCCESS;
        }
        else if (arg == "-save-temps")
        {
            save_temps = true;
        }
        else
        {
            if (!input_file.empty()) error("only one source file allowed");
            input_file = arg;
        }
    }

    if (input_file.empty()) error("please specify a .ns source file");

    // 确定输出文件名
    std::string base_name = stem(input_file);
    if (output_file.empty())
        output_file = base_name + ".exe";

    // 读取源文件
    std::string source = read_source(input_file);

    // ---- 编译流水线 ----

    // 1. 词法 + 语法分析
    ns::Lexer lexer(source, input_file);
    ns::typeManager::init();
    ns::Parser parser(&lexer);
    auto program = parser.parse();
    if (!program) error(parser.what()->whats());

    // 2. 语义分析
    ns::SemanticAnalyzer sa(&lexer);
    if (!sa.check(program.get())) error(sa.what()->whats());

    // 3. IR 生成
    ns::IrGenerator ir_gen;
    auto pir = ir_gen.generate(program.get());

    // 4. 优化
    ns::Optimizer opt;
    opt.optimize(pir);

    // 5. 保存 IR（如需要）
    if (save_temps)
    {
        std::string ir_path = base_name + ".ir";
        std::ofstream ir_file(ir_path);
        for (auto &func : pir.functions)
        {
            ir_file << "===== Function: " << func.name << " =====\n";
            for (auto &tac : func.instructions)
            {
                if (tac.op == ns::Op::LABEL)
                {
                    ir_file << tac.label << ":\n";
                    continue;
                }
                ir_file << "  " << op_to_string(tac.op);
                if (!tac.result.empty()) ir_file << " " << tac.result;
                if (!tac.arg1.empty())   ir_file << ", " << tac.arg1;
                if (!tac.arg2.empty())   ir_file << ", " << tac.arg2;
                ir_file << "\n";
            }
        }
        ir_file.close();
        std::cout << "[save-temps] saved " << ir_path << std::endl;
    }

    // 6. x86 汇编生成
    ns::x86Generator x86_gen;
    std::string asm_code = x86_gen.generate(pir);

    // 7. 保存汇编文件
    std::string asm_path = base_name + ".asm";
    {
        std::ofstream asm_file(asm_path);
        asm_file << asm_code;
        asm_file.close();
    }
    if (!save_temps)
    {
        // 不保留则删除（但是我们需要用它编译，等编译完再删）
    }

    // 8. 编译汇编 → 目标文件 → 可执行文件
    std::string obj_path = base_name + ".obj";
    std::string cmd_nasm = "nasm -f win64 \"" + asm_path + "\" -o \"" + obj_path + "\"";
    int nasm_ret = system(cmd_nasm.c_str());
    if (nasm_ret != 0) error("nasm assembly failed");

    std::string cmd_link = "gcc -o \"" + output_file + "\" \"" + obj_path + "\" -lmsvcrt";
    int link_ret = system(cmd_link.c_str());
    if (link_ret != 0) error("link failed");

    // 9. 清理临时文件
    if (!save_temps)
    {
        remove(asm_path.c_str());
        remove(obj_path.c_str());
    }
    else
    {
        std::cout << "[save-temps] saved " << asm_path << std::endl;
        std::cout << "[save-temps] saved " << obj_path << std::endl;
    }

    std::cout << "compiled to " << output_file << std::endl;
    return EXIT_SUCCESS;
}