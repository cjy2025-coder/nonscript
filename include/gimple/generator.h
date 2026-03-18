#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "gimple/value.h"
#include "gimple/statement.h"
#include "frontend/semantic.h"
#include <stack>
namespace ns
{
    class GimpleCodeGen
    {
    private:
        // 符号表映射：源语言符号 -> GIMPLE 值
        std::unordered_map<std::string, GimpleValue *> symbol_map;

        // 当前生成的函数
        GimpleFunction *current_func;

        // 标签栈（用于循环、条件）
        struct LabelInfo
        {
            std::string start;
            std::string end;
            std::string body;
            std::string continue_label;
        };
        std::stack<LabelInfo> label_stack;

        // 语义分析器引用（复用你的类型检查结果）
        SemanticAnalyzer *semantic_analyzer;

    public:
        GimpleCodeGen(SemanticAnalyzer *analyzer) : semantic_analyzer(analyzer)
        {
            current_func = new GimpleFunction("main", createTypeInfo(DataType::INT64));
        }

        // 主生成函数
        GimpleFunction *generate(Program *program)
        {
            // 遍历所有语句，生成对应的 GIMPLE
            auto stmts = program->stmts;
            for (auto &stmt : stmts)
            {
                generateStatement(stmt);
            }
            return current_func;
        }

    private:
        // 生成语句
        void generateStatement(Statement *stmt)
        {
            if (typeid(*stmt) == typeid(DeclareStatement))
            {
                generateDeclareStatement(static_cast<DeclareStatement *>(stmt));
            }
            else if (typeid(*stmt) == typeid(BlockStatement))
            {
                generateBlockStatement(static_cast<BlockStatement *>(stmt));
            }
            else if (typeid(*stmt) == typeid(FuncDecl))
            {
                generateFuncStatement(static_cast<FuncDecl *>(stmt));
            }
            else if (typeid(*stmt) == typeid(ReturnStatement))
            {
                generateReturnStatement(static_cast<ReturnStatement *>(stmt));
            }
            else if (typeid(*stmt) == typeid(ExpressionStatement))
            {
                generateExpressionStatement(static_cast<ExpressionStatement *>(stmt));
            }
            else if (typeid(*stmt) == typeid(ClassLiteral))
            {
                generateClassStatement(static_cast<ClassLiteral *>(stmt));
            }
        }

        // 生成变量声明
        void generateDeclareStatement(DeclareStatement *stmt)
        {
            auto vars = stmt->getVars();
            for (auto &var_pair : vars)
            {
                auto ident = var_pair.first;
                auto expr = var_pair.second;
                if (expr)
                    std::cout << expr->toString() << std::endl;
                // 获取类型信息（复用语义分析结果）
                auto symbol = semantic_analyzer->find_symbol(ident->getLiteral());
                if (!symbol)
                    continue;

                // 创建 GIMPLE 值
                auto gimple_var = new GimpleValue(ident->getLiteral(), symbol->type_info);
                symbol_map[ident->getLiteral()] = gimple_var;
                auto _ = new GimpleDeclare(gimple_var);
                // 生成声明语句
                current_func->body->statements.push_back(_);

                // 如果有初始化表达式
                if (expr)
                {
                    auto rhs = generateExpression(expr);
                    if (rhs)
                    {
                        current_func->body->addStatement(
                            new GimpleAssign(
                                gimple_var, GimpleOp::ASSIGN, rhs));
                    }
                }
            }
        }

        // 生成函数声明
        void generateFuncStatement(FuncDecl *stmt)
        {
            std::string name = stmt->getLiteral();

            // 获取函数类型信息
            auto symbol = semantic_analyzer->find_symbol(name);
            if (!symbol)
                return;

            // 创建 GIMPLE 函数
            current_func = new GimpleFunction(name, symbol->type_info);

            // 处理参数
            auto params = stmt->getParams();
            for (auto &param : params)
            {
                if (typeid(*param) == typeid(Ident))
                {
                    auto ident = static_cast<Ident *>(param.get());
                    auto param_val = new GimpleValue(
                        ident->getLiteral(),
                        createTypeInfo(string_to_type(ident->getType())));
                    current_func->parameters.push_back(param_val);
                    symbol_map[ident->getLiteral()] = param_val;
                }
            }

            // 生成函数体
            current_func->body = std::make_unique<GimpleScope>();
            generateBlockStatement(stmt->getBody());
        }

