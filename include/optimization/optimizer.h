#pragma once
#include "../ir/ir.h"
#include <unordered_set>
#include <unordered_map>

namespace ns
{
    class Optimizer
    {
    private:
        // 常量折叠：在单个函数内进行
        void constant_folding(FuncIR &func);

        // 死代码消除：删除结果未被使用的临时变量赋值
        void dead_code_elimination(FuncIR &func);

        // 代数简化：如 x + 0 = x, x * 1 = x, x * 0 = 0
        void algebraic_simplification(FuncIR &func);

    public:
        void optimize(ProgramIR &pir);
    };
}