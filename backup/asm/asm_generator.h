#pragma once
namespace ns{
// asm_generator.h
#pragma once
#include "./frontend/semantic.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>

namespace ns
{
    // x86-64 汇编代码生成器（AT&T 语法）
    class AsmCodeGen
    {
    private:
        std::string code;           // 生成的汇编代码
        std::string data_section;   // 数据段
        std::string text_section;   // 代码段
        int label_counter;          // 标签计数器
        int stack_offset;           // 栈偏移量
        
        // 符号表：变量名 -> (偏移量, 类型大小)
        std::unordered_map<std::string, std::pair<int, int>> symbol_table;
        
        // 类型大小映射
        std::unordered_map<std::string, int> type_sizes = {
            {"i8", 1}, {"i16", 2}, {"i32", 4}, {"i64", 8},
            {"f32", 4}, {"f64", 8}, {"bool", 1}, {"string", 8}
        };

    public:
        AsmCodeGen() : label_counter(0), stack_offset(0) {}
        
        ~AsmCodeGen() = default;
        
        std::string get() const
        {
            return ".intel_syntax noprefix\n" + data_section + text_section;
        }
        
        void save(std::string path) const
        {
            std::ofstream of(path);
            of << get();
            of.close();
        }
        
        void run(Program* program)
        {
            init_sections();
            for (auto& stmt : program->stmts)
            {
                genStatement(stmt);
            }
        }

    private:
        void init_sections()
        {
            data_section = ".data\n";
            text_section = ".text\n";
            text_section += ".global main\n";
            text_section += "main:\n";
            text_section += "    push rbp\n";
            text_section += "    mov rbp, rsp\n";
            text_section += "    sub rsp, 48\n";  // 预留栈空间
        }
        
        std::string new_label()
        {
            return ".L" + std::to_string(label_counter++);
        }
        
        void genStatement(Statement* stmt)
        {
            if (!stmt) return;
            if (auto* s = dynamic_cast<DeclareStatement*>(stmt))
                genDeclareStatement(s);
            else if (auto* s = dynamic_cast<ExpressionStatement*>(stmt))
                genExpressionStatement(s);
            else if (auto* s = dynamic_cast<BlockStatement*>(stmt))
                genBlockStatement(s);
            else if (auto* s = dynamic_cast<ReturnStatement*>(stmt))
                genRetStatement(s);
            // 可以根据需要添加更多语句类型
        }
        
        void genDeclareStatement(DeclareStatement* stmt)
        {
            if (!stmt) return;
            
            for (const auto& var : stmt->getVars())
            {
                const auto& ident = var.first;
                const auto& expr = var.second;
                std::string var_name = ident->getLiteral();
                auto type = ident->getType();
                
                // 分配栈空间
                int size = getTypeSize(type);
                stack_offset -= size;
                symbol_table[var_name] = {stack_offset, size};
                
                if (expr)
                {
                    // 生成初始化代码
                    std::string init_asm = genExpression(expr);
                    text_section += "    mov dword ptr [rbp + " + 
                                    std::to_string(stack_offset) + "], eax\n";
                }
            }
        }
        
        void genExpressionStatement(ExpressionStatement* stmt)
        {
            if (!stmt) return;
            genExpression(stmt->expression());
        }
        
        void genBlockStatement(BlockStatement* stmt)
        {
            if (!stmt) return;
            for (auto& s : stmt->value())
            {
                genStatement(s);
            }
        }
        
        void genRetStatement(ReturnStatement* stmt)
        {
            if (!stmt) return;
            std::string ret_asm = genExpression(stmt->getExpression());
            text_section += "    mov rsp, rbp\n";
            text_section += "    pop rbp\n";
            text_section += "    ret\n";
        }
        
