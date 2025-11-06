#pragma once
#include "ir/ir_generator.h"
#include <unordered_map>

namespace ns
{
    //ns::x86Generator 是一个用于将中间表示（IR）转换为 x86 汇编代码的类。它通过计算变量偏移、栈空间大小，并逐条翻译三地址码指令，生成符合 x86-64 规范的汇编代码。
    class x86Generator
    {
    private:
        // 为当前函数IR计算变量偏移（临时变量地址仅在当前函数内有效）
        int get_offset(
            const FuncIR &ir,
            const std::string &var,
            std::unordered_map<std::string, int> &temp_offsets, // 临时变量映射（按函数独立）
            int &next_temp_offset                               // 下一个可用临时偏移
        )
        {
            // 1. 先检查是否是用户变量（如a、b）
            if (ir.var_offset.count(var))
            {
                return ir.var_offset.at(var);
            }
            // 2. 检查是否是已分配的临时变量（如t1、t2）
            if (temp_offsets.count(var))
            {
                return temp_offsets.at(var);
            }
            // 3. 分配新的临时变量偏移
            int off = next_temp_offset;
            temp_offsets[var] = off;
            next_temp_offset -= 8; // 每次分配8字节（64位）
            return off;
        }

        // 计算所需栈空间（取最负的偏移，确保覆盖所有变量）
        int calculate_stack_size(
            const FuncIR &ir,
            const std::unordered_map<std::string, int> &temp_offsets)
        {
            int min_offset = 0; // 初始值（栈向低地址增长，最负的偏移需要最大空间）

            // 检查用户变量
            for (const auto &[var, off] : ir.var_offset)
            {
                if (off < min_offset)
                    min_offset = off;
            }

            // 检查临时变量
            for (const auto &[var, off] : temp_offsets)
            {
                if (off < min_offset)
                    min_offset = off;
            }

            // 栈空间大小 = 最负偏移的绝对值（向上取整为16的倍数，符合x86-64对齐要求）
            int stack_size = -min_offset;
            if (stack_size % 16 != 0)
            {
                stack_size += 16 - (stack_size % 16);
            }
            return stack_size;
        }

    public:
        std::string generate(const FuncIR &ir)
        {
            std::string x86_asm_code;
            std::unordered_map<std::string, int> temp_offsets;            // 当前函数的临时变量地址映射
            std::unordered_map<std::string, std::string> float_constants; // 记录浮点数常量名
            int float_counter = 0;
            int next_temp_offset = -32; // 临时变量起始偏移（与用户变量错开，避免重叠）

            x86_asm_code += "section .rodata\n";
            // 第一步：预计算所有变量的偏移（确保地址唯一）
            // 遍历所有三地址码，为所有变量（包括临时变量）分配地址
            for (const auto &tac : ir.instructions)
            {
                get_offset(ir, tac.result, temp_offsets, next_temp_offset);
                if (tac.op != Op::LDC)
                { // LDC只有一个参数，其他指令需要检查arg1和arg2
                    get_offset(ir, tac.arg1, temp_offsets, next_temp_offset);
                    if (!tac.arg2.empty())
                    {
                        get_offset(ir, tac.arg2, temp_offsets, next_temp_offset);
                    }
                }
                else if (tac.type == IRType::FLOAT64)
                { // 对Float64常量单独处理并记录
                    std::string const_name = ".float_" + std::to_string(float_counter++);
                    float_constants[tac.arg1] = const_name;
                    x86_asm_code += const_name + " dq " + tac.arg1 + "\n"; // 定义浮点数
                }
            }

            // 第二步：计算栈空间并生成栈帧初始化指令
            int stack_size = calculate_stack_size(ir, temp_offsets);
            x86_asm_code += "section .text\n";
            x86_asm_code += "global _start\n\n";
            x86_asm_code += "_start:\n";
            x86_asm_code += "push rbp\n";
            x86_asm_code += "mov rbp, rsp\n";
            if (stack_size > 0)
            {
                x86_asm_code += "sub rsp, " + std::to_string(stack_size) + "\n"; // 分配栈空间
            }

            // 第三步：生成三地址码对应的汇编指令
            for (const auto &tac : ir.instructions)
            {
                if (tac.op == Op::LDC)
                {
                    int off = get_offset(ir, tac.result, temp_offsets, next_temp_offset);
                    // 对于Float64类型使用movsd指令和xmm0寄存器
                    if (tac.type == IRType::FLOAT64)
                    {
                        std::string const_name = float_constants[tac.arg1];
                        x86_asm_code += "movsd xmm0, [" + const_name + "]\n";
                        x86_asm_code += "movsd [rbp" + std::to_string(off) + "], xmm0\n";
                        continue;
                    }
                    std::string prefix = tac.get_asm_prefix();
                    x86_asm_code += "mov " + prefix + " [rbp" + std::to_string(off) + "], " + tac.arg1 + "\n";
                }
                else if (tac.op == Op::ADD)
                { // t = a + b
                    int a_off = get_offset(ir, tac.arg1, temp_offsets, next_temp_offset);
                    int b_off = get_offset(ir, tac.arg2, temp_offsets, next_temp_offset);
                    int res_off = get_offset(ir, tac.result, temp_offsets, next_temp_offset);

                    x86_asm_code += "mov rax, [rbp" + std::to_string(a_off) + "]\n";
                    x86_asm_code += "mov rbx, [rbp" + std::to_string(b_off) + "]\n";
                    x86_asm_code += "add rax, rbx\n";
                    x86_asm_code += "mov [rbp" + std::to_string(res_off) + "], rax\n";
                }
                else if (tac.op == Op::MOV)
                { // a = t
                    int t_off = get_offset(ir, tac.arg1, temp_offsets, next_temp_offset);
                    int a_off = get_offset(ir, tac.result, temp_offsets, next_temp_offset);
                    if (tac.type == IRType::FLOAT64)
                    {
                        x86_asm_code += "movsd xmm0, [rbp" + std::to_string(t_off) + "]\n";
                        x86_asm_code += "movsd [rbp" + std::to_string(a_off) + "], xmm0\n";
                        continue;
                    }

                    x86_asm_code += "mov rax, [rbp" + std::to_string(t_off) + "]\n";
                    x86_asm_code += "mov [rbp" + std::to_string(a_off) + "], rax\n";
                }
                // 其他指令（MUL、SUB等）类似处理
            }

            // 函数结束：恢复栈帧并返回
            x86_asm_code += "mov rsp, rbp\n";
            x86_asm_code += "pop rbp\n";


            x86_asm_code += "mov rax, 60\n";
            x86_asm_code += "xor rdi, rdi\n";
            x86_asm_code += "syscall\n";
            return x86_asm_code;
        }
    };
}
