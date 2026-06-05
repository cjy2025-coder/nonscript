#pragma once
#include "../ir/ir.h"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <algorithm>

namespace ns
{
    class x86Generator
    {
    private:
        std::ostringstream asm_code;
        std::unordered_map<std::string, int> var_offsets;
        std::unordered_map<std::string, int> label_map;
        int next_offset = -8;
        int string_counter = 0;
        int float_counter = 0;
        // Windows x64 ABI: rcx, rdx, r8, r9
        const char* param_regs[4] = {"rcx", "rdx", "r8", "r9"};
        int current_param_reg_idx = 0;
        std::vector<std::string> pending_params;
        std::string data_section;
        std::string bss_section;

        int getOrAllocOffset(const std::string &var)
        {
            auto it = var_offsets.find(var);
            if (it != var_offsets.end())
                return it->second;
            int off = next_offset;
            var_offsets[var] = off;
            next_offset -= 8;
            return off;
        }

        std::string stack_addr(const std::string &var)
        {
            int off = getOrAllocOffset(var);
            std::string sign = (off >= 0) ? "+" : "";
            return "[rbp" + sign + std::to_string(off) + "]";
        }

        std::string new_string_label(const std::string &str)
        {
            std::string label = "str" + std::to_string(string_counter++);
            if (str.find('\\') != std::string::npos)
            {
                data_section += label + " db `" + str + "`, 0\n";
            }
            else
            {
                data_section += label + " db \"" + str + "\", 0\n";
            }
            return label;
        }

        std::string get_asm_suffix(IRType type)
        {
            switch (type)
            {
            case IRType::INT8:  return "b";
            case IRType::INT16: return "w";
            case IRType::INT32: return "l";
            case IRType::INT64:
            default: return "q";
            }
        }

        bool is_number_str(const std::string &s)
        {
            if (s.empty()) return false;
            size_t start = (s[0] == '-') ? 1 : 0;
            for (size_t i = start; i < s.size(); i++)
                if (!isdigit(s[i])) return false;
            return true;
        }

        // 计算栈帧总大小（16字节对齐，最小32字节）
        int calc_frame_size()
        {
            int used = -next_offset - 8;
            int size = (used + 15) / 16 * 16;
            if (size < 32) size = 32;
            return size;
        }

        // 预扫描一条指令的所有变量引用，分配偏移
        void pre_alloc_tac_vars(const TAC &tac)
        {
            if (!tac.result.empty() && tac.result[0] == 't')
                getOrAllocOffset(tac.result);
            if (!tac.arg1.empty() && tac.arg1[0] == 't')
                getOrAllocOffset(tac.arg1);
            if (!tac.arg2.empty() && tac.arg2[0] == 't')
                getOrAllocOffset(tac.arg2);
            // 非临时变量预分配：跳过 CALL 和 MOV 中可能是函数标签的 arg1
            if (tac.op != Op::CALL && tac.op != Op::MOV) {
                if (!tac.result.empty() && tac.result[0] != 't' && tac.result[0] != '.')
                    getOrAllocOffset(tac.result);
                if (!tac.arg1.empty() && tac.arg1[0] != 't' && tac.arg1[0] != '.')
                    getOrAllocOffset(tac.arg1);
                if (!tac.arg2.empty() && tac.arg2[0] != 't' && tac.arg2[0] != '.')
                    getOrAllocOffset(tac.arg2);
            } else if (tac.op == Op::MOV) {
                // MOV: result 是栈变量需要分配，arg1 可能是函数标签跳过
                if (!tac.result.empty() && tac.result[0] != 't' && tac.result[0] != '.')
                    getOrAllocOffset(tac.result);
            }
        }

    public:
        std::string generate(const ProgramIR &pir)
        {
            asm_code.str("");
            data_section = "section .rodata\n";
            bss_section = "section .bss\n";

            asm_code << "default rel\n";
            asm_code << "section .text\n";
            asm_code << "global main\n";
            asm_code << "extern printf\n";
            asm_code << "extern scanf\n";
            asm_code << "extern malloc\n";
            asm_code << "extern calloc\n\n";

            data_section += "int_fmt db \"%lld\", 0\n";
            data_section += "str_fmt db \"%s\", 0\n";
            data_section += "newline db 10, 0\n";
            data_section += "scanf_fmt db \"%lld\", 0\n";

            for (auto &func : pir.functions)
                generate_func(func);

            return data_section + "\n" + bss_section + "\n" + asm_code.str();
        }

