#include "./optimization/optimizer.h"
#include <set>

namespace ns
{
    void Optimizer::optimize(ProgramIR &pir)
    {
        for (auto &func : pir.functions)
        {
            // 多轮优化直到收敛
            for (int i = 0; i < 3; i++)
            {
                algebraic_simplification(func);
                constant_folding(func);
                dead_code_elimination(func);
            }
        }
    }

    void Optimizer::algebraic_simplification(FuncIR &func)
    {
        std::vector<TAC> new_insts;
        std::unordered_map<std::string, std::string> known_values; // 变量名 -> 已知常量值

        for (auto &tac : func.instructions)
        {
            if (tac.op == Op::LDC)
            {
                known_values[tac.result] = tac.arg1;
                new_insts.push_back(tac);
            }
            else if (tac.op == Op::ADD && (tac.arg2 == "0" || tac.arg1 == "0"))
            {
                // x + 0 = x,  0 + x = x
                std::string src = (tac.arg2 == "0") ? tac.arg1 : tac.arg2;
                TAC mov;
                mov.op = Op::MOV;
                mov.result = tac.result;
                mov.arg1 = src;
                new_insts.push_back(mov);
            }
            else if (tac.op == Op::SUB && tac.arg2 == "0")
            {
                // x - 0 = x
                TAC mov;
                mov.op = Op::MOV;
                mov.result = tac.result;
                mov.arg1 = tac.arg1;
                new_insts.push_back(mov);
            }
            else if (tac.op == Op::MUL && (tac.arg2 == "1" || tac.arg1 == "1"))
            {
                // x * 1 = x
                std::string src = (tac.arg2 == "1") ? tac.arg1 : tac.arg2;
                TAC mov;
                mov.op = Op::MOV;
                mov.result = tac.result;
                mov.arg1 = src;
                new_insts.push_back(mov);
            }
            else if (tac.op == Op::MUL && (tac.arg2 == "0" || tac.arg1 == "0"))
            {
                // x * 0 = 0
                TAC ldc;
                ldc.op = Op::LDC;
                ldc.result = tac.result;
                ldc.arg1 = "0";
                new_insts.push_back(ldc);
            }
            else if (tac.op == Op::DIV && tac.arg1 == tac.arg2)
            {
                // x / x = 1 (x != 0)
                TAC ldc;
                ldc.op = Op::LDC;
                ldc.result = tac.result;
                ldc.arg1 = "1";
                new_insts.push_back(ldc);
            }
            else
            {
                new_insts.push_back(tac);
            }
        }

        func.instructions = new_insts;
    }

    void Optimizer::constant_folding(FuncIR &func)
    {
        std::vector<TAC> new_insts;
        std::unordered_map<std::string, std::string> const_values;

        for (auto &tac : func.instructions)
        {
            // 检查 arg1 是否在已知常量表中
            std::string arg1_val = tac.arg1;
            // 只对临时变量（以 t 开头）进行常量替换
            // 用户变量可能在循环中被修改，不能假定为常量
            auto it1 = const_values.find(tac.arg1);
            if (it1 != const_values.end() && !tac.arg1.empty() && tac.arg1[0] == 't')
                arg1_val = it1->second;

            std::string arg2_val = tac.arg2;
            auto it2 = const_values.find(tac.arg2);
            if (it2 != const_values.end() && !tac.arg2.empty() && tac.arg2[0] == 't')
                arg2_val = it2->second;

            // 如果两个操作数都是常量，进行折叠
            bool is_const1 = false;
            bool is_const2 = false;
            long long v1 = 0, v2 = 0;

            try
            {
                v1 = std::stoll(arg1_val);
                is_const1 = true;
            }
            catch (...) {}

            try
            {
                v2 = std::stoll(arg2_val);
                is_const2 = true;
            }
            catch (...) {}

            long long result = 0;
            bool can_fold = false;

            if (is_const1 && is_const2)
            {
                switch (tac.op)
                {
                case Op::ADD: result = v1 + v2; can_fold = true; break;
                case Op::SUB: result = v1 - v2; can_fold = true; break;
                case Op::MUL: result = v1 * v2; can_fold = true; break;
                case Op::DIV: if (v2 != 0) { result = v1 / v2; can_fold = true; } break;
                case Op::MOD: if (v2 != 0) { result = v1 % v2; can_fold = true; } break;
                default: break;
                }
            }

            if (can_fold)
            {
                TAC ldc;
                ldc.op = Op::LDC;
                ldc.result = tac.result;
                ldc.arg1 = std::to_string(result);
                ldc.type = tac.type;
                new_insts.push_back(ldc);
                const_values[tac.result] = std::to_string(result);
            }
            else
            {
                // 如果是 MOV 或 LDC，记录已知值
                if (tac.op == Op::LDC)
                {
                    const_values[tac.result] = tac.arg1;
                }
                else if (tac.op == Op::MOV)
                {
                    if (const_values.count(tac.arg1))
                        const_values[tac.result] = const_values[tac.arg1];
                    else
                        const_values.erase(tac.result);
                }
                else
                {
                    const_values.erase(tac.result);
                }
                new_insts.push_back(tac);
            }
        }

        func.instructions = new_insts;
    }