        // 生成块语句
        void generateBlockStatement(BlockStatement *stmt)
        {
            auto stmts = stmt->value();
            for (auto &stmt : stmts)
            {
                generateStatement(stmt);
            }
        }

        // 生成返回语句
        void generateReturnStatement(ReturnStatement *stmt)
        {
            auto expr = stmt->getExpression();
            GimpleValue *ret_val = nullptr;

            if (expr)
            {
                ret_val = generateExpression(expr);
            }

            current_func->body->addStatement(
                std::make_unique<GimpleReturn>(ret_val).get());
        }

        // 生成表达式语句
        void generateExpressionStatement(ExpressionStatement *stmt)
        {
            auto expr = stmt->expression();
            generateExpression(expr); // 忽略返回值
        }

        // 生成表达式（核心）
        GimpleValue *generateExpression(Expression *expr)
        {
            if (!expr)
                return nullptr;

            if (typeid(*expr) == typeid(Ident))
            {
                return generateIdent(static_cast<Ident *>(expr));
            }
            else if (typeid(*expr) == typeid(I8Literal) ||
                     typeid(*expr) == typeid(I16Literal) ||
                     typeid(*expr) == typeid(I32Literal) ||
                     typeid(*expr) == typeid(I64Literal) ||
                     typeid(*expr) == typeid(Float32Literal) ||
                     typeid(*expr) == typeid(Float64Literal) ||
                     typeid(*expr) == typeid(BoolLiteral) ||
                     typeid(*expr) == typeid(StringLiteral))
            {
                return generateLiteral(expr);
            }
            else if (typeid(*expr) == typeid(InfixExpression))
            {
                return generateInfixExpression(static_cast<InfixExpression *>(expr));
            }
            else if (typeid(*expr) == typeid(PrefixExpression))
            {
                return generatePrefixExpression(static_cast<PrefixExpression *>(expr));
            }
            else if (typeid(*expr) == typeid(CallExpression))
            {
                return generateCallExpression(static_cast<CallExpression *>(expr));
            }
            else if (typeid(*expr) == typeid(IfExpression))
            {
                generateIfExpression(static_cast<IfExpression *>(expr));
                return nullptr; // if 表达式不返回值
            }
            else if (typeid(*expr) == typeid(WhileExpression))
            {
                generateWhileExpression(static_cast<WhileExpression *>(expr));
                return nullptr; // while 表达式不返回值
            }

            return nullptr;
        }

        // 生成标识符
        GimpleValue *generateIdent(Ident *ident)
        {
            auto it = symbol_map.find(ident->getLiteral());
            if (it != symbol_map.end())
            {
                return it->second;
            }
            return nullptr;
        }

        // 生成字面量
        GimpleValue *generateLiteral(Expression *expr)
        {
            // 获取类型信息
            auto type_info = semantic_analyzer->check_expression(expr);
            if (!type_info)
                return nullptr;

            // 创建常量值
            auto value = new GimpleValue("", type_info);
            value->is_constant = true;
            value->constant_value = expr->getLiteral();

            return value;
        }

        // 生成中缀表达式
        GimpleValue *generateInfixExpression(InfixExpression *expr)
        {
            auto left = generateExpression(expr->getLeft());
            auto right = generateExpression(expr->getRight());

            if (!left || !right)
                return nullptr;

            // 获取操作符
            std::string op_str = expr->getOperator();
            GimpleOp op;

            if (op_str == "+")
                op = GimpleOp::ADD;
            else if (op_str == "-")
                op = GimpleOp::SUB;
            else if (op_str == "*")
                op = GimpleOp::MUL;
            else if (op_str == "/")
                op = GimpleOp::DIV;
            else if (op_str == "%")
                op = GimpleOp::MOD;
            else if (op_str == "&&")
                op = GimpleOp::AND;
            else if (op_str == "||")
                op = GimpleOp::OR;
            else if (op_str == "==")
                op = GimpleOp::EQ;
            else if (op_str == "!=")
                op = GimpleOp::NE;
            else if (op_str == "<")
                op = GimpleOp::LT;
            else if (op_str == "<=")
                op = GimpleOp::LTE;
            else if (op_str == ">")
                op = GimpleOp::GT;
            else if (op_str == ">=")
                op = GimpleOp::GTE;
            else if (op_str == "=")
            {
                // 赋值语句
                current_func->body->addStatement(
                    new GimpleAssign(left, GimpleOp::ASSIGN, right));
                return left;
            }
            else
            {
                return nullptr;
            }

            // 对于算术/逻辑运算，创建临时变量
            auto result_type = semantic_analyzer->check_expression(expr);
            auto temp = current_func->createTemp(result_type);
            auto gimple_var = new GimpleValue(temp->toString(), result_type);
            symbol_map[temp->toString()] = gimple_var;
            auto _ = new GimpleDeclare(gimple_var);
            // 生成声明语句
            current_func->body->statements.push_back(_);
            current_func->body->addStatement(
                new GimpleAssign(temp, op, left, right));

            return temp;
        }

