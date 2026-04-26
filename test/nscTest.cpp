/*
 * NonScript Compiler (nsc)
*/
#include"compiler.h"
// ------------------------------
// 主函数：GCC/G++ 风格 CLI
// ------------------------------
int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string input_file;
    std::string output_file = "";
    std::vector<std::string> link_libs;         // -l xxx.dll
    bool is_save_temps=false;
    for (size_t i = 0; i < args.size(); ++i) {
        const auto& arg = args[i];

        if (arg == "-o") {
            if (i + 1 >= args.size()) ns::error("-o 必须指定输出文件");
            output_file = args[++i];
        }
        else if (arg == "-l") {
            if (i + 1 >= args.size()) ns::error("-l 必须指定库名");
            link_libs.push_back(args[++i]);
        }
        else if (arg == "-v" || arg == "--version") {
            std::cout << NS_VERSION << std::endl;
            return EXIT_SUCCESS;
        }
        else if (arg == "-h" || arg == "--help") {
            std::cout << "NonScript Compiler (nsc)\n";
            std::cout << "Usage: nsc [options] file.ns\n";
            std::cout << "Options:\n";
            std::cout << "  -o file   指定输出文件\n";
            std::cout << "  -l lib    链接库\n";
            std::cout << "  -v        版本\n";
            std::cout << "  -h        帮助\n";
            return EXIT_SUCCESS;
        }
        else if(arg == "-save-temps"){
           is_save_temps = true;
        }
        else {
            if (!input_file.empty()) ns::error("只能指定一个源文件");
            input_file = arg;
        }
    }

    if (input_file.empty()) ns::error("请指定 .ns 源文件");
    if (output_file.empty()) output_file = input_file;
    input_file = ns::get_absolute_path(input_file).string();
    output_file = ns::get_absolute_path(output_file).stem().string();
    // ------------------------------
    // 编译流程
    // ------------------------------
    std::string source = ns::read_file(input_file);

    // 词法分析
    ns::Lexer lexer(source, input_file);
    std::vector<ns::Token> tokens;
    ns::Token token;
    while ((token = lexer.scan()).getType() != ns::TokenType::END) {
        tokens.push_back(token);
    }
    tokens.push_back(token);

    // 语法分析
    ns::Parser parser(tokens);
    parser.setLexer(&lexer);
    auto program = parser.parse();
    if (!program) ns::error(parser.what()->whats());

    // 语义分析
    ns::SemanticAnalyzer sa(&lexer);
    sa.setTypeManager(parser.getTypeManager());
    if (!sa.check(program.get())) ns::error(sa.what()->whats());

    // 代码生成
    ns::CplusplusCodeGen codegen;
    codegen.run(program.get());
    std::string cpp_code = codegen.get();
    if(is_save_temps){
        codegen.save("./temp.cpp");
    }
    // 直接编译成 exe！
    bool ok = ns::compile_with_gcc(cpp_code, output_file);
    if (!ok) {
       ns::error("g++ 编译失败");
       return 1;
    }
    // codegen.save(output_file);  // 输出到指定文件

    return EXIT_SUCCESS;
}