    void Optimizer::dead_code_elimination(FuncIR &func)
    {
        // 收集所有被使用的变量
        std::unordered_set<std::string> used_vars;
        for (auto &tac : func.instructions)
        {
            // arg1 和 arg2 的引用构成了使用
            if (!tac.arg1.empty() && tac.arg1[0] == 't')
                used_vars.insert(tac.arg1);
            if (!tac.arg2.empty() && tac.arg2[0] == 't')
                used_vars.insert(tac.arg2);

            // 总是保留的指令（PARAM、CALL、PRINT 等）中，result 和 arg1/arg2 都视为被使用
            if (tac.op == Op::PARAM || tac.op == Op::CALL ||
                tac.op == Op::RET || tac.op == Op::PRINT || tac.op == Op::SCAN ||
                tac.op == Op::JMP || tac.op == Op::JE || tac.op == Op::JNE ||
                tac.op == Op::ALLOC || tac.op == Op::INDEX ||
                tac.op == Op::STORE_INDEX)
            {
                if (!tac.result.empty() && tac.result[0] == 't')
                    used_vars.insert(tac.result);
                if (!tac.arg1.empty() && tac.arg1[0] == 't')
                    used_vars.insert(tac.arg1);
                if (!tac.arg2.empty() && tac.arg2[0] == 't')
                    used_vars.insert(tac.arg2);
            }
        }

        // 收集所有被跳转引用的标签
        std::unordered_set<std::string> used_labels;
        for (auto &tac : func.instructions)
        {
            if (tac.op == Op::JMP || tac.op == Op::JE || tac.op == Op::JNE)
            {
                if (!tac.arg2.empty() && tac.arg2[0] == '.')
                    used_labels.insert(tac.arg2);
            }
        }

        // 也检查 arg1 作为标签（JMP arg1, JE arg1, label 等形式）
        for (auto &tac : func.instructions)
        {
            if ((tac.op == Op::JE || tac.op == Op::JNE) && !tac.arg1.empty() && tac.arg1[0] == '.')
                used_labels.insert(tac.arg1);
            if (tac.op == Op::JMP && !tac.arg1.empty() && tac.arg1[0] == '.')
                used_labels.insert(tac.arg1);
        }

        std::vector<TAC> new_insts;
        for (size_t i = 0; i < func.instructions.size(); i++)
        {
            auto &tac = func.instructions[i];

            // 总是保留：跳转、标签、返回、调用、内置函数、NOP
            if (tac.op == Op::JMP || tac.op == Op::JE || tac.op == Op::JNE ||
                tac.op == Op::RET || tac.op == Op::CALL ||
                tac.op == Op::PRINT || tac.op == Op::SCAN ||
                tac.op == Op::PARAM || tac.op == Op::ALLOC ||
                tac.op == Op::INDEX || tac.op == Op::STORE_INDEX)
            {
                // 这些指令引用的变量也要标记为使用
                if (!tac.arg1.empty() && tac.arg1[0] == 't')
                    used_vars.insert(tac.arg1);
                if (!tac.arg2.empty() && tac.arg2[0] == 't')
                    used_vars.insert(tac.arg2);
                new_insts.push_back(tac);
                continue;
            }

            // 保留被使用的标签
            if (tac.op == Op::LABEL)
            {
                if (used_labels.count(tac.label))
                    new_insts.push_back(tac);
                continue;
            }

            // LDC 和 MOV：如果结果被使用则保留
            if ((tac.op == Op::LDC || tac.op == Op::MOV ||
                 tac.op == Op::ADD || tac.op == Op::SUB ||
                 tac.op == Op::MUL || tac.op == Op::DIV ||
                 tac.op == Op::MOD || tac.op == Op::NEG ||
                 tac.op == Op::NOT || tac.op == Op::AND ||
                 tac.op == Op::OR ||
                 tac.op == Op::CMP_EQ || tac.op == Op::CMP_NE ||
                 tac.op == Op::CMP_LT || tac.op == Op::CMP_GT ||
                 tac.op == Op::CMP_LE || tac.op == Op::CMP_GE))
            {
                if (used_vars.count(tac.result) || tac.result.empty() || tac.result[0] != 't')
                    new_insts.push_back(tac);
                // 否则：死代码，丢弃
                continue;
            }

            new_insts.push_back(tac);
        }

        func.instructions = new_insts;
    }
}