        // 生成前缀表达式
        GimpleValue *generatePrefixExpression(PrefixExpression *expr)
        {
            auto right = generateExpression(expr->getRight());
            if (!right)
                return nullptr;

            std::string op_str = expr->getOperator();

            if (op_str == "!")
            {
                // 逻辑非
                auto temp = current_func->createTemp(right->type);
                // 需要特殊处理，简化起见直接返回原值
                return right;
            }
            else if (op_str == "+" || op_str == "-")
            {
                // 一元加减
                return right;
            }

            return nullptr;
        }

        // 生成函数调用
        GimpleValue *generateCallExpression(CallExpression *expr)
        {
            auto func = expr->getFunc();
            auto func_name = func->getLiteral();

            // 收集参数
            std::vector<GimpleValue *> args;
            auto &expr_args = expr->getArgs();
            for (auto &arg : expr_args)
            {
                auto arg_val = generateExpression(arg.get());
                if (arg_val)
                {
                    args.push_back(arg_val);
                }
            }

            // 获取返回类型
            auto call_type = semantic_analyzer->check_call_expression(expr);
            GimpleValue *result = nullptr;

            if (call_type && call_type->baseType != DataType::NONE)
            {
                result = current_func->createTemp(call_type);
            }

            current_func->body->addStatement(
                std::make_unique<GimpleCall>(func_name, args, result).get());

            return result;
        }

        // 生成 if 表达式
        void generateIfExpression(IfExpression *expr)
        {
            auto condition = generateExpression(expr->getCondition());
            if (!condition)
                return;

            std::string then_label = current_func->newLabel("if_then");
            std::string else_label = current_func->newLabel("if_else");
            std::string end_label = current_func->newLabel("if_end");

            // 条件跳转
            current_func->body->addStatement(
                std::make_unique<GimpleCondGoto>(condition, then_label, else_label).get());

            // then 块
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(then_label).get());
            generateStatement(expr->getConsequence());

            // 跳转到结束
            current_func->body->addStatement(
                std::make_unique<GimpleGoto>(end_label).get());

            // else 块
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(else_label).get());
            auto alternative = expr->getAlternative();
            if (alternative)
            {
                generateStatement(alternative);
            }

            // 结束标签
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(end_label).get());
        }

        // 生成 while 表达式
        void generateWhileExpression(WhileExpression *expr)
        {
            std::string start_label = current_func->newLabel("while_start");
            std::string cond_label = current_func->newLabel("while_cond");
            std::string body_label = current_func->newLabel("while_body");
            std::string end_label = current_func->newLabel("while_end");

            // 保存标签信息
            label_stack.push({start_label, end_label, body_label, cond_label});

            // 开始标签
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(start_label).get());

            // 跳转到条件检查
            current_func->body->addStatement(
                std::make_unique<GimpleGoto>(cond_label).get());

            // 条件标签
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(cond_label).get());

            // 条件检查
            auto condition = generateExpression(expr->getCondition());
            if (condition)
            {
                current_func->body->addStatement(
                    std::make_unique<GimpleCondGoto>(condition, body_label, end_label).get());
            }

            // 循环体标签
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(body_label).get());

            // 生成循环体
            generateStatement(expr->getBody());

            // 跳回条件检查
            current_func->body->addStatement(
                std::make_unique<GimpleGoto>(cond_label).get());

            // 结束标签
            current_func->body->addStatement(
                std::make_unique<GimpleLabel>(end_label).get());

            label_stack.pop();
        }

        // 生成类声明（简化）
        void generateClassStatement(ClassLiteral *stmt)
        {
            // 类声明在 GIMPLE 中可能转换为结构体定义
            // 这里简化处理，只记录符号
            auto name = stmt->getLiteral();
            // 可以为类生成对应的结构体定义
        }
    };
}