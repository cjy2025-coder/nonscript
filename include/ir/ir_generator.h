#pragma once
#include "ir.h"
#include "../frontend/ast.h"
#include "../frontend/semantic.h"
#include <unordered_map>
#include <unordered_set>

namespace ns
{
    // Name Mangling: 将函数名和参数类型编码为唯一的IR符号名
    inline std::string mangle_name(const std::string &func_name, const std::vector<_type *> &param_types)
    {
        if (param_types.empty()) {
            return func_name + "_void";
        }
        std::string mangled = func_name;
        for (auto *pt : param_types) {
            mangled += "_" + pt->alias;
        }
        return mangled;
    }

    // 从FuncDecl获取参数类型列表
    inline std::vector<_type *> get_param_types_from_func(FuncDecl *func)
    {
        std::vector<_type *> types;
        for (auto &p : func->getParams()) {
            types.push_back(typeManager::find(p->name->getType())->baseType);
        }
        return types;
    }

    class IrGenerator
    {
    private:
        int temp_counter = 0;
        int label_counter = 0;
        FuncIR current_func;
        ProgramIR program_ir;
        std::unordered_map<std::string, int> var_offsets;  // 变量名 -> 栈偏移
        int next_offset = -8;

        // 类字段信息：类名 -> (字段名 -> 字段偏移)
        std::unordered_map<std::string, std::unordered_map<std::string, int>> class_fields;
        // 类字段默认值：类名 -> (字段名 -> 初始值表达式)
        std::unordered_map<std::string, std::unordered_map<std::string, Expression*>> class_field_inits;
        // 类字段类型：类名 -> (字段名 -> 类型字符串，如"string")
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> class_field_types;
        // 当前方法所在类（用于解析字段引用）
        std::string current_class_name;
        // 当前类已知的所有字段名
        std::unordered_set<std::string> current_class_field_names;

    private:
        std::string new_temp();
        std::string new_label();
        int getOrAllocOffset(const std::string &varname);
        void emit(Op op, const std::string &result, const std::string &arg1 = "", const std::string &arg2 = "", IRType type = IRType::INT64);
        void emit_label(const std::string &label);

        // 表达式访问
        std::string visit(Expression *expr);
        std::string visit_infix(InfixExpression *expr);
        std::string visit_prefix(PrefixExpression *expr);
        std::string visit_call(CallExpression *expr);
        std::string visit_if(IfExpression *expr);
        std::string visit_while(WhileExpression *expr);
        std::string visit_until(UntilExpression *expr);
        std::string visit_switch(SwitchExpression *expr);
        std::string visit_lambda(LambdaExpr *expr);
        std::string visit_new(NewExpression *expr);
        std::string visit_index(IndexExpression *expr);
        std::string visit_array(ArrayLiteral *expr);

        // 语句访问
        void visit_statement(Statement *stmt);
        void visit_declare(DeclareStatement *stmt);
        void visit_expression_stmt(ExpressionStatement *stmt);
        void visit_block(BlockStatement *stmt);
        void visit_func(FuncDecl *stmt);
        void visit_class(ClassLiteral *stmt);
        void visit_ret(ReturnStatement *stmt);
        void visit_for(ForLoop *stmt);
        void visit_try(TryCatchStatement *stmt);

        std::string get_ir_type_str(Expression *expr);

        // 收集类及其父类的所有字段（递归）
        void collect_class_fields(ClassLiteral *stmt);

    public:
        IrGenerator() = default;
        ProgramIR generate(Program *program);
    };
}