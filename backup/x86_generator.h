#pragma once
#include "../frontend/semantic.h"
#include "../type.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <stack>
#include <fstream>

namespace ns
{
    class x86Generator
    {
    private:
        // 变量偏移量映射（变量名 -> 相对 rbp 的偏移，负数）
        std::unordered_map<std::string, int> var_offsets;
        int next_offset = -8; // 从 -8 开始向下增长

        // 字符串常量池
        struct StrConst
        {
            std::string label;
            std::string value;
        };
        std::vector<StrConst> string_constants;
        int string_counter = 0;

        // 浮点常量池
        struct FloatConst
        {
            std::string label;
            std::string value;
        };
        std::vector<FloatConst> float_constants;
        int float_counter = 0;

        // 标签计数器
        int label_counter = 0;

        // 生成的汇编代码
        std::string asm_code;
        // 函数外部代码（rodata 段）
        std::string data_section;
        // 函数体代码
        std::string text_section;
        // 函数声明（外部函数引用）
        std::string extern_section;

        // 字符串常量前缀
        std::string newLabel()
        {
            return ".L" + std::to_string(label_counter++);
        }

        // 获取或分配变量偏移
        int getOrAllocOffset(const std::string &varname)
        {
            auto it = var_offsets.find(varname);
            if (it != var_offsets.end())
            {
                return it->second;
            }
            int off = next_offset;
            var_offsets[varname] = off;
            next_offset -= 8;
            return off;
        }

        // 获取变量偏移（不分配）
        int getOffset(const std::string &varname) const
        {
            return var_offsets.at(varname);
        }

        // 对齐栈空间大小到16的倍数
        int alignStackSize(int raw_size) const
        {
            if (raw_size <= 0)
                return 0;
            int aligned = raw_size;
            if (aligned % 16 != 0)
                aligned += 16 - (aligned % 16);
            return aligned;
        }

        // 获取栈总大小
        int getStackSize() const
        {
            int min_offset = 0;
            for (const auto &[name, off] : var_offsets)
            {
                if (off < min_offset)
                    min_offset = off;
            }
            return alignStackSize(-min_offset);
        }

        // 生成访问栈上变量的汇编代码（加载到指定寄存器）
        std::string loadToReg(int offset, const std::string &reg = "rax", bool sign_extend = false)
        {
            return "mov " + reg + ", [rbp" + std::to_string(offset) + "]\n";
        }

        // 生成从寄存器存储到栈上的汇编代码
        std::string storeFromReg(int offset, const std::string &reg = "rax")
        {
            return "mov [rbp" + std::to_string(offset) + "], " + reg + "\n";
        }

        // 添加字符串常量到.rodata
        std::string addStringConstant(const std::string &str)
        {
            std::string label = "str" + std::to_string(string_counter++);

            // 转义字符串内容
            std::string escaped;
            for (char c : str)
            {
                switch (c)
                {
                case '\n':
                    escaped += "\\n";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
                    break;
                case '\0':
                    escaped += "\\0";
                    break;
                default:
                    escaped += c;
                    break;
                }
            }

            string_constants.push_back({label, str});
            data_section += label + " db \"" + escaped + "\", 0\n";
            return label;
        }

        // 添加浮点常量到.rodata
        std::string addFloatConstant(const std::string &val)
        {
            std::string label = "flt" + std::to_string(float_counter++);
            float_constants.push_back({label, val});
            // 用 dq (8字节) 存储双精度浮点
            data_section += label + " dq " + val + "\n";
            return label;
        }

    public:
        x86Generator()
        {
            // 初始化外部函数声明
            extern_section = "extern printf\n";
            extern_section += "extern scanf\n";
            extern_section += "extern putchar\n";
            extern_section += "extern malloc\n";
            extern_section += "extern free\n";
            extern_section += "extern exit\n";

            // 初始化数据段声明
            data_section += "section .rodata\n";
            data_section += "int_fmt db \"%lld\", 0\n";
            data_section += "str_fmt db \"%s\", 0\n";
            data_section += "str_endl db 10, 0\n";
            data_section += "scan_int_fmt db \"%lld\", 0\n";
        }