        std::string genExpression(Expression* expr)
        {
            if (!expr) return "";
            
            if (auto* e = dynamic_cast<I32Literal*>(expr))
            {
                text_section += "    mov eax, " + e->getLiteral() + "\n";
                return "eax";
            }
            
            if (auto* e = dynamic_cast<StringLiteral*>(expr))
            {
                std::string str_label = new_label();
                data_section += str_label + ": .string \"" + 
                                escapeString(e->getLiteral()) + "\"\n";
                text_section += "    lea rax, " + str_label + "[rip]\n";
                return "rax";
            }
            
            if (auto* e = dynamic_cast<Ident*>(expr))
            {
                std::string var_name = e->getLiteral();
                if (symbol_table.find(var_name) != symbol_table.end())
                {
                    int offset = symbol_table[var_name].first;
                    text_section += "    mov eax, dword ptr [rbp + " + 
                                    std::to_string(offset) + "]\n";
                    return "eax";
                }
                return "";
            }
            
            if (auto* e = dynamic_cast<InfixExpression*>(expr))
            {
                std::string op = e->getOperator();
                genExpression(e->getLeft());
                text_section += "    push rax\n";
                genExpression(e->getRight());
                text_section += "    pop rbx\n";
                
                if (op == "+")
                    text_section += "    add eax, ebx\n";
                else if (op == "-")
                    text_section += "    sub eax, ebx\n";
                else if (op == "*")
                    text_section += "    imul eax, ebx\n";
                else if (op == "/")
                    text_section += "    mov edx, 0\n    div ebx\n";
                else if (op == "==")
                {
                    text_section += "    cmp ebx, eax\n";
                    text_section += "    sete al\n";
                    text_section += "    movzx eax, al\n";
                }
                else if (op == "!=")
                {
                    text_section += "    cmp ebx, eax\n";
                    text_section += "    setne al\n";
                    text_section += "    movzx eax, al\n";
                }
                else if (op == "<")
                {
                    text_section += "    cmp ebx, eax\n";
                    text_section += "    setl al\n";
                    text_section += "    movzx eax, al\n";
                }
                else if (op == ">")
                {
                    text_section += "    cmp ebx, eax\n";
                    text_section += "    setg al\n";
                    text_section += "    movzx eax, al\n";
                }
                return "eax";
            }
            
            if (auto* e = dynamic_cast<CallExpression*>(expr))
            {
                return genCallExpression(e);
            }
            
            return "";
        }
        
        std::string genCallExpression(CallExpression* expr)
        {
            auto* ident = dynamic_cast<Ident*>(expr->getFunc());
            if (!ident) return "";
            
            std::string func_name = ident->getLiteral();
            auto& args = expr->getArgs();
            
            // 处理 print 函数
            if (func_name == "print")
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    genExpression(args[i].get());
                    text_section += "    mov rsi, rax\n";
                    text_section += "    lea rdi, [rip + .LC0]\n";
                    text_section += "    xor eax, eax\n";
                    text_section += "    call printf@plt\n";
                }
                data_section += ".LC0: .string \"%s\"\n";
                return "";
            }
            
            // 处理普通函数调用
            for (int i = args.size() - 1; i >= 0; --i)
            {
                genExpression(args[i].get());
                text_section += "    push rax\n";
            }
            
            text_section += "    call " + func_name + "\n";
            
            // 清理参数（假设 x86-64 System V ABI）
            if (args.size() > 0)
                text_section += "    add rsp, " + std::to_string(args.size() * 8) + "\n";
            
            return "eax";
        }
        
        int getTypeSize(const std::string& type) const
        {
            auto it = type_sizes.find(type);
            if (it != type_sizes.end())
                return it->second;
            return 8;  // 指针大小
        }
        
        std::string escapeString(const std::string& input)
        {
            std::string output;
            for (char c : input)
            {
                switch (c)
                {
                case '\n': output += "\\n"; break;
                case '\t': output += "\\t"; break;
                case '"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                default: output += c; break;
                }
            }
            return output;
        }
    };
}
}; // end of namespace ns