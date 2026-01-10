#include "./ir/ir_generator.h"

namespace ns
{
    // 生成新的临时变量名（t1、t2、t3...）
    std::string IrGenerator::new_temp()
    {
        return "T" + std::to_string(++temp_counter); // 假设temp_counter是类成员变量，初始为0
    }
    std::string IrGenerator::visit_infix_expression(const InfixExpression *expr)
    {
        std::string left = visit(expr->getLeft());
        std::string right = visit(expr->getRight());
        std::string res = new_temp();
        TAC tac = {};
        tac.result = res;
        tac.arg1 = left;
        tac.arg2 = right;
        switch (expr->getToken().getType())
        {
        case TokenType::ADD:
            tac.op = Op::ADD;
            break;
        case TokenType::MINUS:
            tac.op = Op::SUB;
            break;
        case TokenType::MUL:
            tac.op = Op::MUL;
            break;
        case TokenType::DIV:
            tac.op = Op::DIV;
            break;
        case TokenType::MOD:
            tac.op = Op::MOD;
            break;
        default:
            break;
        }
        current_func.instructions.emplace_back(tac);
        return res;
    }
    std::string IrGenerator::visit_prefix_expression(const PrefixExpression *expr)
    {
    }
    std::string IrGenerator::visit_simple_expression(const Expression *expr)
    {
        // 标识符
        if (auto *ident = dynamic_cast<const Ident *>(expr))
        {
            
            return ident->getLiteral();
        }
        std::string res = new_temp();
        TAC tac = {};
        tac.op = Op::LDC;
        tac.result = res;
        // int8常量
        if (auto *num = dynamic_cast<const I8Literal *>(expr))
        {
            tac.type = IRType::INT8;
            tac.arg1 = std::to_string(num->getValue());
        }
        // int16常量
        else if (auto *num = dynamic_cast<const I16Literal *>(expr))
        {
            tac.type = IRType::INT16;
            tac.arg1 = std::to_string(num->getValue());
        }
        // int32常量
        else if (auto *num = dynamic_cast<const I32Literal *>(expr))
        {
            tac.type = IRType::INT32;
            tac.arg1 = std::to_string(num->getValue());
        }
        // int64常量
        else if (auto *num = dynamic_cast<const I64Literal *>(expr))
        {
            tac.type = IRType::INT64;
            tac.arg1 = std::to_string(num->getValue());
        }
        // float32常量
        else if (auto *num = dynamic_cast<const Float32Literal *>(expr))
        {
            tac.type = IRType::FLOAT32;
            tac.arg1 = std::to_string(num->getValue());
        }
        // float64常量
        else if (auto *num = dynamic_cast<const Float64Literal *>(expr))
        {
            tac.type = IRType::FLOAT64;
            tac.arg1 = std::to_string(num->getValue());
            tac.arg2 = "";
        }
        // 布尔常量,true/false
        else if (auto *val = dynamic_cast<const BoolLiteral *>(expr))
        {
            tac.type = IRType::BOOLEAN;
            tac.arg1 = (val->value() ? "1" : "0");
        }
        // 字符串
        else if (auto *val = dynamic_cast<const StringLiteral *>(expr))
        {
            tac.type = IRType::STRING;
            tac.arg1 = val->getLiteral();
        }
        current_func.instructions.emplace_back(tac);
        return res;
    }
    std::string IrGenerator::visit(const Expression *expr)
    {
        // 二元运算符
        if (auto *bin = dynamic_cast<const InfixExpression *>(expr))
        {
            return visit_infix_expression(bin);
        }
        else
        {
            return visit_simple_expression(expr);
        }
        return "undef";
    }
    FuncIR IrGenerator::generate_declare_statement(const DeclareStatement *stmt)
    {
        auto vars = stmt->getVars();
        for (auto &it : vars) // 遍历每个变量声明
        {
            std::string var_name = it.first->getLiteral();
            std::string init_var = visit(it.second);
            TAC tac;
            tac.op = Op::MOV;
            tac.result = var_name; // 目标变量（如"a"）
            tac.arg1 = init_var;   // 源变量（如"t3"）
            tac.arg2 = "";
            current_func.instructions.emplace_back(tac);
        }
        return current_func;
    }
    FuncIR IrGenerator::generate(const Program &program)
    {
        current_func.name = "main";
        for (const auto &stmt : program.stmts)
        {
            if (auto *stmt_ = dynamic_cast<const DeclareStatement *>(stmt))
            {
               return generate_declare_statement(stmt_);
            }
        }
        return current_func;
    }
}