        std::string generate(Program *program)
        {
            text_section = "section .text\n";
            text_section += "global main\n\n";
            text_section += "main:\n";
            text_section += "push rbp\n";
            text_section += "mov rbp, rsp\n";

            // 遍历所有语句生成代码
            for (auto &stmt : program->stmts)
            {
                genStatement(stmt);
            }

            // 函数结束，恢复栈并返回0
            int stack_size = getStackSize();
            if (stack_size > 0)
            {
                text_section += "add rsp, " + std::to_string(stack_size) + "\n";
            }
            text_section += "mov rsp, rbp\n";
            text_section += "pop rbp\n";
            text_section += "xor rax, rax\n";
            text_section += "ret\n";

            // 组装
            asm_code = extern_section + "\n" + data_section + "\n" + text_section;
            return asm_code;
        }

        void save(const std::string &path) const
        {
            std::ofstream of(path);
            of << asm_code;
            of.close();
        }

        std::string get() const
        {
            return asm_code;
        }

    private:
        // ==================== 语句生成 ====================
        void genStatement(Statement *stmt)
        {
            if (!stmt)
                return;

            if (auto *s = dynamic_cast<DeclareStatement *>(stmt))
                genDeclareStatement(s);
            else if (auto *s = dynamic_cast<ExpressionStatement *>(stmt))
                genExpressionStatement(s);
            else if (auto *s = dynamic_cast<BlockStatement *>(stmt))
                genBlockStatement(s);
            else if (auto *s = dynamic_cast<FuncDecl *>(stmt))
                genFuncDecl(s);
            else if (auto *s = dynamic_cast<ClassLiteral *>(stmt))
                genClassStatement(s);
            else if (auto *s = dynamic_cast<ReturnStatement *>(stmt))
                genRetStatement(s);
            else if (auto *s = dynamic_cast<ShortStatement *>(stmt))
                genShortStatement(s);
            else if (auto *s = dynamic_cast<TryCatchStatement *>(stmt))
                genTryCatchStatement(s);
            // ImportStatement - 暂时忽略
        }

        void genShortStatement(ShortStatement *stmt)
        {
            // break / continue 暂时处理为跳转到函数结尾（需要更复杂的标签管理，暂简化）
            if (stmt->getLiteral() == "break" || stmt->getLiteral() == "continue")
            {
                // 占位
            }
        }

        void genRetStatement(ReturnStatement *stmt)
        {
            if (stmt->getExpression())
            {
                genExpressionToReg(stmt->getExpression(), "rax");
            }
            // 跳出函数
            int stack_size = getStackSize();
            if (stack_size > 0)
            {
                text_section += "add rsp, " + std::to_string(stack_size) + "\n";
            }
            text_section += "mov rsp, rbp\n";
            text_section += "pop rbp\n";
            text_section += "ret\n";
        }

        void genDeclareStatement(DeclareStatement *stmt)
        {
            for (const auto &var : stmt->getVars())
            {
                const auto &ident = var.first;
                const auto &expr = var.second;

                std::string varname = ident->getLiteral();
                int offset = getOrAllocOffset(varname);

                if (expr)
                {
                    // 需要将 expr 的值放入 rax，然后存入栈
                    genExpressionToReg(expr, "rax");
                    text_section += storeFromReg(offset, "rax");
                }
                else
                {
                    // 没有初始化表达式，清零
                    text_section += "xor rax, rax\n";
                    text_section += storeFromReg(offset, "rax");
                }
            }
        }

        void genExpressionStatement(ExpressionStatement *stmt)
        {
            if (!stmt || !stmt->expression())
                return;
            // 表达式的结果压入 rax（但作为语句忽略返回值）
            genExpressionToReg(stmt->expression(), "rax");
        }

        void genBlockStatement(BlockStatement *stmt)
        {
            if (!stmt)
                return;
            for (auto &s : stmt->value())
            {
                genStatement(s);
            }
        }

        void genBlockBody(BlockStatement *stmt)
        {
            genBlockStatement(stmt);
        }