    private:
        void generate_func(const FuncIR &func)
        {
            var_offsets.clear();
            next_offset = -8;

            for (const auto& param : func.params) {
                getOrAllocOffset(param);
            }

            for (auto &tac : func.instructions) {
                pre_alloc_tac_vars(tac);
            }

            int frame_size = calc_frame_size();

            asm_code << func.name << ":\n";
            asm_code << "  push rbp\n";
            asm_code << "  mov rbp, rsp\n";
            asm_code << "  sub rsp, " << frame_size << "\n";

            current_param_reg_idx = 0;

            for (; current_param_reg_idx < func.param_count && current_param_reg_idx < 4; current_param_reg_idx++)
            {
                asm_code << "  mov " << stack_addr(func.params[current_param_reg_idx]) << ", " << param_regs[current_param_reg_idx] << "\n";
            }
            for (; current_param_reg_idx < func.param_count; current_param_reg_idx++){
                int param_offset = 16 + (current_param_reg_idx - 4) * 8;
                asm_code << "  mov rax, [rbp+" << param_offset << "]\n";
                asm_code << "  mov " << stack_addr(func.params[current_param_reg_idx]) << ", rax\n";
            }

            current_param_reg_idx = 0;
            for (auto &tac : func.instructions)
                generate_tac(tac);
            current_param_reg_idx = 0;

            asm_code << "  mov rsp, rbp\n";
            asm_code << "  pop rbp\n";
            asm_code << "  ret\n\n";
        }

