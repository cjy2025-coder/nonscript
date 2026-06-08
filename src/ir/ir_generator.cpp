#include "./ir/ir_generator.h"
#include "util/util.h"
namespace ns
{
    std::string IrGenerator::new_temp()
    {
        return "t" + std::to_string(++temp_counter);
    }

    std::string IrGenerator::new_label()
    {
        return ".L" + std::to_string(++label_counter);
    }

    int IrGenerator::getOrAllocOffset(const std::string &varname)
    {
        auto it = var_offsets.find(varname);
        if (it != var_offsets.end())
            return it->second;
        int off = next_offset;
        var_offsets[varname] = off;
        next_offset -= 8;
        return off;
    }

    void IrGenerator::emit(Op op, const std::string &result, const std::string &arg1, const std::string &arg2, IRType type)
    {
        TAC tac;
        tac.op = op;
        tac.result = result;
        tac.arg1 = arg1;
        tac.arg2 = arg2;
        tac.type = type;
        current_func.instructions.push_back(tac);
    }

    void IrGenerator::emit_label(const std::string &label)
    {
        TAC tac;
        tac.op = Op::LABEL;
        tac.label = label;
        current_func.instructions.push_back(tac);
    }

    // ==================== 表达式 ====================

    std::string IrGenerator::visit(Expression *expr)
    {
        if (!expr) return "";

        if (auto *e = dynamic_cast<I8Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, e->getLiteral(), "", IRType::INT8);
            return t;
        }
        if (auto *e = dynamic_cast<I16Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, e->getLiteral(), "", IRType::INT16);
            return t;
        }
        if (auto *e = dynamic_cast<I32Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, e->getLiteral(), "", IRType::INT32);
            return t;
        }
        if (auto *e = dynamic_cast<I64Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, e->getLiteral(), "", IRType::INT64);
            return t;
        }
        if (auto *e = dynamic_cast<Float32Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, std::to_string(e->getValue()), "", IRType::FLOAT32);
            return t;
        }
        if (auto *e = dynamic_cast<Float64Literal *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, std::to_string(e->getValue()), "", IRType::FLOAT64);
            return t;
        }
        if (auto *e = dynamic_cast<BoolLiteral *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, (e->getLiteral() == "true") ? "1" : "0", "", IRType::BOOLEAN);
            return t;
        }
        if (auto *e = dynamic_cast<StringLiteral *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, "\"" + escapeString(e->getLiteral()) + "\"", "", IRType::STRING);
            return t;
        }
        if (auto *e = dynamic_cast<NullLiteral *>(expr))
        {
            std::string t = new_temp();
            emit(Op::LDC, t, "0", "", IRType::PTR);
            return t;
        }

        if (auto *e = dynamic_cast<Ident *>(expr))
        {
            std::string name = e->getLiteral();
            // 检查是否为当前类的字段（包括继承字段）
            if (!current_class_name.empty() && current_class_field_names.count(name))
            {
                // 通过 this 指针访问字段
                std::string t = new_temp();
                // 在所有已收集的类字段中查找
                int field_idx = 0;
                for (auto &[cls, fields] : class_fields) {
                    auto it = fields.find(name);
                    if (it != fields.end()) {
                        field_idx = it->second;
                        break;
                    }
                }
                // 生成 INDEX: this[field_idx] → t
                emit(Op::INDEX, t, "this", std::to_string(field_idx));
                return t;
            }
            return name;
        }

        if (auto *e = dynamic_cast<PrefixExpression *>(expr))
            return visit_prefix(e);

        if (auto *e = dynamic_cast<InfixExpression *>(expr))
            return visit_infix(e);

        if (auto *e = dynamic_cast<CallExpression *>(expr))
            return visit_call(e);

        if (auto *e = dynamic_cast<IfExpression *>(expr))
            return visit_if(e);
        if (auto *e = dynamic_cast<UntilExpression *>(expr))
            return visit_until(e);
        if (auto *e = dynamic_cast<WhileExpression *>(expr))
            return visit_while(e);
        if (auto *e = dynamic_cast<SwitchExpression *>(expr))
            return visit_switch(e);

        if (auto *e = dynamic_cast<LambdaExpr *>(expr))
            return visit_lambda(e);

        if (auto *e = dynamic_cast<NewExpression *>(expr))
            return visit_new(e);

        if (auto *e = dynamic_cast<IndexExpression *>(expr))
            return visit_index(e);

        if (auto *e = dynamic_cast<ArrayLiteral *>(expr))
            return visit_array(e);

        if (auto *e = dynamic_cast<ThisExpr *>(expr))
            return "this";

        return "";
    }

    std::string IrGenerator::visit_infix(InfixExpression *expr)
    {
        std::string op = expr->getOperator();

        // 成员访问：将字段名解析为数值索引
        if (op == "." || op == "->")
        {
            std::string left = visit(expr->getLeft());
            std::string field_name = expr->getRight()->getLiteral();
            std::string t = new_temp();
            // 在所有已收集的类字段中查找该字段的索引
            std::string index_val = field_name;
            for (auto &[cls, fields] : class_fields) {
                auto it = fields.find(field_name);
                if (it != fields.end()) {
                    index_val = std::to_string(it->second);
                    break;
                }
            }
            emit(Op::INDEX, t, left, index_val);
            return t;
        }

        std::string left = visit(expr->getLeft());
        std::string right = visit(expr->getRight());
        std::string t = new_temp();

        if (op == "=")
        {
            // 检查左侧是否为当前类的字段（需要写回堆内存）
            if (auto *left_ident = dynamic_cast<Ident *>(expr->getLeft()))
            {
                std::string field_name = left_ident->getLiteral();
                if (!current_class_name.empty() && current_class_field_names.count(field_name))
                {
                    // 是类字段赋值：生成 STORE_INDEX 写回堆内存
                    int field_idx = 0;
                    for (auto &[cls, fields] : class_fields) {
                        auto it = fields.find(field_name);
                        if (it != fields.end()) {
                            field_idx = it->second;
                            break;
                        }
                    }
                    emit(Op::STORE_INDEX, right, "this", std::to_string(field_idx));
                    std::string t = new_temp();
                    emit(Op::MOV, t, right);
                    return t;
                }
            }
            emit(Op::MOV, left, right);
            return left;
        }

        Op opcode;
        if (op == "+") opcode = Op::ADD;
        else if (op == "-") opcode = Op::SUB;
        else if (op == "*") opcode = Op::MUL;
        else if (op == "/") opcode = Op::DIV;
        else if (op == "%") opcode = Op::MOD;
        else if (op == "&&") opcode = Op::AND;
        else if (op == "||") opcode = Op::OR;
        else if (op == "==") opcode = Op::CMP_EQ;
        else if (op == "!=") opcode = Op::CMP_NE;
        else if (op == "<")  opcode = Op::CMP_LT;
        else if (op == ">")  opcode = Op::CMP_GT;
        else if (op == "<=") opcode = Op::CMP_LE;
        else if (op == ">=") opcode = Op::CMP_GE;
        else return "";

        emit(opcode, t, left, right);
        return t;
    }

    // 辅助函数：推断表达式的类型字符串（前向声明）
    static std::string infer_expr_type(Expression *expr);

    std::string IrGenerator::visit_prefix(PrefixExpression *expr)
    {
        std::string op = expr->getOperator();
        std::string right = visit(expr->getRight());
        std::string t = new_temp();

        if (op == "-")
            emit(Op::NEG, t, right);
        else if (op == "!" || op == "not")
            emit(Op::NOT, t, right);
        else
            emit(Op::MOV, t, right);

        return t;
    }

    std::string IrGenerator::visit_call(CallExpression *expr)
    {
        auto *func_expr = expr->getFunc();
        std::string func_name;
        bool is_method_call = false;
        std::string this_var;

        // 检查是否为方法调用: obj.method() 或 obj->method()
        if (auto *infix = dynamic_cast<InfixExpression *>(func_expr))
        {
            if (infix->getOperator() == "." || infix->getOperator() == "->")
            {
                is_method_call = true;
                // 生成 this 指针
                this_var = visit(infix->getLeft());
                // 获取方法名
                std::string raw_method_name = infix->getRight()->getLiteral();
                func_name = raw_method_name;
                
                // 从左侧对象类型推断完整的 class.method_param 名称
                auto *left_expr = infix->getLeft();
                std::string obj_type;
                if (auto *left_ident = dynamic_cast<Ident *>(left_expr)) {
                    obj_type = left_ident->getType();
                }
                if (!obj_type.empty()) {
                    // 收集调用参数类型
                    std::vector<_type *> call_param_types;
                    for (auto &arg : expr->getArgs()) {
                        // 从语义类型信息获取
                        std::string arg_type_name = arg->getType();
                        if (!arg_type_name.empty()) {
                            TypeInfo *ti = typeManager::find(arg_type_name);
                            if (ti) call_param_types.push_back(ti->baseType);
                        }
                    }
                    func_name = obj_type + "." + mangle_name(raw_method_name, call_param_types);
                }
            }
        }
        else if (auto *ident = dynamic_cast<Ident *>(func_expr))
        {
            std::string raw_name = ident->getLiteral();
            // 内置函数不进行 Name Mangling
            if (raw_name == "print" || raw_name == "scan") {
                func_name = raw_name;
            } else {
                // 对于直接函数调用，使用 mangled name
                std::vector<_type *> call_param_types;
                for (auto &arg : expr->getArgs()) {
                    std::string arg_type_name = infer_expr_type(arg.get());
                    if (!arg_type_name.empty()) {
                        TypeInfo *ti = typeManager::find(arg_type_name);
                        if (ti) call_param_types.push_back(ti->baseType);
                    }
                }
                func_name = mangle_name(raw_name, call_param_types);
            }
        }

        auto &args = expr->getArgs();

        // 内置函数 print
        if (func_name == "print")
        {
            for (auto &arg : args)
            {
                // 检查是否为数组字面量：展开逐个元素打印
                if (auto *arr_lit = dynamic_cast<ArrayLiteral *>(arg.get()))
                {
                    for (auto &elem : arr_lit->getElems())
                    {
                        std::string e = visit(elem.get());
                        bool is_str_elem = (dynamic_cast<StringLiteral *>(elem.get()) != nullptr);
                        if (!is_str_elem) {
                            std::string type_name = elem->getType();
                            is_str_elem = (type_name == "string");
                        }
                        emit(Op::PRINT, "", e, is_str_elem ? "str" : "int");
                    }
                }
                else
                {
                    std::string a = visit(arg.get());
                    // 检查参数类型：如果是字符串字面量或表达式类型为 string
                    bool is_str = (dynamic_cast<StringLiteral *>(arg.get()) != nullptr);
                    if (!is_str) {
                        std::string type_name = arg->getType();
                        is_str = (type_name == "string");
                    }
                    // 额外检查：如果是类字段 Ident，通过字段类型映射判断
                    if (!is_str) {
                        if (auto *ident_arg = dynamic_cast<Ident *>(arg.get())) {
                            std::string field_name = ident_arg->getLiteral();
                            // 在所有类中查找该字段的类型
                            for (auto &[cls, fields] : class_field_types) {
                                auto it = fields.find(field_name);
                                if (it != fields.end()) {
                                    is_str = (it->second == "string");
                                    break;
                                }
                            }
                        }
                    }
                    emit(Op::PRINT, "", a, is_str ? "str" : "int");
                }
            }
            std::string t = new_temp();
            emit(Op::LDC, t, "0");
            return t;
        }

        // 内置函数 scan
        if (func_name == "scan")
        {
            for (auto &arg : args)
            {
                if (auto *ident = dynamic_cast<Ident *>(arg.get()))
                {
                    emit(Op::SCAN, ident->getLiteral());
                }
            }
            std::string t = new_temp();
            emit(Op::LDC, t, "0");
            return t;
        }

        // 普通函数/方法调用
        if (is_method_call)
        {
            // 方法调用：先传递 this 指针
            emit(Op::PARAM, this_var);
        }

        for (auto &arg : args)
        {
            std::string a = visit(arg.get());
            emit(Op::PARAM, a);
        }

        // 如果没有函数名，尝试从左侧表达式类型推断
        if (func_name.empty() && is_method_call)
        {
            func_name = "unknown_method";
        }

        std::string t = new_temp();
        emit(Op::CALL, t, func_name);
        return t;
    }

    std::string IrGenerator::visit_if(IfExpression *expr)
    {
        std::string cond = visit(expr->getCondition());
        std::string else_label = new_label();
        std::string end_label = new_label();

        emit(Op::JE, "", cond, else_label);
        visit_block(expr->getConsequence());
        emit(Op::JMP, "", end_label);
        emit_label(else_label);
        if (expr->getAlternative())
            visit_block(expr->getAlternative());
        emit_label(end_label);

        std::string t = new_temp();
        emit(Op::LDC, t, "0");
        return t;
    }

    std::string IrGenerator::visit_while(WhileExpression *expr)
    {
        std::string begin_label = new_label();
        std::string end_label = new_label();

        emit_label(begin_label);
        std::string cond = visit(expr->getCondition());
        emit(Op::JE, "", cond, end_label);
        visit_block(expr->getBody());
        emit(Op::JMP, "", begin_label);
        emit_label(end_label);

        std::string t = new_temp();
        emit(Op::LDC, t, "0");
        return t;
    }

    std::string IrGenerator::visit_until(UntilExpression *expr)
    {
        std::string begin_label = new_label();
        std::string end_label = new_label();

        emit_label(begin_label);
        visit_block(expr->getBody());
        std::string cond = visit(expr->getCondition());
        emit(Op::JE, "", cond, begin_label);
        emit_label(end_label);

        std::string t = new_temp();
        emit(Op::LDC, t, "0");
        return t;
    }

    std::string IrGenerator::visit_switch(SwitchExpression *expr)
    {
        std::string cond = visit(expr->getExpr());
        std::string end_label = new_label();

        for (auto &cs : expr->getCases())
        {
            std::string case_val = visit(cs->mcase.get());
            std::string next_label = new_label();

            // 比较：if (cond != case_val) goto next
            std::string cmp_t = new_temp();
            emit(Op::CMP_NE, cmp_t, cond, case_val);
            emit(Op::JNE, "", cmp_t, next_label);

            visit_block(cs->mbody.get());
            emit(Op::JMP, "", end_label);
            emit_label(next_label);
        }

        if (expr->getDefaultCase())
            visit_block(expr->getDefaultCase()->mbody.get());

        emit_label(end_label);

        std::string t = new_temp();
        emit(Op::LDC, t, "0");
        return t;
    }

    std::string IrGenerator::visit_lambda(LambdaExpr *expr)
    {
        std::string lambda_name = "lambda_" + std::to_string(++label_counter);

        // 保存当前函数上下文
        FuncIR saved_func = current_func;
        auto saved_offsets = var_offsets;
        int saved_next = next_offset;

        current_func = FuncIR();
        current_func.name = lambda_name;
        var_offsets.clear();
        next_offset = -8;

        // 处理参数
        const auto &params = expr->getParams();
        current_func.param_count = params.size();
        for (auto &p : params)
        {
            std::string pname = p->name->getLiteral();
            int off = getOrAllocOffset(pname);
            var_offsets[pname] = off;
        }

        visit_block(expr->getBody());
        program_ir.functions.push_back(current_func);

        // 恢复上下文
        current_func = saved_func;
        var_offsets = saved_offsets;
        next_offset = saved_next;

        std::string t = new_temp();
        emit(Op::LDC, t, lambda_name, "", IRType::PTR);
        return t;
    }

    // 辅助函数：推断表达式的类型字符串
    static std::string infer_expr_type(Expression *expr) {
        if (dynamic_cast<StringLiteral*>(expr)) return "string";
        if (dynamic_cast<I8Literal*>(expr)) return "i8";
        if (dynamic_cast<I16Literal*>(expr)) return "i16";
        if (dynamic_cast<I32Literal*>(expr)) return "i32";
        if (dynamic_cast<I64Literal*>(expr)) return "i64";
        if (dynamic_cast<Float32Literal*>(expr)) return "f32";
        if (dynamic_cast<Float64Literal*>(expr)) return "f64";
        if (dynamic_cast<BoolLiteral*>(expr)) return "bool";
        if (dynamic_cast<NullLiteral*>(expr)) return "void";
        std::string t = expr->getType();
        if (!t.empty()) return t;
        return "";
    }

    std::string IrGenerator::visit_new(NewExpression *expr)
    {
        auto *right = expr->getRight();
        std::string class_name;

        if (auto *ident = dynamic_cast<Ident *>(right))
            class_name = ident->getLiteral();
        else if (auto *call = dynamic_cast<CallExpression *>(right))
        {
            if (auto *ident = dynamic_cast<Ident *>(call->getFunc()))
                class_name = ident->getLiteral();
        }

        std::string t = new_temp();
        if (!class_name.empty())
        {
            emit(Op::ALLOC, t, class_name);
            // 初始化所有有默认值的字段
            if (class_field_inits.count(class_name)) {
                for (auto &[fname, init_expr] : class_field_inits[class_name]) {
                    if (init_expr) {
                        std::string val = visit(init_expr);
                        int field_idx = 0;
                        if (class_fields[class_name].count(fname))
                            field_idx = class_fields[class_name][fname];
                        emit(Op::STORE_INDEX, val, t, std::to_string(field_idx));
                    }
                }
            }
            
            // 如果有构造函数参数，调用构造函数
            if (auto *call = dynamic_cast<CallExpression *>(right))
            {
                auto &args = call->getArgs();
                if (!args.empty())
                {
                    // 收集参数类型以构造 mangled name: class.__constructor__paramtype
                    std::vector<_type *> ctor_param_types;
                    for (auto &arg : args) {
                        std::string arg_type_name = infer_expr_type(arg.get());
                        if (!arg_type_name.empty()) {
                            TypeInfo *ti = typeManager::find(arg_type_name);
                            if (ti) ctor_param_types.push_back(ti->baseType);
                        }
                    }
                    std::string mangled_ctor = class_name + "." + mangle_name("__constructor__", ctor_param_types);
                    // 先传 this 指针
                    emit(Op::PARAM, t);
                    // 传参数
                    for (auto &arg : args)
                    {
                        std::string a = visit(arg.get());
                        emit(Op::PARAM, a);
                    }
                    std::string result = new_temp();
                    emit(Op::CALL, result, mangled_ctor);
                }
            }
        }
        else
            emit(Op::LDC, t, "0", "", IRType::PTR);
        return t;
    }

    std::string IrGenerator::visit_index(IndexExpression *expr)
    {
        std::string left = visit(expr->getLeft());
        std::string idx = visit(expr->getIndex());
        std::string t = new_temp();
        emit(Op::INDEX, t, left, idx);
        return t;
    }

    std::string IrGenerator::visit_array(ArrayLiteral *expr)
    {
        std::string t = new_temp();

        for (auto &elem : expr->getElems())
        {
            std::string e = visit(elem.get());
            emit(Op::PARAM, e);
        }

        emit(Op::ALLOC, t, "array");
        return t;
    }

    // ==================== 语句 ====================

    void IrGenerator::visit_statement(Statement *stmt)
    {
        if (!stmt) return;

        if (auto *s = dynamic_cast<DeclareStatement *>(stmt))
            visit_declare(s);
        else if (auto *s = dynamic_cast<ExpressionStatement *>(stmt))
            visit_expression_stmt(s);
        else if (auto *s = dynamic_cast<BlockStatement *>(stmt))
            visit_block(s);
        else if (auto *s = dynamic_cast<FuncDecl *>(stmt))
            visit_func(s);
        else if (auto *s = dynamic_cast<ClassLiteral *>(stmt))
            visit_class(s);
        else if (auto *s = dynamic_cast<ReturnStatement *>(stmt))
            visit_ret(s);
        else if (auto *s = dynamic_cast<ForLoop *>(stmt))
            visit_for(s);
        else if (auto *s = dynamic_cast<TryCatchStatement *>(stmt))
            visit_try(s);
        // ShortStatement (break/continue) - 暂跳过
    }

    void IrGenerator::visit_declare(DeclareStatement *stmt)
    {
        for (auto &var : stmt->getVars())
        {
            std::string varname = var.first->getLiteral();
            getOrAllocOffset(varname);

            if (var.second)
            {
                std::string src = visit(var.second);
                emit(Op::MOV, varname, src);
            }
        }
    }

    void IrGenerator::visit_expression_stmt(ExpressionStatement *stmt)
    {
        if (stmt && stmt->expression())
            visit(stmt->expression());
    }

    void IrGenerator::visit_block(BlockStatement *stmt)
    {
        if (!stmt) return;
        for (auto &s : stmt->value())
            visit_statement(s);
    }

    void IrGenerator::visit_func(FuncDecl *stmt)
    {
        std::string func_name = stmt->getLiteral();
        std::string mangled_name = func_name;

        // 如果不是构造函数且参数不为空，使用 mangled name
        if (func_name != "__constructor__")
        {
            auto param_types = get_param_types_from_func(stmt);
            mangled_name = mangle_name(func_name, param_types);
        }

        // 保存当前函数上下文
        FuncIR saved_func = current_func;
        auto saved_offsets = var_offsets;
        int saved_next = next_offset;

        current_func = FuncIR();
        current_func.name = mangled_name;
        var_offsets.clear();
        next_offset = -8;

        // 处理参数
        const auto &params = stmt->getParams();
        current_func.param_count = params.size();
        for (const auto &p : params)
        {
            std::string pname = p->name->getLiteral();
            current_func.params.push_back(pname);
        }

        visit_block(stmt->getBody());

        program_ir.functions.push_back(current_func);

        // 恢复
        current_func = saved_func;
        var_offsets = saved_offsets;
        next_offset = saved_next;
    }

    void IrGenerator::collect_class_fields(ClassLiteral *stmt)
    {
        std::string class_name = stmt->getLiteral();
        int field_idx = 0;

        // 先包含父类的字段（继承的字段在索引上排在前面）
        for (auto &base : stmt->getBaseClasses())
        {
            std::string parent_name = base->getLiteral();
            if (class_fields.count(parent_name))
            {
                for (auto &[fname, idx] : class_fields[parent_name])
                {
                    class_fields[class_name][fname] = field_idx;
                    // 从父类复制字段类型
                    if (class_field_types.count(parent_name) && class_field_types[parent_name].count(fname))
                    {
                        class_field_types[class_name][fname] = class_field_types[parent_name][fname];
                    }
                    // 从父类复制字段初始值表达式
                    if (class_field_inits.count(parent_name) && class_field_inits[parent_name].count(fname))
                    {
                        class_field_inits[class_name][fname] = class_field_inits[parent_name][fname];
                    }
                    field_idx++;
                }
            }
        }

        // 再添加当前类的字段
        for (auto &mem : stmt->getMembers())
        {
            if (mem.type == MemberType::Field)
            {
                if (auto *decl = dynamic_cast<DeclareStatement *>(mem.declaration.get()))
                {
                    for (auto &var : decl->getVars())
                    {
                        std::string field_name = var.first->getLiteral();
                        // 避免重复添加继承来的字段
                        if (class_fields[class_name].count(field_name))
                            continue;
                        class_fields[class_name][field_name] = field_idx;
                        // 记录字段类型
                        class_field_types[class_name][field_name] = var.first->getType();
                        // 存储字段的初始值表达式（如果有）
                        if (var.second) {
                            class_field_inits[class_name][field_name] = var.second;
                        }
                        field_idx++;
                    }
                }
            }
        }

        // 记录类大小
        int total_fields = (int)class_fields[class_name].size();
        program_ir.class_sizes[class_name] = total_fields * 8;
        if (program_ir.class_sizes[class_name] < 8)
            program_ir.class_sizes[class_name] = 8;
    }

    void IrGenerator::visit_class(ClassLiteral *stmt)
    {
        std::string class_name = stmt->getLiteral();

        // 收集该类的字段信息
        collect_class_fields(stmt);

        // 构建该类的字段集合（包括所有类的字段，用于支持继承）
        std::unordered_set<std::string> all_field_names;
        for (auto &[cls, fields] : class_fields) {
            for (auto &[fname, _] : fields) {
                all_field_names.insert(fname);
            }
        }

        // 为类方法生成函数
        for (auto &mem : stmt->getMembers())
        {
            if (mem.type == MemberType::Method)
            {
                if (auto *func = dynamic_cast<FuncDecl *>(mem.declaration.get()))
                {
                    std::string method_name = func->getLiteral();
                    
                    // 类名.方法名_参数类型组合，避免重载冲突
                    auto param_types = get_param_types_from_func(func);
                    method_name = class_name + "." + mangle_name(method_name, param_types);
                    FuncIR saved_func = current_func;
                    auto saved_offsets = var_offsets;
                    int saved_next = next_offset;
                    std::string saved_class_name = current_class_name;
                    auto saved_field_names = current_class_field_names;

                    current_func = FuncIR();
                    current_func.name = method_name;
                    var_offsets.clear();
                    next_offset = -8;

                    // 设置当前类上下文（用于字段解析）
                    current_class_name = class_name;
                    current_class_field_names = all_field_names;

                    // this 作为第一个参数
                    getOrAllocOffset("this");
                    current_func.params.push_back("this");

                    const auto &params = func->getParams();
                    current_func.param_count = params.size() + 1;
                    for (const auto &p : params)
                    {
                        std::string pname = p->name->getLiteral();
                        current_func.params.push_back(pname);
                        getOrAllocOffset(pname);
                    }

                    visit_block(func->getBody());
                    program_ir.functions.push_back(current_func);

                    current_func = saved_func;
                    var_offsets = saved_offsets;
                    next_offset = saved_next;
                    current_class_name = saved_class_name;
                    current_class_field_names = saved_field_names;
                }
            }
        }
    }

    void IrGenerator::visit_ret(ReturnStatement *stmt)
    {
        if (stmt->getExpression())
        {
            std::string val = visit(stmt->getExpression());
            emit(Op::RET, "", val);
        }
        else
        {
            emit(Op::RET, "");
        }
    }

    void IrGenerator::visit_for(ForLoop *stmt)
    {
        std::string begin_label = new_label();
        std::string end_label = new_label();

        // 初始化变量
        for (auto &var : stmt->getVariables())
        {
            if (var->value)
            {
                std::string val = visit(var->value.get());
                if (var->var)
                {
                    std::string varname = var->var->getLiteral();
                    getOrAllocOffset(varname);
                    emit(Op::MOV, varname, val);
                }
            }
        }

        emit_label(begin_label);
        if (stmt->getCondtion())
        {
            std::string cond = visit(stmt->getCondtion());
            emit(Op::JE, "", cond, end_label);
        }
        visit_block(stmt->getBody());
        // actions
        for (auto &action : stmt->getActions())
            visit(action.get());
        emit(Op::JMP, "", begin_label);
        emit_label(end_label);
    }

    void IrGenerator::visit_try(TryCatchStatement *stmt)
    {
        visit_block(stmt->getTryBody());
    }

    // ==================== 入口 ====================

    ProgramIR IrGenerator::generate(Program *program)
    {
        program_ir = ProgramIR();

        // 创建主函数 IR
        current_func = FuncIR();
        current_func.name = "main";
        var_offsets.clear();
        next_offset = -8;

        // 处理所有顶层语句
        for (auto &stmt : program->stmts)
            visit_statement(stmt);

        // 确保 main 函数有返回
        if (current_func.instructions.empty() ||
            current_func.instructions.back().op != Op::RET)
        {
            emit(Op::LDC, new_temp(), "0");
            emit(Op::RET, "");
        }

        program_ir.functions.push_back(current_func);

        return program_ir;
    }
}