        // ==================== 表达式生成 ====================
        // 将表达式的值计算到指定寄存器
        void genExpressionToReg(Expression *expr, const std::string &reg = "rax")
        {
            if (!expr)
                return;

            // 字面量
            if (auto *e = dynamic_cast<I8Literal *>(expr))
            {
                text_section += "mov " + reg + ", " + e->getLiteral() + "\n";
                return;
            }
            if (auto *e = dynamic_cast<I16Literal *>(expr))
            {
                text_section += "mov " + reg + ", " + e->getLiteral() + "\n";
                return;
            }
            if (auto *e = dynamic_cast<I32Literal *>(expr))
            {
                text_section += "mov " + reg + ", " + e->getLiteral() + "\n";
                return;
            }
            if (auto *e = dynamic_cast<I64Literal *>(expr))
            {
                text_section += "mov " + reg + ", " + e->getLiteral() + "\n";
                return;
            }
            if (auto *e = dynamic_cast<BoolLiteral *>(expr))
            {
                std::string val = (e->getLiteral() == "true") ? "1" : "0";
                text_section += "mov " + reg + ", " + val + "\n";
                return;
            }
            if (auto *e = dynamic_cast<NullLiteral *>(expr))
            {
                text_section += "xor " + reg + ", " + reg + "\n";
                return;
            }

            // 浮点字面量
            if (auto *e = dynamic_cast<Float32Literal *>(expr))
            {
                std::string label = addFloatConstant(std::to_string(e->getValue()));
                text_section += "movsd xmm0, [" + label + "]\n";
                text_section += "cvtsd2si " + reg + ", xmm0\n"; // float->int 暂存
                return;
            }
            if (auto *e = dynamic_cast<Float64Literal *>(expr))
            {
                std::string label = addFloatConstant(std::to_string(e->getValue()));
                text_section += "movsd xmm0, [" + label + "]\n";
                text_section += "cvtsd2si " + reg + ", xmm0\n";
                return;
            }

            // 字符串字面量
            if (auto *e = dynamic_cast<StringLiteral *>(expr))
            {
                std::string label = addStringConstant(e->getLiteral());
                text_section += "lea " + reg + ", [" + label + "]\n";
                return;
            }

            // 标识符（变量读取）
            if (auto *e = dynamic_cast<Ident *>(expr))
            {
                int offset = getOrAllocOffset(e->getLiteral());
                text_section += loadToReg(offset, reg);
                return;
            }

            // this 指针
            if (dynamic_cast<ThisExpr *>(expr))
            {
                if (reg != "rdi")
                    text_section += "mov " + reg + ", rdi\n";
                return;
            }

            // new 表达式
            if (auto *e = dynamic_cast<NewExpression *>(expr))
            {
                genNewExpression(e, reg);
                return;
            }

            // 前缀表达式
            if (auto *e = dynamic_cast<PrefixExpression *>(expr))
            {
                genPrefixExpression(e, reg);
                return;
            }

            // 中缀表达式
            if (auto *e = dynamic_cast<InfixExpression *>(expr))
            {
                genInfixExpression(e, reg);
                return;
            }

            // 函数调用
            if (auto *e = dynamic_cast<CallExpression *>(expr))
            {
                genCallExpression(e, reg);
                return;
            }

            // 索引表达式
            if (auto *e = dynamic_cast<IndexExpression *>(expr))
            {
                genIndexExpression(e, reg);
                return;
            }

            // if 表达式
            if (auto *e = dynamic_cast<IfExpression *>(expr))
            {
                genIfExpression(e, reg);
                return;
            }

            // while 表达式
            if (auto *e = dynamic_cast<WhileExpression *>(expr))
            {
                genWhileExpression(e, reg);
                return;
            }

            // until 表达式
            if (auto *e = dynamic_cast<UntilExpression *>(expr))
            {
                genUntilExpression(e, reg);
                return;
            }

            // switch 表达式
            if (auto *e = dynamic_cast<SwitchExpression *>(expr))
            {
                genSwitchExpression(e, reg);
                return;
            }

            // lambda
            if (auto *e = dynamic_cast<LambdaExpr *>(expr))
            {
                genLambdaExpression(e, reg);
                return;
            }

            // 数组字面量
            if (auto *e = dynamic_cast<ArrayLiteral *>(expr))
            {
                genArrayLiteral(e, reg);
                return;
            }
        }

        // ==================== 具体表达式实现 ====================