        void generate_tac(const TAC &tac)
        {
            switch (tac.op)
            {
            case Op::LABEL:
                asm_code << tac.label << ":\n";
                break;

            case Op::LDC:
                if (tac.type == IRType::STRING)
                {
                    std::string str_val = tac.arg1;
                    if (str_val.size() >= 2 && str_val[0] == '"' && str_val.back() == '"')
                        str_val = str_val.substr(1, str_val.size() - 2);
                    std::string label = new_string_label(str_val);
                    asm_code << "  lea rax, [" << label << "]\n";
                    asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                }
                else
                {
                    asm_code << "  mov rax, " << tac.arg1 << "\n";
                    asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                }
                break;

            case Op::MOV:
                if (var_offsets.count(tac.arg1)) {
                    asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                } else {
                    asm_code << "  lea rax, [" << tac.arg1 << "]\n";
                }
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::ADD:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  add rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::SUB:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  sub rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::MUL:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  imul rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::DIV:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  xor rdx, rdx\n";
                asm_code << "  idiv " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::MOD:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  xor rdx, rdx\n";
                asm_code << "  idiv " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rdx\n";
                break;

            case Op::NEG:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  neg rax\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::NOT:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, 0\n";
                asm_code << "  sete al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::AND:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  and rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::OR:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  or rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_EQ:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  sete al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_NE:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  setne al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_LT:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  setl al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_GT:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  setg al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_LE:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  setle al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::CMP_GE:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, " << stack_addr(tac.arg2) << "\n";
                asm_code << "  setge al\n";
                asm_code << "  movzx rax, al\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::JMP:
                asm_code << "  jmp " << tac.arg1 << "\n";
                break;

            case Op::JE:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, 0\n";
                asm_code << "  je " << tac.arg2 << "\n";
                break;

            case Op::JNE:
                asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                asm_code << "  cmp rax, 0\n";
                asm_code << "  jne " << tac.arg2 << "\n";
                break;

            case Op::CALL:
               {
                    int nparams = (int)pending_params.size();
                    int stack_params_size = (nparams > 4) ? (nparams - 4) * 8 : 0;
                    int total_alloc = 32 + stack_params_size;

                    asm_code << "  sub rsp, " << total_alloc << "\n";

                    for (int i = 0; i < nparams && i < 4; i++) {
                        asm_code << "  mov " << param_regs[i] << ", " << stack_addr(pending_params[i]) << "\n";
                    }

                    for (int i = 4; i < nparams; i++) {
                        int stack_off = (i - 4) * 8;
                        asm_code << "  mov rax, " << stack_addr(pending_params[i]) << "\n";
                        asm_code << "  mov [rsp+" << stack_off << "], rax\n";
                    }

                     if (var_offsets.count(tac.arg1)) {
                         asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                         asm_code << "  call rax\n";
                     } else {
                         asm_code << "  call " << tac.arg1 << "\n";
                     }
                    asm_code << "  add rsp, " << total_alloc << "\n";

                    pending_params.clear();
                    current_param_reg_idx = 0;
                    if (!tac.result.empty())
                        asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
               }
               break;

            case Op::RET:
                if (!tac.arg1.empty())
                {
                    asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";
                }
                break;

            case Op::PARAM:
                {
                    pending_params.push_back(tac.result);
                }
                break;

            case Op::INDEX:
                // 读取: result = arg1[arg2]
                {
                    std::string idx_val = tac.arg2;
                    bool is_const_idx = is_number_str(idx_val);

                    asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";  // 基地址 -> rax
                    if (is_const_idx) {
                        long long offset = std::stoll(idx_val) * 8;
                        asm_code << "  mov rdx, [rax + " << offset << "]\n";
                        asm_code << "  mov " << stack_addr(tac.result) << ", rdx\n";
                    } else {
                        asm_code << "  mov rdx, " << stack_addr(tac.arg2) << "\n";
                        asm_code << "  mov rdx, [rax + rdx*8]\n";
                        asm_code << "  mov " << stack_addr(tac.result) << ", rdx\n";
                    }
                }
                break;

            case Op::STORE_INDEX:
                // 写入: arg1[arg2] = result
                {
                    std::string idx_val = tac.arg2;
                    bool is_const_idx = is_number_str(idx_val);

                    asm_code << "  mov rax, " << stack_addr(tac.arg1) << "\n";  // 基地址 -> rax
                    asm_code << "  mov rdx, " << stack_addr(tac.result) << "\n";  // 值 -> rdx
                    if (is_const_idx) {
                        long long offset = std::stoll(idx_val) * 8;
                        asm_code << "  mov [rax + " << offset << "], rdx\n";
                    } else {
                        asm_code << "  mov rcx, " << stack_addr(tac.arg2) << "\n";  // 索引 -> rcx
                        asm_code << "  mov [rax + rcx*8], rdx\n";
                    }
                }
                break;

            case Op::ALLOC:
                // calloc(1, size) → rax
                if (tac.arg1 == "array")
                    asm_code << "  mov rdx, 64\n";
                else
                    asm_code << "  mov rdx, 64\n";
                asm_code << "  mov rcx, 1\n";
                asm_code << "  sub rsp, 32\n";
                asm_code << "  call calloc\n";
                asm_code << "  add rsp, 32\n";
                asm_code << "  mov " << stack_addr(tac.result) << ", rax\n";
                break;

            case Op::PRINT:
                if (tac.arg2 == "str") {
                    asm_code << "  sub rsp, 32\n";
                    asm_code << "  mov rdx, " << stack_addr(tac.arg1) << "\n";
                    asm_code << "  lea rcx, [rel str_fmt]\n";
                    asm_code << "  call printf\n";
                    asm_code << "  add rsp, 32\n";
                } else {
                    asm_code << "  sub rsp, 32\n";
                    asm_code << "  mov rdx, " << stack_addr(tac.arg1) << "\n";
                    asm_code << "  lea rcx, [rel int_fmt]\n";
                    asm_code << "  call printf\n";
                    asm_code << "  add rsp, 32\n";
                }
                break;

            case Op::SCAN:
                if (!tac.result.empty())
                {
                    asm_code << "  sub rsp, 32\n";
                    asm_code << "  lea rcx, [rel scanf_fmt]\n";
                    asm_code << "  lea rdx, " << stack_addr(tac.result) << "\n";
                    asm_code << "  call scanf\n";
                    asm_code << "  add rsp, 32\n";
                }
                break;

            default:
                break;
            }
        }
    };
}