#include "./frontend/semantic.h"

namespace ns
{
    TypeInfo *_errorType = new TypeInfo();
    void SemanticAnalyzer::push_symbol(std::string name, Symbol *symbol)
    {
        symbol_table[name] = symbol; //?
    }
    bool SemanticAnalyzer::is_arithmetic_op(std::string op)
    {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "%";
    }
    bool SemanticAnalyzer::is_logic_op(std::string op)
    {
        return op == "&&" || op == "||";
    }
    bool SemanticAnalyzer::is_int(_type *type)
    {
        return type->alias == "i8" || type->alias == "i16" || type->alias == "i32" || type->alias == "i64";
    }
    bool SemanticAnalyzer::is_float(_type *type)
    {
        return type->alias == "f32" || type->alias == "f64";
    }
    bool SemanticAnalyzer::is_number(_type *type)
    {
        return is_float(type) || is_int(type);
    }
    bool SemanticAnalyzer::is_string(_type *type)
    {
        return type->alias == "string";
    }
    bool SemanticAnalyzer::is_bool(_type *type)
    {
        return type->alias == "bool";
    }
    TypeInfo *SemanticAnalyzer::get_wider_numeric_type(TypeInfo *left, TypeInfo *right)
    {
        static std::map<std::string, int> type_precedence = {
            {"i8", 1}, {"i16", 2}, {"i32", 3}, {"i64", 4}, {"f32", 5}, {"f64", 6}};
        int left_prio = type_precedence[left->baseType->alias];
        int right_prio = type_precedence[right->baseType->alias];
        return left_prio > right_prio ? left : right;
    }
    ErrorManager *SemanticAnalyzer::what()
    {
        return errorManager;
    }
    TypeInfo *createTypeInfo(std::string alias, typeManager *_typeManager)
    {
        TypeInfo *ret = new TypeInfo();
        ret->baseType = _typeManager->find(alias);
        return ret;
    }
    bool SemanticAnalyzer::exit_scope()
    {
        if (--current_scope <= 0)
            return false;
        std::vector<std::string> to_remove;
        for (const auto &entry : symbol_table)
        {
            if (entry.second->scope_level > current_scope)
            {
                to_remove.push_back(entry.first);
            }
        }
        for (const auto &name : to_remove)
        {
            symbol_table.erase(name);
        }
        return true;
    }
    Symbol *SemanticAnalyzer::find_symbol(const std::string &name)
    {
        auto it = symbol_table.find(name);
        if (it == symbol_table.end())
        {

            return nullptr;
        }
        if (it->second->scope_level > current_scope)
        {
            return nullptr;
        }
        return it->second;
    }
    std::string type_to_string(_type *type)
    {
        return type ? type->alias : "";
    }
    _type *string_to_type(std::string type, typeManager *manager)
    {
        return manager->find(type);
    }
    TypeInfo *SemanticAnalyzer::check_ident(Ident *ident)
    {
        auto symbol = find_symbol(ident->getLiteral());
        if (!symbol || symbol->scope_level > current_scope) // 没有找到这个标识符
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "undefined symbol: " + ident->getLiteral(), ident->getToken().getLocation());
            _MARK_ERROR
        }
        return symbol->type_info;
    }
    TypeInfo *SemanticAnalyzer::check_prefix_expression(PrefixExpression *expr)
    {
        auto expression = expr->getRight();
        auto expression_type = check_expression(expression);
        if (!expression_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expression->getLiteral(), expression->getToken().getLocation());
            _MARK_ERROR
        }
        else if (expression_type == _errorType)
        {
            return expression_type;
        }
        auto op = expr->getOperator();
        if ((op == "+" || op == "-") && is_number(expression_type->baseType))
        {
            return expression_type;
        }
        else if (op == "!" && is_bool(expression_type->baseType))
        {
            return expression_type;
        }
        else if (op == "new")
        {
        }
        raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expression->getLiteral(), expression->getToken().getLocation());
        _MARK_ERROR
    }
    TypeInfo *SemanticAnalyzer::check_infix_expression(InfixExpression *expr)
    {
        auto left = expr->getLeft();
        auto left_type = check_expression(left); // 计算左侧表达式
        bool hasError = false;
        if (!left_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expr->getLeft()->toString(), expr->getLeft()->getToken().getLocation());
            hasError = true;
        }
        else if (left_type == _errorType)
        {
            hasError = true;
        }
        auto right = expr->getRight();
        auto op = expr->getOperator();
        if (op == ".") // 优先判断是否在访问类成员
        {
            auto t = symbol_table.find(left_type->baseType->alias);
            auto t_ = std::get<ClassDetail>(t->second->type_info->detail);
            auto _ = t_.mems.find(right->getLiteral());
            if (_ == t_.mems.end())
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unkown member : " + right->getLiteral() + " of class " + left_type->baseType->alias,
                           right->getToken().getLocation());
                _MARK_ERROR
            }
            else if (_->second.level != AccessLevel::Public)
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "invisiable member : " + right->getLiteral() + " of class " + left_type->baseType->alias,
                           right->getToken().getLocation());
                _MARK_ERROR
            }
            else
            {
                return _->second.ti;
            }
        }
        auto right_type = check_expression(right); // 计算右侧表达式
        if (!right_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expr->getRight()->toString(), expr->getRight()->getToken().getLocation());
            hasError = true;
        }
        else if (right_type == _errorType)
        {
            hasError = true;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        if (op == "=")
        { // 是赋值语句
            if (left_type->baseType != right_type->baseType &&
                !(is_number(left_type->baseType) && is_number(right_type->baseType)))
            {
                hasError = true;
                raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type for assignment statements: " + expr->toString(), expr->getToken().getLocation());
            }
            else
            {
                return createTypeInfo(left_type->baseType->alias, types);
            }
        }
        else if (is_arithmetic_op(op))
        {
            if (is_number(left_type->baseType) && is_number(right_type->baseType))
            {
                if (op == "%")
                {
                    if (is_float(left_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + left->getLiteral(), left->getToken().getLocation());
                        _MARK_ERROR
                    }
                    if (is_float(right_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + right->getLiteral(), right->getToken().getLocation());
                        _MARK_ERROR
                    }
                }
                return get_wider_numeric_type(left_type, right_type);
            }
            else if (op == "+")
            { // 处理字符串加法
                if (is_string(left_type->baseType) && is_string(right_type->baseType))
                {
                    return createTypeInfo("string", types);
                }
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unsupported operation + between ",
                           right->getToken().getLocation());
                _MARK_ERROR
            }
        }
        else if (is_logic_op(op))
        {
            if (is_bool(left_type->baseType) && is_bool(right_type->baseType))
            {
                return left_type;
            }
        }
        // else if(op == "."){
        //     if(left_type->baseType == _type::OBJECT)
        // }
        else
            _MARK_ERROR
    }
    TypeInfo *SemanticAnalyzer::check_index_expression(IndexExpression *expr)
    {
        auto array = expr->getLeft();
        auto type = check_expression(array);
        if (!type || type == _errorType)
        {
            _MARK_ERROR
        }
        else if (type->baseType->alias != "array")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect an array instead of got type: " + type_to_string(type->baseType),
                       expr->getToken().getLocation());
            _MARK_ERROR
        }
        auto idx = expr->getIndex();
        auto type_ = check_expression(idx);
        if (!type_ || type == _errorType)
        {
            _MARK_ERROR
        }
        else if (type_->baseType->alias != "i8" && type_->baseType->alias != "i16" &&
                 type_->baseType->alias != "i32" && type_->baseType->alias != "i64")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect an integer as index instead of got type: " + type_to_string(type_->baseType),
                       idx->getToken().getLocation());
            _MARK_ERROR
        }
        // 尝试获取元素的类型，以及检查下标是否越界
        // 下标是整数常量
        int64_t _ = -1;
        bool static_check = true;
        if (auto *constant_idx = dynamic_cast<I8Literal *>(idx))
        {
            _ = constant_idx->getValue();
        }
        else if (auto *constant_idx = dynamic_cast<I16Literal *>(idx))
        {
            _ = constant_idx->getValue();
        }
        else if (auto *constant_idx = dynamic_cast<I32Literal *>(idx))
        {
            _ = constant_idx->getValue();
        }
        else if (auto *constant_idx = dynamic_cast<I64Literal *>(idx))
        {
            _ = constant_idx->getValue();
        }
        else
        {
            static_check = false;
        }
        // 无法静态检查时，临时返回一个类型
        if (!static_check)
            return createTypeInfo("undefined", types);
        ArrayDetail arrayDetail = std::get<ArrayDetail>(type->detail);
        // 对下标进行静态检查
        if (_ < 0 || _ >= arrayDetail.size)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "array index " + std::to_string(_) +
                           " out of bounds [0, " + std::to_string(arrayDetail.size - 1) + "]",
                       idx->getToken().getLocation());
            _MARK_ERROR
        }
        else
        {
            return arrayDetail.elems_types[_];
        }
    }
    TypeInfo *SemanticAnalyzer::check_array_literal(ArrayLiteral *arr)
    {
        TypeInfo *typeInfo = createTypeInfo("array", types);
        std::vector<TypeInfo *> elemTypes = {};
        int size = 0;
        std::vector<std::unique_ptr<Expression>> &elements = arr->getElems();
        size = elements.size();
        bool hasError = false;
        for (int i = 0; i < size; i++)
        {
            auto elemType = check_expression(elements[i].get());
            if (!elemType)
            {
                hasError = true;
                continue;
            }
            elemTypes.emplace_back(elemType);
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        ArrayDetail arrayDetail = {};
        arrayDetail.size = size;             // 数组长度信息
        arrayDetail.elems_types = elemTypes; // 数组各个元素的类型
        typeInfo->detail = arrayDetail;
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_block_statement(BlockStatement *stmt)
    {
        auto stmts = stmt->value();
        TypeInfo *typeInfo = nullptr;
        bool hasError = false;
        for (auto &stmt : stmts)
        {
            typeInfo = check_statement(stmt);
            if (_SEMANTIC_ERROR(typeInfo))
            {
                hasError = true;
                continue;
            }
            if (typeid(*stmt) == typeid(ReturnStatement))
                return typeInfo;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        return createTypeInfo("void", types);
    }
    TypeInfo *SemanticAnalyzer::check_param(std::shared_ptr<FuncParam> param)
    {
        auto &ident = param->name;
        auto &val = param->init;
        if (!val)
        { // 形参无默认值
            TypeInfo *typeInfo = createTypeInfo(ident->getType(), types);
            std::string name = ident->getLiteral();
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = typeInfo;
            push_symbol(name, new_symbol);
            return typeInfo;
        }
        else // 处理形参有默认值的情况
        {
            auto val_type_info = check_expression(val.get());
            if (_SEMANTIC_ERROR(val_type_info))
                _MARK_ERROR
            TypeInfo *typeInfo = createTypeInfo(ident->getType(), types);
            if (val_type_info->baseType != typeInfo->baseType)
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type: " + ident->getType(), ident->getToken().getLocation());
                _MARK_ERROR
            }
            std::string name = ident->getLiteral();
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = typeInfo;
            push_symbol(name, new_symbol);
            return typeInfo;
        }
    }
    TypeInfo *SemanticAnalyzer::check_call_expression(CallExpression *expr)
    {
        auto func = expr->getFunc();
        auto typeInfo = check_expression(expr->getFunc());
        if (_SEMANTIC_ERROR(typeInfo))
        {
            _MARK_ERROR
        }
        else if (typeInfo->baseType->alias != "func")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect function " + func->getLiteral(),
                       func->getToken().getLocation());
            return nullptr;
        }
        
        ns::FuncDetail &func_detail = std::get<ns::FuncDetail>(typeInfo->detail);
        if(func_detail.is_unlimited_args_func){//不限制形参列表
           return createTypeInfo(func_detail.return_type->alias, types);
        }
        std::vector<std::unique_ptr<Expression>> &args = expr->getArgs();
        auto arg_types = func_detail.param_types;
        if (args.size() != arg_types.size())
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "no match arguments for function: " + func->getLiteral(),
                       func->getToken().getLocation());
            return nullptr;
        }
        for (int i = 0; i < args.size(); i++)
        {
            auto type = check_expression(args[i].get());
            if (type->baseType != arg_types[i])
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unmatch type: " + args[i]->getLiteral(),
                           args[i]->getToken().getLocation());
                return nullptr;
            }
        }
        return createTypeInfo(func_detail.return_type->alias, types);
    }
    TypeInfo *SemanticAnalyzer::check_func_statement(FuncDecl *stmt)
    {
        TypeInfo *typeInfo = createTypeInfo("func", types);
        std::string name = stmt->getLiteral();
        if (find_symbol(name))
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "redefine function: " + name,
                       stmt->getToken().getLocation());
            _MARK_ERROR
        }
        auto params = stmt->getParams();
        bool hasError = false;
        enter_scope(); // 进入函数的局部变量作用域
        FuncDetail funcDetail = {};
        for (auto &param : params)
        {
            TypeInfo *info = check_param(param);
            if (_SEMANTIC_ERROR(info))
            {
                hasError = true;
                continue;
            }
            funcDetail.param_types.push_back(info->baseType);
            // 将形参名写入符号表
            Symbol *new_symbol = new Symbol();
            new_symbol->name = param->name->getLiteral();
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = info;
            push_symbol(param->name->getLiteral(), new_symbol);
        }
        if (!stmt->check_if_extern_func()) // 不是外部的 extern 函数
        {
            auto body = stmt->getBody();
            auto t = check_block_statement(body);
            if (_SEMANTIC_ERROR(t))
            {
                hasError = true;
            }
            else if (stmt->ret_type && t->baseType->id != stmt->ret_type->id) // 声明的返回值类型和实际返回的不一致
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unmatch return type:" + t->baseType->alias + ".expect type: " + stmt->ret_type->alias,
                           stmt->getToken().getLocation());
                hasError = true;
            }

            if (hasError)
            {
                _MARK_ERROR
            }
            if (!stmt->ret_type)
            {
                stmt->ret_type = t->baseType;
            }
        }
        
        funcDetail.return_type = stmt->ret_type;
        typeInfo->detail = funcDetail;
        exit_scope(); // 离开局部变量作用域

        // 将函数名写入符号表
        Symbol *new_symbol = new Symbol();
        new_symbol->name = name;
        new_symbol->scope_level = current_scope;
        new_symbol->type_info = typeInfo;
        push_symbol(name, new_symbol);
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_lambda_expression(LambdaExpr *expr)
    {
        TypeInfo *typeInfo = createTypeInfo("func", types);
        auto params = expr->getParams();
        bool hasError = false;
        enter_scope(); // 进入函数的局部变量作用域
        FuncDetail funcDetail = {};
        for (auto &param : params)
        {
            TypeInfo *info = check_param(param);
            if (_SEMANTIC_ERROR(info))
            {
                hasError = true;
                continue;
            }
            funcDetail.param_types.push_back(info->baseType);
        }
        auto body = expr->getBody();
        auto t = check_block_statement(body);
        if (_SEMANTIC_ERROR(t))
        {
            hasError = true;
        }
        else if (expr->ret_type && t->baseType->id != expr->ret_type->id)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "unmatch return type:" + t->baseType->alias + " . expect type: " + expr->ret_type->alias,
                       expr->getToken().getLocation());
            hasError = true;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        if (!expr->ret_type)
        {
            expr->ret_type = t->baseType;
        }
        funcDetail.return_type = t->baseType;
        typeInfo->detail = funcDetail;
        exit_scope(); // 离开局部变量作用域
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_if_expression(IfExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError = false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
        {
            hasError = true;
        }
        auto consequence = expr->getConsequence();
        if (_SEMANTIC_ERROR(check_statement(consequence)))
        {
            hasError = true;
        }
        auto alternative = expr->getAlternative();
        if (alternative != NULL)
        {
            if (_SEMANTIC_ERROR(check_statement(alternative)))
            {
                hasError = true;
            }
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        return createTypeInfo("void", types);
    }
    TypeInfo *SemanticAnalyzer::check_while_expression(WhileExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError = false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
        {
            hasError = true;
        }
        auto body = expr->getBody();
        if (_SEMANTIC_ERROR(check_statement(body)))
        {
            hasError = true;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        return createTypeInfo("void", types);
    }
    TypeInfo *SemanticAnalyzer::check_expression(Expression *expr)
    {
        if (!expr)
            return nullptr;
        if (typeid(*expr) == typeid(I8Literal))
            return createTypeInfo("i8", types);
        else if (typeid(*expr) == typeid(I16Literal))
            return createTypeInfo("i16", types);
        else if (typeid(*expr) == typeid(I32Literal))
            return createTypeInfo("i32", types);
        else if (typeid(*expr) == typeid(I64Literal))
            return createTypeInfo("i64", types);
        else if (typeid(*expr) == typeid(Float32Literal))
            return createTypeInfo("f32", types);
        else if (typeid(*expr) == typeid(Float64Literal))
            return createTypeInfo("f64", types);
        else if (typeid(*expr) == typeid(NullLiteral))
            return createTypeInfo("void", types);
        else if (typeid(*expr) == typeid(BoolLiteral))
            return createTypeInfo("bool", types);
        else if (typeid(*expr) == typeid(StringLiteral))
            return createTypeInfo("string", types);
        else if (typeid(*expr) == typeid(ArrayLiteral))
            return check_array_literal((ArrayLiteral *)expr);
        else if (typeid(*expr) == typeid(LambdaExpr))
            return check_lambda_expression((LambdaExpr *)expr);
        else if (typeid(*expr) == typeid(Ident))
            return check_ident((Ident *)expr);
        else if (typeid(*expr) == typeid(PrefixExpression))
            return check_prefix_expression((PrefixExpression *)expr);
        else if (typeid(*expr) == typeid(InfixExpression))
            return check_infix_expression((InfixExpression *)expr);
        else if (typeid(*expr) == typeid(IndexExpression))
            return check_index_expression((IndexExpression *)expr);
        else if (typeid(*expr) == typeid(CallExpression))
        {
            return check_call_expression((CallExpression *)expr);
        }
        else if (typeid(*expr) == typeid(IfExpression))
        {
            return check_if_expression((IfExpression *)expr);
        }
        else if (typeid(*expr) == typeid(WhileExpression))
        {
            return check_while_expression((WhileExpression *)expr);
        }
        else if (typeid(*expr) == typeid(NewExpression))
        {
            return check_new_expression((NewExpression *)(expr));
        }
        else
            _MARK_ERROR
    }
    TypeInfo *SemanticAnalyzer::check_new_expression(NewExpression *expr)
    {
        auto temp = expr->getRight();
        if (auto *_ = dynamic_cast<Ident *>(temp))
        { // 直接调用默认构造函数
            auto type = _->getLiteral();
            auto _type = types->find(type);
            if (!_type)
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "Unkown class: " + type,
                           temp->getToken().getLocation());
                _MARK_ERROR
            }
            return createTypeInfo(type, types);
        }
        else if (auto *_ = dynamic_cast<CallExpression *>(temp))
        {
            auto type = _->getFunc()->getLiteral();
            auto _type = types->find(type);
            if (!_type)
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "Unkown class: " + type,
                           temp->getToken().getLocation());
                _MARK_ERROR
            }
            return createTypeInfo(type, types);
        }
        else
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "Invalid alloc operation of: " + temp->toString(),
                       temp->getToken().getLocation());
            _MARK_ERROR
        }
    }
    TypeInfo *SemanticAnalyzer::check_class_statement(ClassLiteral *stmt)
    {
        // 类名
        auto name = stmt->getLiteral();
        if (find_symbol(name))
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "redefine class: " + name,
                       stmt->getToken().getLocation());
            _MARK_ERROR
        }
        std::vector<std::unique_ptr<Ident>> &parents = stmt->getBaseClasses();
        bool hasError = false;
        // for (auto &parent : parents)
        // {
        //     if (_SEMANTIC_ERROR(check_expression(parent.get())))
        //     {
        //         hasError = true;
        //     }
        // }
        auto &members = stmt->getMembers();
        ClassDetail clsd = {};
        enter_scope();
        for (auto &member : members)
        {
            auto stmt = member.declaration.get();
            if (auto *_stmt = dynamic_cast<DeclareStatement *>(stmt))
            {
                const auto &vars = _stmt->getVars();
                for (const auto &var : vars)
                {
                    auto name_ = var.first->getLiteral(); // 获取标识符名称
                    if (find_symbol(name_))
                    { // 变量已经被申明了
                        raiseError(ErrorType::SEMANTIC_ERROR, "redefined symbol: " + name_, var.first->getToken().getLocation());
                        hasError = true;
                        continue;
                    }
                    auto type = var.first->getType(); // 获取标识符类型
                    auto expr = var.second;
                    TypeInfo *right_type = nullptr;
                    if (expr) // 有赋值则分析赋值表达式
                    {
                        right_type = check_expression(expr);
                        if (right_type == _errorType)
                            hasError = true; // 解析赋值表达式出错
                        else if (type_to_string(right_type->baseType) != type)
                        {
                            if (type == "")
                            { // 变量类型未显式定义，自动推导，然后固定
                                var.first->setType(right_type->baseType);
                            }
                            // 都是数字，可以尝试强制转化
                            else if (is_number(right_type->baseType) && is_number(string_to_type(type, types)))
                            {
                            }
                            // 类型不匹配,但是也无法数字兼容
                            else
                            {
                                auto t = var.second->getToken();
                                raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type: " + type, var.second->getToken().getLocation());
                                hasError = true;
                            }
                        }
                    }
                    else if(type != ""){ 
                         right_type = createTypeInfo(type,types);
                    }
                    else
                    {
                        right_type = createTypeInfo("undefined", types);
                    }
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = name_;
                    new_symbol->scope_level = current_scope;
                    new_symbol->type_info = right_type;
                    push_symbol(name_, new_symbol);
                    MemDetail md = {};
                    md.level = member.access;
                    md.type = member.type;
                    md.ti = new_symbol->type_info;
                    clsd.mems[name_] = md;
                }
            }
            else if (auto *_stmt = dynamic_cast<FuncDecl *>(stmt))
            {
                TypeInfo *typeInfo = createTypeInfo("func", types);
                std::string name_ = _stmt->getLiteral();
                if (find_symbol(name_))
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "redefine function: " + name_,
                               _stmt->getToken().getLocation());
                    _MARK_ERROR
                }
                auto params = _stmt->getParams();
                bool hasError = false;
                enter_scope(); // 进入函数的局部变量作用域
                FuncDetail funcDetail = {};
                for (auto &param : params)
                {
                    TypeInfo *info = check_param(param);
                    if (_SEMANTIC_ERROR(info))
                    {
                        hasError = true;
                        continue;
                    }
                    funcDetail.param_types.push_back(info->baseType);
                    // 将形参名写入符号表
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = param->name->getLiteral();
                    new_symbol->scope_level = current_scope;
                    new_symbol->type_info = info;
                    push_symbol(param->name->getLiteral(), new_symbol);
                }
                auto body = _stmt->getBody();
                auto t = check_block_statement(body);
                if (_SEMANTIC_ERROR(t))
                {
                    hasError = true;
                }
                else if (_stmt->ret_type && t->baseType->id != _stmt->ret_type->id)
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "unmatch return type:" + t->baseType->alias + ".expect type: " + _stmt->ret_type->alias,
                               _stmt->getToken().getLocation());
                    hasError = true;
                }
                if (hasError)
                {
                    continue;
                }
                if (!_stmt->ret_type)
                {
                     _stmt->ret_type = t->baseType;
                }
                funcDetail.return_type = t->baseType;
                typeInfo->detail = funcDetail;
                exit_scope(); // 离开局部变量作用域

                // 将函数名写入符号表
                Symbol *new_symbol = new Symbol();
                new_symbol->name = name_;
                new_symbol->scope_level = current_scope;
                new_symbol->type_info = typeInfo;
                push_symbol(name_, new_symbol);
                MemDetail md = {};
                md.level = member.access;
                md.type = member.type;
                md.ti = new_symbol->type_info;
                clsd.mems[name_] = md;
            }
            // auto _ = check_statement(member.declaration.get());
            // if (_SEMANTIC_ERROR(_))
            // {
            //     hasError = true;
            // }
            // else
            // {
            // }
        }
        exit_scope();
        if (hasError)
        {
            _MARK_ERROR
        }
        auto ret = createTypeInfo(name, types);
        ret->detail = clsd;
        // 将类名写入符号表
        Symbol *new_symbol = new Symbol();
        new_symbol->name = name;
        new_symbol->scope_level = current_scope;
        new_symbol->type_info = ret;
        push_symbol(name, new_symbol);
        return ret;
    }
    TypeInfo *SemanticAnalyzer::check_return_statement(ReturnStatement *stmt)
    {
        auto val = stmt->getExpression();
        return check_expression(val);
    }
    TypeInfo *SemanticAnalyzer::check_expression_statement(ExpressionStatement *expr)
    {
        auto exp = expr->expression();
        return check_expression(exp);
    }
    TypeInfo *SemanticAnalyzer::check_statement(Statement *stmt)
    {
        if (typeid(*stmt) == typeid(DeclareStatement))
        {
            return check_declare_statement((DeclareStatement *)stmt);
        }
        else if (typeid(*stmt) == typeid(BlockStatement))
        {
            return check_block_statement((BlockStatement *)stmt);
        }
        else if (typeid(*stmt) == typeid(ReturnStatement))
        {
            return check_return_statement((ReturnStatement *)stmt);
        }
        else if (typeid(*stmt) == typeid(FuncDecl))
        {
            return check_func_statement((FuncDecl *)stmt);
        }
        else if (typeid(*stmt) == typeid(ClassLiteral))
        {
            return check_class_statement((ClassLiteral *)stmt);
        }
        else if (typeid(*stmt) == typeid(ExpressionStatement))
        {
            return check_expression_statement((ExpressionStatement *)stmt);
        }
        else
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "unsupport statement: " + stmt->toString(), stmt->getToken().getLocation());
            _MARK_ERROR
        }
    }
    int SemanticAnalyzer::check(Program *program)
    {
        auto stmts = program->stmts;
        for (auto &stmt : stmts)
        {
            auto _ = check_statement(stmt);
            if (_SEMANTIC_ERROR(_))
                return 0;
        }
        return 1;
    }
    TypeInfo *SemanticAnalyzer::check_declare_statement(DeclareStatement *stmt)
    {
        auto vars = stmt->getVars();
        bool hasError = false;
        for (auto it = vars.begin(); it != vars.end(); it++)
        {
            auto name = it->first->getLiteral(); // 获取标识符名称
            if (find_symbol(name))
            { // 变量已经被申明了
                raiseError(ErrorType::SEMANTIC_ERROR, "redefined symbol: " + name, it->first->getToken().getLocation());
                hasError = true;
            }
            auto type = it->first->getType(); // 获取标识符类型
            auto expr = it->second;
            TypeInfo *right_type = nullptr;
            if (expr) // 有赋值则分析赋值表达式
            {
                right_type = check_expression(expr);
                if (right_type == _errorType)
                    hasError = true; // 解析赋值表达式出错
                else if (type_to_string(right_type->baseType) != type)
                {
                    if (type == "")
                    { // 变量类型未显式定义，自动推导，然后固定
                    }
                    // 都是数字，可以尝试强制转化
                    else if (is_number(right_type->baseType) && is_number(string_to_type(type, types)))
                    {
                    }
                    // 类型不匹配,但是也无法数字兼容
                    else
                    {
                        auto t = it->second->getToken();
                        raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type: " + type, it->second->getToken().getLocation());
                        hasError = true;
                    }
                }
            }
            else if(type != ""){ 
                right_type = createTypeInfo(type,types);
            }
            else
            {
                right_type = createTypeInfo("undefined", types);
            }
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = right_type;
            it->first->setType(right_type->baseType);
            push_symbol(name, new_symbol);
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        return createTypeInfo("void", types);
    }
}