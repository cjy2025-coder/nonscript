#pragma once
#include <string>
#include <vector>
#include <map>

namespace ns
{
    // IR 操作类型
    enum class Op
    {
        LDC,    // 加载常量
        MOV,    // 变量赋值
        ADD, SUB, MUL, DIV, MOD, // 算术运算
        NEG,    // 取负
        NOT,    // 逻辑非
        AND, OR, // 逻辑运算
        CMP_EQ, CMP_NE, CMP_LT, CMP_GT, CMP_LE, CMP_GE, // 比较
        JMP,    // 无条件跳转
        JE, JNE, // 条件跳转
        LABEL,  // 标签定义
        CALL,   // 函数调用
        RET,    // 返回
        PARAM,  // 参数传递
        ALLOC,  // 内存分配 (new)
        INDEX,  // 数组/成员读取: result = arg1[arg2]
        STORE_INDEX, // 数组/成员写入: arg1[arg2] = result
        PRINT,  // print 内置
        SCAN,   // scan 内置
        NOP     // 空操作
    };

    // IR 数据类型
    enum class IRType
    {
        INT8, INT16, INT32, INT64,
        FLOAT32, FLOAT64,
        BOOLEAN,
        STRING,
        PTR,    // 指针
        VOID
    };

    // 三地址码指令
    struct TAC
    {
        Op op = Op::NOP;
        std::string result;  // 目标变量
        std::string arg1;    // 操作数1
        std::string arg2;    // 操作数2
        IRType type = IRType::INT64;
        std::string label;   // LABEL 指令的标签名

        // 获取汇编前缀（用于数据宽度）
        std::string get_asm_prefix() const
        {
            switch (type)
            {
            case IRType::INT8:  return "byte ";
            case IRType::INT16: return "word ";
            case IRType::INT32: return "dword ";
            case IRType::INT64: return "qword ";
            default: return "qword ";
            }
        }
    };

    // 单个函数的 IR
    struct FuncIR
    {
        std::string name;                       // 函数名
        std::vector<TAC> instructions;          // 三地址码列表
        // std::map<std::string, int> var_offset;
        std::vector<std::string> params;  // 参数名列表
        int param_count = 0;                    // 参数个数
    };

    // 整个程序的 IR（多函数）
    struct ProgramIR
    {
        std::vector<FuncIR> functions;
        std::map<std::string, int> class_sizes;  // 类名 -> 对象大小
    };
}