        void genPrefixExpression(PrefixExpression *expr, const std::string &reg)
        {
            std::string op = expr->getOperator();
            genExpressionToReg(expr->getRight(), reg);

            if (op == "-")
            {
                text_section += "neg " + reg + "\n";
            }
            else if (op == "!")
            {
                text_section += "cmp " + reg + ", 0\n";
                text_section += "sete al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == "~")
            {
                text_section += "not " + reg + "\n";
            }
        }

        void genInfixExpression(InfixExpression *expr, const std::string &reg)
        {
            std::string op = expr->getOperator();

            // 特殊处理：成员访问 (.) 
            if (op == "." || op == "->")
            {
                genMemberAccess(expr, reg);
                return;
            }

            // 对于逻辑或和逻辑与，需要短路求值
            if (op == "||")
            {
                genLogicalOr(expr, reg);
                return;
            }
            if (op == "&&")
            {
                genLogicalAnd(expr, reg);
                return;
            }

            // 一般二元运算：将左操作数放入 reg，右操作数放入 rbx
            genExpressionToReg(expr->getLeft(), reg);

            // 保存左操作数到栈临时位置
            text_section += "push " + reg + "\n";
            genExpressionToReg(expr->getRight(), "rbx");
            text_section += "pop " + reg + "\n";

            if (op == "+")
            {
                text_section += "add " + reg + ", rbx\n";
            }
            else if (op == "-")
            {
                text_section += "sub " + reg + ", rbx\n";
            }
            else if (op == "*")
            {
                text_section += "imul " + reg + ", rbx\n";
            }
            else if (op == "/")
            {
                text_section += "xor rdx, rdx\n";
                text_section += "idiv rbx\n";
                // 商在 rax
            }
            else if (op == "%")
            {
                text_section += "xor rdx, rdx\n";
                text_section += "idiv rbx\n";
                text_section += "mov " + reg + ", rdx\n";
            }
            else if (op == "==")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "sete al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == "!=")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "setne al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == "<")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "setl al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == ">")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "setg al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == "<=")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "setle al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == ">=")
            {
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "setge al\n";
                text_section += "movzx " + reg + ", al\n";
            }
            else if (op == "&")
            {
                text_section += "and " + reg + ", rbx\n";
            }
            else if (op == "|")
            {
                text_section += "or " + reg + ", rbx\n";
            }
            else if (op == "^")
            {
                text_section += "xor " + reg + ", rbx\n";
            }
            else if (op == "<<")
            {
                text_section += "mov rcx, rbx\n";
                text_section += "shl " + reg + ", cl\n";
            }
            else if (op == ">>")
            {
                text_section += "mov rcx, rbx\n";
                text_section += "sar " + reg + ", cl\n";
            }
        }

