#include "./frontend/semantic.h"

namespace ns
{
    void SemanticAnalyzer::push_symbol(std::string name, Symbol *symbol)
    {
        symbol_table[name] = symbol;
    }
    bool SemanticAnalyzer::is_arithmetic_op(std::string op)
    {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "%";
    }
    bool SemanticAnalyzer::is_logic_op(std::string op)
    {
        return op == "&&" || op == "||";
    }
    bool SemanticAnalyzer::is_int(DataType type)
    {
        return type == DataType::INT8 || type == DataType::INT16 || type == DataType::INT32 || type == DataType::INT64;
    }
    bool SemanticAnalyzer::is_float(DataType type)
    {
        return type == DataType::FLOAT32 || type == DataType::FLOAT64;
    }
    bool SemanticAnalyzer::is_number(DataType type)
    {
        return is_float(type) || is_int(type);
    }
    bool SemanticAnalyzer::is_string(DataType type)
    {
        return type == DataType::STRING;
    }
    bool SemanticAnalyzer::is_bool(DataType type)
    {
        return type == DataType::BOOLEAN;
    }
    TypeInfo *SemanticAnalyzer::get_wider_numeric_type(TypeInfo *left, TypeInfo *right)
    {
        static std::map<DataType, int> type_precedence = {
            {DataType::INT8, 1}, {DataType::INT16, 2}, {DataType::INT32, 3}, {DataType::INT64, 4}, {DataType::FLOAT32, 5}, {DataType::FLOAT64, 6}};
        int left_prio = type_precedence[left->baseType];
        int right_prio = type_precedence[right->baseType];
        return left_prio > right_prio ? left : right;
    }
    ErrorManager *SemanticAnalyzer::what()
    {
        return errorManager;
    }
    TypeInfo *createTypeInfo(DataType baseType_)
    {
        TypeInfo *ret = new TypeInfo();
        ret->baseType = baseType_;
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
        return it->second;
    }
    std::string SemanticAnalyzer::type_to_string(DataType type)
    {
        switch (type)
        {
        case DataType::INT8:
            return "int8";
        case DataType::INT16:
            return "int16";
        case DataType::INT32:
            return "int32";
        case DataType::INT64:
            return "int64";
        case DataType::FLOAT32:
            return "float32";
        case DataType::FLOAT64:
            return "float64";
        case DataType::FUNCTION:
            return "function";
        case DataType::ARRAY:
            return "array";
        case DataType::PAIR:
            return "pair";
        case DataType::NONE:
            return "null";
        case DataType::CLASS:
            return "class";
        case DataType::STRING:
            return "string";
        case DataType::OBJECT:
            return "object";
        case DataType::BOOLEAN:
            return "bool";
        default:
            return "unknown";
        }
    }
    DataType SemanticAnalyzer::string_to_type(std::string type)
    {
        if (type == "int8")
            return DataType::INT8;
        if (type == "int16")
            return DataType::INT16;
        if (type == "int32")
            return DataType::INT32;
        if (type == "int64")
            return DataType::INT64;
        if (type == "float32")
            return DataType::FLOAT32;
        if (type == "float64")
            return DataType::FLOAT64;
        if (type == "function")
            return DataType::FUNCTION;
        if (type == "array")
            return DataType::ARRAY;
        if (type == "pair")
            return DataType::PAIR;
        if (type == "null")
            return DataType::NONE;
        if (type == "bool")
            return DataType::BOOLEAN;
        if (type == "" || type == "any")
            return DataType::ANY;
        return DataType::ERROR;
    }
    TypeInfo *SemanticAnalyzer::check_ident(Ident *ident)
    {
        auto symbol = find_symbol(ident->getLiteral());
        if (!symbol) // 没有找到这个标识符
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "undefined symbol: " + ident->getLiteral(), ident->getToken().getLocation());
            return createTypeInfo(DataType::ERROR);
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
            return createTypeInfo(DataType::ERROR);
        }
        else if (expression_type->baseType == DataType::ERROR)
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
        raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expression->getLiteral(), expression->getToken().getLocation());
        return createTypeInfo(DataType::ERROR);
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
        else if (left_type->baseType == DataType::ERROR)
        {
            hasError = true;
        }
        auto right = expr->getRight();
        auto right_type = check_expression(right); // 计算右侧表达式
        if (!right_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expr->getRight()->toString(), expr->getRight()->getToken().getLocation());
            hasError = true;
        }
        else if (right_type->baseType == DataType::ERROR)
        {
            hasError = true;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        auto op = expr->getOperator();
        if (is_arithmetic_op(op))
        {
            if (is_number(left_type->baseType) && is_number(right_type->baseType))
            {
                if (op == "%")
                {
                    if (is_float(left_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + left->getLiteral(), left->getToken().getLocation());
                        return createTypeInfo(DataType::ERROR);
                    }
                    if (is_float(right_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + right->getLiteral(), right->getToken().getLocation());
                        return createTypeInfo(DataType::ERROR);
                    }
                }
                return get_wider_numeric_type(left_type, right_type);
            }
            else if (op == "+")
            { // 处理字符串加法
                if (is_string(left_type->baseType) && is_string(right_type->baseType))
                {
                    return createTypeInfo(DataType::STRING);
                }
                raiseError(ErrorType::SEMANTIC_ERROR, "unsupported operation + between ", right->getToken().getLocation());
                return createTypeInfo(DataType::ERROR);
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
        //     if(left_type->baseType == DataType::OBJECT)
        // }
        return createTypeInfo(DataType::ERROR);
    }
    TypeInfo *SemanticAnalyzer::check_index_expression(IndexExpression *expr)
    {
        auto array = expr->getLeft();
        auto type = check_expression(array);
        if (!type || type->baseType == DataType::ERROR)
        {
            _MARK_ERROR
        }
        if (type->baseType != DataType::ARRAY)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect an array instead of got type: " + type_to_string(type->baseType),
                       expr->getToken().getLocation());
            _MARK_ERROR
        }
        auto idx = expr->getIndex();
        auto type_ = check_expression(idx);
        if (!type_ || type->baseType == DataType::ERROR)
        {
            _MARK_ERROR
        }
        if (type_->baseType != DataType::INT8 && type_->baseType != DataType::INT16 && type_->baseType != DataType::INT32 && type_->baseType != DataType::INT64)
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
            return createTypeInfo(DataType::UNKNOWN);
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
        TypeInfo *typeInfo = createTypeInfo(DataType::ARRAY);
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
        bool hasError=false;
        for (auto &stmt : stmts)
        {
            typeInfo = check_statement(stmt);
            if (_SEMANTIC_ERROR(typeInfo)){
                hasError=true;
                continue;
            }
            if (typeid(*stmt) == typeid(ReturnStatement))
                return typeInfo;
        }
        if(hasError){
            _MARK_ERROR
        }
        return createTypeInfo(DataType::NONE);
    }
    TypeInfo *SemanticAnalyzer::check_param(Expression *param)
    {
        if (typeid(*param) == typeid(Ident)) // 形参无默认值
        {
            Ident *ident = (Ident *)param;
            auto t1 = ident->getType();
            TypeInfo *typeInfo = createTypeInfo(string_to_type(ident->getType()));
            std::string name = ident->getLiteral();
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = typeInfo;
            push_symbol(name, new_symbol);
            return typeInfo;
        }
        else if (typeid(*param) == typeid(InfixExpression)) // 处理形参有默认值的情况
        {
            InfixExpression *infixExpression = (InfixExpression *)param;
            Ident *ident = (Ident *)infixExpression->getLeft();
            auto right = infixExpression->getRight();
            auto right_type_info = check_expression(right);
            if (_SEMANTIC_ERROR(right_type_info))
                _MARK_ERROR
            TypeInfo *typeInfo = createTypeInfo(string_to_type(ident->getType()));
            if (right_type_info->baseType != typeInfo->baseType)
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
        if (typeInfo->baseType != DataType::FUNCTION)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect function " + func->getLiteral(),
                       func->getToken().getLocation());
            return nullptr;
        }
        std::vector<std::unique_ptr<Expression>> &args = expr->getArgs();
        ns::FuncDetail &func_detail = std::get<ns::FuncDetail>(typeInfo->detail);
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
        return createTypeInfo(func_detail.return_type);
    }
    TypeInfo *SemanticAnalyzer::check_func_statement(FuncDecl *stmt)
    {
        TypeInfo *typeInfo = createTypeInfo(DataType::FUNCTION);
        auto params = stmt->getParams();
        bool hasError = false;
        enter_scope(); // 进入函数的局部变量作用域
        FuncDetail funcDetail = {};
        for (auto &param : params)
        {
            TypeInfo *info = check_param(param.get());
            if (_SEMANTIC_ERROR(info))
            {
                hasError = true;
                continue;
            }
            funcDetail.param_types.push_back(info->baseType);
        }
        auto body = stmt->getBody();
        auto t = check_block_statement(body);
        if (_SEMANTIC_ERROR(t))
        {
            hasError = true;
        }
        if (hasError)
        {
            _MARK_ERROR
        }
        funcDetail.return_type = t->baseType;
        typeInfo->detail = funcDetail;
        exit_scope(); // 离开局部变量作用域
        // 将函数名写入符号表
        std::string name = stmt->getLiteral();
        Symbol *new_symbol = new Symbol();
        new_symbol->name = name;
        new_symbol->scope_level = current_scope;
        new_symbol->type_info = typeInfo;
        push_symbol(name, new_symbol);
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_lambda_expression(LambdaExpr *expr)
    {
        TypeInfo *typeInfo = createTypeInfo(DataType::FUNCTION);
        auto params = expr->getParams();
        bool hasError = false;
        enter_scope(); // 进入函数的局部变量作用域
        FuncDetail funcDetail = {};
        for (auto &param : params)
        {
            TypeInfo *info = check_param(param.get());
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
        if (hasError)
        {
            _MARK_ERROR
        }
        funcDetail.return_type = t->baseType;
        typeInfo->detail = funcDetail;
        exit_scope(); // 离开局部变量作用域
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_if_expression(IfExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError=false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
        {
           hasError=true;
        }
        auto consequence = expr->getConsequence();
        if (_SEMANTIC_ERROR(check_statement(consequence)))
        {
            hasError=true;
        }
        auto alternative = expr->getAlternative();
        if (alternative != NULL)
        {
            if (_SEMANTIC_ERROR(check_statement(alternative)))
            {
               hasError=true;
            }
        }
        if(hasError){
            _MARK_ERROR
        }
        return createTypeInfo(DataType::NONE);
    }
    TypeInfo *SemanticAnalyzer::check_while_expression(WhileExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError=false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
        {
            hasError=true;
        }
        auto body = expr->getBody();
        if (_SEMANTIC_ERROR(check_statement(body)))
        {
            hasError=true;
        }
        if(hasError){
            _MARK_ERROR
        }
        return createTypeInfo(DataType::NONE);
    }
    TypeInfo *SemanticAnalyzer::check_expression(Expression *expr)
    {
        if (!expr)
            return nullptr;
        if (typeid(*expr) == typeid(I8Literal))
            return createTypeInfo(DataType::INT8);
        else if (typeid(*expr) == typeid(I16Literal))
            return createTypeInfo(DataType::INT16);
        else if (typeid(*expr) == typeid(I32Literal))
            return createTypeInfo(DataType::INT32);
        else if (typeid(*expr) == typeid(I64Literal))
            return createTypeInfo(DataType::INT64);
        else if (typeid(*expr) == typeid(Float32Literal))
            return createTypeInfo(DataType::FLOAT32);
        else if (typeid(*expr) == typeid(Float64Literal))
            return createTypeInfo(DataType::FLOAT64);
        else if (typeid(*expr) == typeid(NullLiteral))
            return createTypeInfo(DataType::NONE);
        else if (typeid(*expr) == typeid(BoolLiteral))
            return createTypeInfo(DataType::BOOLEAN);
        else if (typeid(*expr) == typeid(StringLiteral))
            return createTypeInfo(DataType::STRING);
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
        else
            _MARK_ERROR
    }
    TypeInfo *SemanticAnalyzer::check_class_statement(ClassLiteral *stmt)
    {
        std::vector<std::unique_ptr<Ident>> &parents = stmt->getBaseClasses();
        bool hasError=true;
        for (auto &parent : parents)
        {
            if (_SEMANTIC_ERROR(check_expression(parent.get())))
            {
                hasError=true;
            }
        }
        auto &members = stmt->getMembers();
        for (auto &member : members)
        {
            if (_SEMANTIC_ERROR(check_statement(member.declaration.get())))
            {
                hasError=true;
            }
        }
        if(hasError){
            _MARK_ERROR
        }
        return createTypeInfo(DataType::NONE);
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
                if (right_type->baseType == DataType::ERROR)
                    hasError = true; // 解析赋值表达式出错
                if (type != "any" && type_to_string(right_type->baseType) != type)
                { // 类型不匹配,但是也无法数字兼容
                    if (!is_number(right_type->baseType) && !is_number(string_to_type(type)))
                    {
                        auto t = it->second->getToken();
                        raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type: " + name, it->second->getToken().getLocation());
                        hasError = true;
                    }
                }
            }
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->scope_level = current_scope;
            new_symbol->type_info = right_type;
            push_symbol(name, new_symbol);
        }
        if (hasError)
        {
            return createTypeInfo(DataType::ERROR);
        }
        return createTypeInfo(DataType::NONE);
    }
}