        void genLogicalOr(InfixExpression *expr, const std::string &reg)
        {
            std::string end_label = newLabel();
            std::string true_label = newLabel();

            genExpressionToReg(expr->getLeft(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "jne " + true_label + "\n";

            genExpressionToReg(expr->getRight(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "jne " + true_label + "\n";

            // 两者都为 false
            text_section += "xor " + reg + ", " + reg + "\n";
            text_section += "jmp " + end_label + "\n";

            text_section += true_label + ":\n";
            text_section += "mov " + reg + ", 1\n";

            text_section += end_label + ":\n";
        }

        void genLogicalAnd(InfixExpression *expr, const std::string &reg)
        {
            std::string end_label = newLabel();
            std::string false_label = newLabel();

            genExpressionToReg(expr->getLeft(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "je " + false_label + "\n";

            genExpressionToReg(expr->getRight(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "je " + false_label + "\n";

            // 两者都为 true
            text_section += "mov " + reg + ", 1\n";
            text_section += "jmp " + end_label + "\n";

            text_section += false_label + ":\n";
            text_section += "xor " + reg + ", " + reg + "\n";

            text_section += end_label + ":\n";
        }

        void genIfExpression(IfExpression *expr, const std::string &reg)
        {
            // if-expression 作为表达式使用时，需要将结果存入变量
            // 但 consequence 和 alternative 是 BlockStatement*（语句块）
            // 我们用一个临时变量存储 if 表达式的结果
            std::string result_var = ".if_result" + std::to_string(label_counter);
            int result_offset = getOrAllocOffset(result_var);

            std::string else_label = newLabel();
            std::string end_label = newLabel();

            genExpressionToReg(expr->getCondition(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "je " + else_label + "\n";

            // consequence 语句块
            genBlockBody(expr->getConsequence());
            // 默认结果值为 1
            text_section += "mov " + reg + ", 1\n";
            text_section += storeFromReg(result_offset, reg);
            text_section += "jmp " + end_label + "\n";

            text_section += else_label + ":\n";
            if (expr->getAlternative())
            {
                genBlockBody(expr->getAlternative());
                text_section += "mov " + reg + ", 1\n";
            }
            else
            {
                text_section += "xor " + reg + ", " + reg + "\n";
            }
            text_section += storeFromReg(result_offset, reg);

            text_section += end_label + ":\n";
            // 重新加载结果到寄存器
            text_section += loadToReg(result_offset, reg);
        }

        void genWhileExpression(WhileExpression *expr, const std::string &reg)
        {
            std::string begin_label = newLabel();
            std::string end_label = newLabel();

            text_section += begin_label + ":\n";
            genExpressionToReg(expr->getCondition(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "je " + end_label + "\n";

            genBlockBody(expr->getBody());
            text_section += "jmp " + begin_label + "\n";

            text_section += end_label + ":\n";
            // while 表达式的结果为 0
            text_section += "xor " + reg + ", " + reg + "\n";
        }

        void genUntilExpression(UntilExpression *expr, const std::string &reg)
        {
            std::string begin_label = newLabel();
            std::string end_label = newLabel();

            text_section += begin_label + ":\n";
            genBlockBody(expr->getBody());
            genExpressionToReg(expr->getCondition(), reg);
            text_section += "cmp " + reg + ", 0\n";
            text_section += "je " + begin_label + "\n";

            text_section += end_label + ":\n";
            text_section += "xor " + reg + ", " + reg + "\n";
        }

        void genSwitchExpression(SwitchExpression *expr, const std::string &reg)
        {
            std::string end_label = newLabel();
            std::string result_var = ".switch_result" + std::to_string(label_counter);
            int result_offset = getOrAllocOffset(result_var);

            genExpressionToReg(expr->getExpr(), reg);

            for (const auto &cs : expr->getCases())
            {
                std::string next_label = newLabel();

                // 比较当前 case 值
                text_section += "push " + reg + "\n";
                genExpressionToReg(cs->mcase.get(), "rbx");
                text_section += "pop " + reg + "\n";
                text_section += "cmp " + reg + ", rbx\n";
                text_section += "jne " + next_label + "\n";

                genBlockBody(cs->mbody.get());
                text_section += "jmp " + end_label + "\n";

                text_section += next_label + ":\n";
            }

            // default case
            auto *default_case = expr->getDefaultCase();
            if (default_case)
            {
                genBlockBody(default_case->mbody.get());
            }

            text_section += end_label + ":\n";
            text_section += "xor " + reg + ", " + reg + "\n";
        }

        void genIndexExpression(IndexExpression *expr, const std::string &reg)
        {
            genExpressionToReg(expr->getLeft(), reg); // 数组基地址
            text_section += "push " + reg + "\n";
            genExpressionToReg(expr->getIndex(), "rbx"); // 索引
            text_section += "pop " + reg + "\n";
            text_section += "mov " + reg + ", [" + reg + " + rbx*8]\n";
        }

        void genArrayLiteral(ArrayLiteral *expr, const std::string &reg)
        {
            // 分配内存: malloc(size * 8)
            int count = expr->getElems().size();
            text_section += "mov rdi, " + std::to_string(count * 8 + 8) + "\n";
            text_section += "call malloc\n";
            // rax = 分配好的数组指针
            text_section += "mov [rax], " + std::to_string(count) + "\n"; // 存储长度

            for (int i = 0; i < count; i++)
            {
                text_section += "push rax\n";
                genExpressionToReg(expr->getElems()[i].get(), "rbx");
                text_section += "pop rax\n";
                text_section += "mov [rax + " + std::to_string((i + 1) * 8) + "], rbx\n";
            }
            // rax 保留为返回值
            if (reg != "rax")
                text_section += "mov " + reg + ", rax\n";
        }

        void genNewExpression(NewExpression *expr, const std::string &reg)
        {
            // new ClassName() -> malloc(sizeof(ClassName))
            auto *right = expr->getRight();
            std::string class_name;

            if (auto *ident = dynamic_cast<Ident *>(right))
            {
                class_name = ident->getLiteral();
            }
            else if (auto *call = dynamic_cast<CallExpression *>(right))
            {
                auto *func = call->getFunc();
                if (auto *ident = dynamic_cast<Ident *>(func))
                {
                    class_name = ident->getLiteral();
                }
            }

            if (class_name.empty())
            {
                text_section += "xor " + reg + ", " + reg + "\n";
                return;
            }

            // 假定对象大小为 64 字节（可根据类成员计算，此处简化）
            text_section += "mov rdi, 64\n";
            text_section += "call malloc\n";
            // rax = 对象指针

            if (reg != "rax")
                text_section += "mov " + reg + ", rax\n";
        }

        void genMemberAccess(InfixExpression *expr, const std::string &reg)
        {
            genExpressionToReg(expr->getLeft(), reg);
            // 右侧成员名，简化：返回对象指针
        }

        // ==================== 函数调用 ====================
        void genCallExpression(CallExpression *expr, const std::string &reg)
        {
            auto *func_expr = expr->getFunc();
            std::string func_name;

            if (auto *ident = dynamic_cast<Ident *>(func_expr))
            {
                func_name = ident->getLiteral();
            }

            auto &args = expr->getArgs();

            // ===== 内置函数：print =====
            if (func_name == "print")
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    genExpressionToReg(args[i].get(), "rdi");
                    // 调用 printf("%s", x) 或 printf("%lld", x)
                    // 简化：作为字符串打印
                    text_section += "mov rsi, rdi\n";
                    text_section += "lea rdi, [str_fmt]\n";
                    text_section += "call printf\n";
                }
                if (reg != "rax")
                    text_section += "xor " + reg + ", " + reg + "\n";
                return;
            }

            // ===== 内置函数：println =====
            if (func_name == "println")
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    genExpressionToReg(args[i].get(), "rdi");
                    text_section += "mov rsi, rdi\n";
                    text_section += "lea rdi, [str_fmt]\n";
                    text_section += "call printf\n";
                }
                // 打印换行
                text_section += "lea rdi, [str_endl]\n";
                text_section += "call printf\n";
                if (reg != "rax")
                    text_section += "xor " + reg + ", " + reg + "\n";
                return;
            }

            // ===== 内置函数：scan =====
            if (func_name == "scan")
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    text_section += "sub rsp, 8\n";
                    text_section += "lea rsi, [rsp]\n";
                    text_section += "lea rdi, [scan_int_fmt]\n";
                    text_section += "call scanf\n";

                    if (auto *ident = dynamic_cast<Ident *>(args[i].get()))
                    {
                        std::string varname = ident->getLiteral();
                        int offset = getOrAllocOffset(varname);
                        text_section += "pop rax\n";
                        text_section += storeFromReg(offset, "rax");
                    }
                    else
                    {
                        text_section += "add rsp, 8\n";
                    }
                }
                if (reg != "rax")
                    text_section += "xor " + reg + ", " + reg + "\n";
                return;
            }

            // ===== 普通函数调用 =====
            std::vector<std::string> arg_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

            // 从右向左压栈
            for (int i = (int)args.size() - 1; i >= 0; i--)
            {
                genExpressionToReg(args[i].get(), "rax");
                text_section += "push rax\n";
            }

            for (int i = 0; i < (int)args.size() && i < 6; i++)
            {
                text_section += "pop " + arg_regs[i] + "\n";
            }

            text_section += "call " + func_name + "\n";

            if (reg != "rax")
                text_section += "mov " + reg + ", rax\n";
        }

        // ==================== 函数定义 ====================
        void genFuncDecl(FuncDecl *stmt)
        {
            std::string func_name = stmt->getLiteral();

            // 保存当前状态
            auto saved_var_offsets = var_offsets;
            int saved_next_offset = next_offset;
            auto saved_string_constants = string_constants;
            int saved_string_counter = string_counter;

            var_offsets.clear();
            next_offset = -8;

            std::string func_body;
            std::swap(text_section, func_body);

            // 函数头
            text_section = "\n" + func_name + ":\n";
            text_section += "push rbp\n";
            text_section += "mov rbp, rsp\n";

            // 声明参数
            const auto &params = stmt->getParams();
            std::vector<std::string> param_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            for (size_t i = 0; i < params.size() && i < 6; i++)
            {
                std::string param_name = params[i]->name->getLiteral();
                int offset = getOrAllocOffset(param_name);
                text_section += storeFromReg(offset, param_regs[i]);
            }

            // 函数体
            genBlockBody(stmt->getBody());

            // 函数结束
            int stack_size = getStackSize();
            if (stack_size > 0)
            {
                text_section += "add rsp, " + std::to_string(stack_size) + "\n";
            }
            text_section += "mov rsp, rbp\n";
            text_section += "pop rbp\n";
            text_section += "ret\n\n";

            // 保存函数定义
            std::string func_text = text_section;
            text_section = func_body;

            // 函数定义追加到全局代码
            asm_code += func_text;

            var_offsets = saved_var_offsets;
            next_offset = saved_next_offset;
            string_constants = saved_string_constants;
            string_counter = saved_string_counter;
        }

        // ==================== Lambda ====================
        void genLambdaExpression(LambdaExpr *expr, const std::string &reg)
        {
            std::string lambda_label = ".lambda." + std::to_string(label_counter++);

            // 保存上下文
            auto saved_offsets = var_offsets;
            auto saved_next = next_offset;
            var_offsets.clear();
            next_offset = -8;

            std::string saved_text;
            std::swap(saved_text, text_section);

            text_section = lambda_label + ":\n";
            text_section += "push rbp\n";
            text_section += "mov rbp, rsp\n";

            const auto &params = expr->getParams();
            std::vector<std::string> param_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
            for (size_t i = 0; i < params.size() && i < 6; i++)
            {
                std::string param_name = params[i]->name->getLiteral();
                int offset = getOrAllocOffset(param_name);
                text_section += storeFromReg(offset, param_regs[i]);
            }

            genBlockBody(expr->getBody());

            text_section += "mov rsp, rbp\n";
            text_section += "pop rbp\n";
            text_section += "ret\n";

            std::string func_code = text_section;
            text_section = saved_text;
            text_section += "lea " + reg + ", [" + lambda_label + "]\n";

            // 追加函数定义到 asm_code
            asm_code += func_code + "\n";

            var_offsets = saved_offsets;
            next_offset = saved_next;
        }

        // ==================== try/catch ====================
        void genTryCatchStatement(TryCatchStatement *stmt)
        {
            genBlockBody(stmt->getTryBody());
        }

        // ==================== 类 ====================
        void genClassStatement(ClassLiteral *stmt)
        {
            std::string class_name = stmt->getLiteral();
            auto &members = stmt->getMembers();

            // 在数据段生成类的大小信息
            data_section += class_name + "_size equ 64\n";

            // 为类中的方法生成函数
            for (const auto &mem : members)
            {
                if (mem.type == MemberType::Method)
                {
                    if (auto *func = dynamic_cast<FuncDecl *>(mem.declaration.get()))
                    {
                        std::string method_name = class_name + "_" + func->getLiteral();
                        auto saved_offsets = var_offsets;
                        auto saved_next = next_offset;
                        var_offsets.clear();
                        next_offset = -8;

                        std::string saved_text;
                        std::swap(saved_text, text_section);

                        text_section = method_name + ":\n";
                        text_section += "push rbp\n";
                        text_section += "mov rbp, rsp\n";

                        const auto &params = func->getParams();
                        std::vector<std::string> param_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

                        // 第一个参数是 this（rdi）
                        int this_offset = getOrAllocOffset("this");
                        text_section += storeFromReg(this_offset, "rdi");

                        for (size_t i = 0; i < params.size() && i < 5; i++)
                        {
                            std::string param_name = params[i]->name->getLiteral();
                            int offset = getOrAllocOffset(param_name);
                            text_section += storeFromReg(offset, param_regs[i + 1]);
                        }

                        genBlockBody(func->getBody());

                        int stack_size = getStackSize();
                        if (stack_size > 0)
                            text_section += "add rsp, " + std::to_string(stack_size) + "\n";
                        text_section += "mov rsp, rbp\n";
                        text_section += "pop rbp\n";
                        text_section += "ret\n\n";

                        asm_code += text_section;
                        text_section = saved_text;

                        var_offsets = saved_offsets;
                        next_offset = saved_next;
                    }
                }
            }
        }
    };
}