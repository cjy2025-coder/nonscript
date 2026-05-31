#include "./frontend/semantic.h"
#include <functional>

namespace ns
{
    TypeInfo *_errorType = new TypeInfo();
    // void SemanticAnalyzer::push_symbol(std::string name, Symbol *symbol)
    // {
    //     symbol_table[name] = symbol; //?
    // }
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
        return em_;
    }
    std::string type_to_string(_type *type)
    {
        return type ? type->alias : "";
    }
    _type *string_to_type(std::string type)
    {
        return typeManager::find(type)->baseType;
    }
    TypeInfo *SemanticAnalyzer::check_ident(Ident *ident)
    {
        auto symbol = find_symbol(ident->getLiteral());
        if (!symbol) // 没有找到这个标识符
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "'" + ident->getLiteral() + "' is not defined", ident->getToken().getLocation());
            MARK_ERROR;
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
            MARK_ERROR;
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
        else if (op == "typeid")
        {
            return typeManager::find("string");
        }
        raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expression->getLiteral(), expression->getToken().getLocation());
        MARK_ERROR;
    }
    TypeInfo *SemanticAnalyzer::check_infix_expression(InfixExpression *expr)
    {
        const auto &left = expr->getLeft();
        auto left_type = check_expression(left);
        if (!left_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid left-hand expression in infix operation", left->getToken().getLocation());
            MARK_ERROR;
        }
        else if (left_type == _errorType)
        {
            MARK_ERROR;
        }

        const auto &right = expr->getRight();
        auto op = expr->getOperator();

        // ---- 递归查找类成员（包括父类） ----
        auto findMemberInClass = [&](const ClassDetail &detail, const std::string &member_name, auto &&self_ref) -> const MemDetail *
        {
            // 在当前类中查找
            auto it = detail.mems.find(member_name);
            if (it != detail.mems.end())
                return &(it->second);
            // 递归查找父类
            for (auto *parent_ti : detail.parents)
            {
                Symbol *parent_s = st->find(parent_ti->baseType->alias);
                if (parent_s && std::holds_alternative<ClassDetail>(parent_s->type_info->detail))
                {
                    const auto &parent_detail = std::get<ClassDetail>(parent_s->type_info->detail);
                    const MemDetail *found = self_ref(parent_detail, member_name, self_ref);
                    if (found)
                        return found;
                }
            }
            return nullptr;
        };

        // ---- 成员访问 . ----
        if (op == ".")
        {
            _type *base_type = left_type->baseType;
            Symbol *s = st->find(base_type->alias);
            if (!s || !std::holds_alternative<ClassDetail>(s->type_info->detail))
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "type '" + base_type->alias + "' is not a class, cannot access member '" + right->getLiteral() + "'", left->getToken().getLocation());
                MARK_ERROR;
            }
            const auto &class_detail = std::get<ClassDetail>(s->type_info->detail);
            const MemDetail *found = findMemberInClass(class_detail, right->getLiteral(), findMemberInClass);
            if (!found)
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "type '" + base_type->alias + "' has no member named '" + right->getLiteral() + "'", right->getToken().getLocation());
                MARK_ERROR;
            }
            if (found->level != AccessLevel::Public)
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "member '" + right->getLiteral() + "' of type '" + base_type->alias + "' is not accessible", right->getToken().getLocation());
                MARK_ERROR;
            }
            return found->ti;
        }

        // ---- 右侧表达式 ----
        auto right_type = check_expression(right);
        if (!right_type)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "invalid right-hand expression in infix operation", right->getToken().getLocation());
            MARK_ERROR;
        }
        else if (right_type == _errorType)
        {
            MARK_ERROR;
        }

        // ---- 赋值 = ----
        if (op == "=")
        {
            if (left_type->baseType != right_type->baseType &&
                !(is_number(left_type->baseType) && is_number(right_type->baseType)))
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                    "cannot assign value of type '" + right_type->baseType->alias + "' to variable of type '" + left_type->baseType->alias + "'",
                    left->getToken().getLocation());
                MARK_ERROR;
            }
            return typeManager::find(left_type->baseType->alias);
        }

        // ---- 算术运算符 + - * / % ----
        if (is_arithmetic_op(op))
        {
            if (is_number(left_type->baseType) && is_number(right_type->baseType))
            {
                if (op == "%")
                {
                    if (is_float(left_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "modulo (%) requires integer type, but left operand is '" + left_type->baseType->alias + "'", left->getToken().getLocation());
                        MARK_ERROR;
                    }
                    if (is_float(right_type->baseType))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "modulo (%) requires integer type, but right operand is '" + right_type->baseType->alias + "'", right->getToken().getLocation());
                        MARK_ERROR;
                    }
                }
                return get_wider_numeric_type(left_type, right_type);
            }
            raiseError(ErrorType::SEMANTIC_ERROR,
                "operator '" + op + "' cannot be applied to operands of types '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
                right->getToken().getLocation());
            MARK_ERROR;
        }

        // ---- 逻辑运算符 && || ----
        if (is_logic_op(op))
        {
            if (is_bool(left_type->baseType) && is_bool(right_type->baseType))
                return left_type;
            raiseError(ErrorType::SEMANTIC_ERROR,
                "operator '" + op + "' requires 'bool' operands, but got '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
                right->getToken().getLocation());
            MARK_ERROR;
        }

        // ---- 其他运算符（==, !=, <, >, <=, >=, <<, >>, &, |, ^ 等） ----
        raiseError(ErrorType::SEMANTIC_ERROR,
            "unsupported operator '" + op + "' between types '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
            right->getToken().getLocation());
        MARK_ERROR;
    }
// TypeInfo *SemanticAnalyzer::check_infix_expression(InfixExpression *expr)
//     {
//         auto left = expr->getLeft();
//         auto left_type = check_expression(left); // 计算左侧表达式
//         bool hasError = false;
//         if (!left_type)
//         {
//             raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expr->getLeft()->toString(), expr->getLeft()->getToken().getLocation());
//             hasError = true;
//         }
//         else if (left_type == _errorType)
//         {
//             hasError = true;
//         }
//         auto right = expr->getRight();
//         auto op = expr->getOperator();
//         if (op == ".") // 优先判断是否在访问类成员
//         {
//             auto t = st->find(left_type->baseType->alias);
//             auto t_ = std::get<ClassDetail>(t->type_info->detail);
//             auto _ = t_.mems.find(right->getLiteral());
//             if (_ == t_.mems.end())
//             {
//                 raiseError(ErrorType::SEMANTIC_ERROR,
//                            "unkown member : " + right->getLiteral() + " of class " + left_type->baseType->alias,
//                            right->getToken().getLocation());
//                 MARK_ERROR;
//             }
//             else if (_->second.level != AccessLevel::Public)
//             {
//                 raiseError(ErrorType::SEMANTIC_ERROR,
//                            "invisiable member : " + right->getLiteral() + " of class " + left_type->baseType->alias,
//                            right->getToken().getLocation());
//                 MARK_ERROR;
//             }
//             else
//             {
//                 return _->second.ti;
//             }
//         }
//         auto right_type = check_expression(right); // 计算右侧表达式
//         if (!right_type)
//         {
//             raiseError(ErrorType::SEMANTIC_ERROR, "invalid expression: " + expr->getRight()->toString(), expr->getRight()->getToken().getLocation());
//             hasError = true;
//         }
//         else if (right_type == _errorType)
//         {
//             hasError = true;
//         }
//         if (hasError)
//         {
//             MARK_ERROR;
//         }
//         if (op == "=")
//         { // 是赋值语句
//             if (left_type->baseType != right_type->baseType &&
//                 !(is_number(left_type->baseType) && is_number(right_type->baseType)))
//             {
//                 hasError = true;
//                 raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type for assignment statements: " + expr->toString(), expr->getToken().getLocation());
//             }
//             else
//             {
//                 return typeManager::find(left_type->baseType->alias);
//             }
//         }
//         else if (is_arithmetic_op(op))
//         {
//             if (is_number(left_type->baseType) && is_number(right_type->baseType))
//             {
//                 if (op == "%")
//                 {
//                     if (is_float(left_type->baseType))
//                     {
//                         raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + left->getLiteral(), left->getToken().getLocation());
//                         MARK_ERROR;
//                     }
//                     if (is_float(right_type->baseType))
//                     {
//                         raiseError(ErrorType::SEMANTIC_ERROR, "Modulo operation requires integer types" + right->getLiteral(), right->getToken().getLocation());
//                         MARK_ERROR;
//                     }
//                 }
//                 return get_wider_numeric_type(left_type, right_type);
//             }
//             else if (op == "+")
//             { // 处理字符串加法
//                 if (is_string(left_type->baseType) && is_string(right_type->baseType))
//                 {
//                     return typeManager::find("string");
//                 }
//                 raiseError(ErrorType::SEMANTIC_ERROR,
//                            "unsupported operation + between ",
//                            right->getToken().getLocation());
//                 MARK_ERROR;
//             }
//         }
//         else if (is_logic_op(op))
//         {
//             if (is_bool(left_type->baseType) && is_bool(right_type->baseType))
//             {
//                 return left_type;
//             }
//         }
//         // else if(op == "."){
//         //     if(left_type->baseType == _type::OBJECT)
//         // }
//         else
//             MARK_ERROR;
//     }
    TypeInfo *SemanticAnalyzer::check_index_expression(IndexExpression *expr)
    {
        auto array = expr->getLeft();
        auto type = check_expression(array);
        if (!type || type == _errorType)
        {
            MARK_ERROR;
        }
        else if (type->baseType->alias != "array")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect an array instead of got type: " + type_to_string(type->baseType),
                       expr->getToken().getLocation());
            MARK_ERROR;
        }
        auto idx = expr->getIndex();
        auto type_ = check_expression(idx);
        if (!type_ || type == _errorType)
        {
            MARK_ERROR;
        }
        else if (type_->baseType->alias != "i8" && type_->baseType->alias != "i16" &&
                 type_->baseType->alias != "i32" && type_->baseType->alias != "i64")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect an integer as index instead of got type: " + type_to_string(type_->baseType),
                       idx->getToken().getLocation());
            MARK_ERROR;
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
            return typeManager::find("undefined");
        ArrayDetail arrayDetail = std::get<ArrayDetail>(type->detail);
        // 对下标进行静态检查
        if (_ < 0 || _ >= arrayDetail.size)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "array index " + std::to_string(_) +
                           " out of bounds [0, " + std::to_string(arrayDetail.size - 1) + "]",
                       idx->getToken().getLocation());
            MARK_ERROR;
        }
        else
        {
            return arrayDetail.elems_types[_];
        }
    }
    TypeInfo *SemanticAnalyzer::check_array_literal(ArrayLiteral *arr)
    {
        TypeInfo *typeInfo = typeManager::find("array");
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
            MARK_ERROR;
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
            MARK_ERROR;
        }
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_param(std::shared_ptr<FuncParam> param)
    {
        auto &ident = param->name;
        auto &val = param->init;
        if (!val)
        { // 形参无默认值
            TypeInfo *typeInfo = typeManager::find(ident->getType());
            std::string name = ident->getLiteral();
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->type_info = typeInfo;
            push_symbol(name, new_symbol);
            return typeInfo;
        }
        else // 处理形参有默认值的情况
        {
            auto val_type_info = check_expression(val.get());
            if (_SEMANTIC_ERROR(val_type_info))
                MARK_ERROR;
            TypeInfo *typeInfo = typeManager::find(ident->getType());
            if (val_type_info->baseType != typeInfo->baseType)
            {
                if (!(is_number(val_type_info->baseType) && is_number(typeInfo->baseType)))
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "function parameter '" + ident->getLiteral() + "' is declared as type '" + ident->getType() + "', but initialize with a type '" + val_type_info->baseType->alias + "' expression",
                               ident->getToken().getLocation());
                    MARK_ERROR;
                }
            }
            std::string name = ident->getLiteral();
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
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
            MARK_ERROR;
        }
        else if (typeInfo->baseType->alias != "func") // 检查括号左边是否是函数名
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect function " + func->getLiteral(),
                       func->getToken().getLocation());
            MARK_ERROR;
        }

        ns::FuncDetail &func_detail = std::get<ns::FuncDetail>(typeInfo->detail);
        std::vector<std::unique_ptr<Expression>> &args = expr->getArgs();
        if (func_detail.is_unlimited_args_func)
        { // 不限制形参列表,此时只检查涉及的符号是否定义
            for (int i = 0; i < args.size(); i++)
            {
                auto type = check_expression(args[i].get());
                if (_SEMANTIC_ERROR(type))
                {
                    MARK_ERROR;
                }
                return typeManager::find(func_detail.return_type->alias);
            }
        }

        auto arg_types = func_detail.param_types;
        if (args.size() != arg_types.size())
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "no match arguments for function: " + func->getLiteral(),
                       func->getToken().getLocation());
            MARK_ERROR;
        }
        for (int i = 0; i < args.size(); i++)
        {
            auto type = check_expression(args[i].get());
            if (_SEMANTIC_ERROR(type))
            {
                MARK_ERROR;
            }
            else if (type->baseType != arg_types[i])
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unmatch type: " + args[i]->getLiteral(),
                           args[i]->getToken().getLocation());
                MARK_ERROR;
            }
        }
        return typeManager::find(func_detail.return_type->alias);
    }
    TypeInfo *SemanticAnalyzer::check_func_statement(FuncDecl *stmt)
    {
        TypeInfo *typeInfo = typeManager::find("func");
        std::string name = stmt->getLiteral();
        if (find_symbol(name))
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "redefine function: " + name,
                       stmt->getToken().getLocation());
            MARK_ERROR;
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
                if (is_number(stmt->ret_type) && is_number(t->baseType)) // 都是数字，尝试强制转换
                {
                }
                else
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "unmatch return type '" + t->baseType->alias + "' for function '" + name + "' ,expect type: '" + stmt->ret_type->alias + "'",
                               stmt->getToken().getLocation());
                    hasError = true;
                }
            }

            if (hasError)
            {
                MARK_ERROR;
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
        new_symbol->type_info = typeInfo;
        push_symbol(name, new_symbol);
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_lambda_expression(LambdaExpr *expr)
    {
        TypeInfo *typeInfo = typeManager::find("func");
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
            MARK_ERROR;
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
            MARK_ERROR;
        }
        return typeManager::find("void");
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
            MARK_ERROR;
        }
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_expression(Expression *expr)
    {
        if (!expr)
            return nullptr;
        if (typeid(*expr) == typeid(I8Literal))
            return typeManager::find("i8");
        else if (typeid(*expr) == typeid(I16Literal))
            return typeManager::find("i16");
        else if (typeid(*expr) == typeid(I32Literal))
            return typeManager::find("i32");
        else if (typeid(*expr) == typeid(I64Literal))
            return typeManager::find("i64");
        else if (typeid(*expr) == typeid(Float32Literal))
            return typeManager::find("f32");
        else if (typeid(*expr) == typeid(Float64Literal))
            return typeManager::find("f64");
        else if (typeid(*expr) == typeid(NullLiteral))
            return typeManager::find("void");
        else if (typeid(*expr) == typeid(BoolLiteral))
            return typeManager::find("bool");
        else if (typeid(*expr) == typeid(StringLiteral))
            return typeManager::find("string");
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
            MARK_ERROR;
    }
    TypeInfo *SemanticAnalyzer::check_new_expression(NewExpression *expr)
    {
        auto temp = expr->getRight();
        if (auto *_ = dynamic_cast<Ident *>(temp))
        { // 直接调用默认构造函数
            auto type = _->getLiteral();
            auto _type = typeManager::find(type);
            if (!_type)
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "Unkown class: " + type,
                           temp->getToken().getLocation());
                MARK_ERROR;
            }
            return typeManager::find(type);
        }
        else if (auto *_ = dynamic_cast<CallExpression *>(temp))
        {
            auto type = _->getFunc()->getLiteral();
            auto _type = typeManager::find(type);
            if (!_type)
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "Unkown class: " + type,
                           temp->getToken().getLocation());
                MARK_ERROR;
            }
            return typeManager::find(type);
        }
        else
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "Invalid alloc operation of: " + temp->toString(),
                       temp->getToken().getLocation());
            MARK_ERROR;
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
            MARK_ERROR;
        }
        std::vector<std::unique_ptr<Ident>> &parents = stmt->getBaseClasses();
        bool hasError = false;
        ClassDetail clsd = {};
        for (auto &parent : parents)
        {
            const auto &check_result = check_ident(parent.get());
            clsd.parents.push_back(check_result);
            if (_SEMANTIC_ERROR(check_result))
            {
                hasError = true;
            }
        }
        auto &members = stmt->getMembers();

        // 递归将父类所有可见成员加入符号表
        std::function<void(const std::string &)> importParentMems = [&](const std::string &parent_name)
        {
            Symbol *parent_s = st->find(parent_name);
            if (!parent_s || !std::holds_alternative<ClassDetail>(parent_s->type_info->detail))
                return;
            const auto &parent_detail = std::get<ClassDetail>(parent_s->type_info->detail);
            // 先递归导入父类的父类
            for (auto *grandparent_ti : parent_detail.parents)
            {
                importParentMems(grandparent_ti->baseType->alias);
            }
            // 导入父类成员（不覆盖已定义的）
            for (const auto &[mem_name, mem] : parent_detail.mems)
            {
                if (mem.level != AccessLevel::Private && !find_symbol(mem_name))
                {
                    Symbol *sym = new Symbol();
                    sym->name = mem_name;
                    sym->type_info = mem.ti;
                    push_symbol(mem_name, sym);
                }
            }
        };

        enter_scope();
        // 先导入父类成员（让子类可以覆盖）
        for (auto &parent : parents)
        {
            importParentMems(parent->getLiteral());
        }
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
                            else if (is_number(right_type->baseType) && is_number(string_to_type(type)))
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
                    else if (type != "")
                    {
                        right_type = typeManager::find(type);
                    }
                    else
                    {
                        right_type = typeManager::find("undefined");
                    }
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = name_;
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
                TypeInfo *typeInfo = typeManager::find("func");
                std::string name_ = _stmt->getLiteral();
                if (find_symbol(name_))
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "redefine function: " + name_,
                               _stmt->getToken().getLocation());
                    MARK_ERROR;
                }
                auto params = _stmt->getParams();
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
            MARK_ERROR;
        }
        auto ret = typeManager::find(name);
        ret->detail = clsd;
        // 将类名写入符号表
        Symbol *new_symbol = new Symbol();
        new_symbol->name = name;
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
        const auto & exp = expr->expression();
        return check_expression(exp);
    }
    TypeInfo *SemanticAnalyzer::check_import_statement(ImportStatement *stmt)
    {
        // undo
        // 在退出库文件的解析后，它的SymbolManager一定只有一层，
        // 此时可以把它所有的符号写入当前语法分析器的SymbolManager的最顶层作用域
        // 对所有包含的库文件做上述操作,即：
        // import "a.ss"
        // import "b.ss"
        // a.ss,b.ss的符号将一起写入当前语法分析器的SymbolManager的最顶层作用域
        // 而不是 a.ss -> b.ss -> 当前源文件 的作用域结构
        
        // const std::string & path = stmt->getPath();
        // auto absolute_path = ns::get_absolute_path(path).string();
        // std::string source = read_file(absolute_path);
        // ns::Lexer lexer(source, absolute_path);
        // std::vector<ns::Token> tokens;
        // ns::Token token;
        // while ((token = lexer.scan()).getType() != ns::TokenType::END) {
        //     tokens.push_back(token);
        // }
        // tokens.push_back(token);
        // // 语法分析
        // ns::Parser parser(tokens);
        // parser.setLexer(&lexer);
        // auto program = parser.parse();
        // if (!program){
        //     ns::error(parser.what()->whats());
        //     MARK_ERROR;
        // }
        // // 语义分析
        // ns::SemanticAnalyzer sa(&lexer);
        // if (!sa.check(program.get())) {
        //     ns::error(sa.what()->whats());
        //     MARK_ERROR;
        // }
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::collect_and_check_all_import_statement(Program *program)
    {
        auto &stmts = program->stmts;
        for (auto &stmt : stmts)
        {
            // 是import语句，则进行检查
            if (auto *ims = dynamic_cast<ImportStatement *>(stmt))
            {
                const TypeInfo *const cr = check_import_statement(ims);
                if (_SEMANTIC_ERROR(cr))
                {
                    MARK_ERROR;
                }
            }
        }
        return typeManager::find("void");
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
        else if (auto *st = dynamic_cast<ForLoop *>(stmt))
        {
            return check_for_loop_statement(st);
        }
        else
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "unsupport statement: " + stmt->toString(), stmt->getToken().getLocation());
            MARK_ERROR;
        }
    }
    TypeInfo *SemanticAnalyzer::check_for_loop_statement(ForLoop *stmt)
    {
        auto &vars = stmt->getVariables();
        const auto &condtion = stmt->getCondtion();
        const auto &actions = stmt->getActions();
        const auto &body = stmt->getBody();
        enter_scope(); // 进入 for-loop 局部作用域
        for (auto &var : vars)
        {
            auto &id = var->var;
            auto &val = var->value;

            // 没有初始化
            if (!val)
            {
                if (var->external_variable)
                {
                    auto evt = check_ident(id.get());
                    // 符号不存在
                    if (_SEMANTIC_ERROR(evt))
                    {
                        MARK_ERROR;
                    }
                }
                else
                {
                    // 把 id 信息加入符号表
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = id->getLiteral();
                    new_symbol->type_info = typeManager::find("undefined");
                    push_symbol(id->getLiteral(), new_symbol);
                }
                continue;
            }
            auto vt = check_expression(val.get());

            if (_SEMANTIC_ERROR(vt))
            {
                MARK_ERROR;
            }

            // 是捕获的外部变量，直接检查类型，不用写入符号表
            if (var->external_variable)
            {
                auto evt = check_ident(id.get());
                // 符号不存在
                if (_SEMANTIC_ERROR(evt))
                {

                    MARK_ERROR;
                }
                // 类型不匹配
                else if (evt != vt)
                {

                    if (!(is_number(evt->baseType) && is_number(vt->baseType)))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR,
                                   "can't convert type '" + vt->baseType->alias + "' to type '" + evt->baseType->alias + "'",
                                   id->getToken().getLocation());
                        MARK_ERROR;
                    }
                }
            }
            else
            {
                auto id_type = id->getType();
                // 没有显式申明类型
                if (id_type == "")
                {
                    id->setType(vt->baseType);
                }
                else if (id_type != vt->baseType->alias)
                {
                    if (!(is_number(string_to_type(id_type)) && is_number(vt->baseType)))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR,
                                   "can't convert type '" + vt->baseType->alias + "' to type '" + id_type + "'",
                                   id->getToken().getLocation());
                        MARK_ERROR;
                    }
                }
                // 把 id 信息加入符号表
                Symbol *new_symbol = new Symbol();
                new_symbol->name = id->getLiteral();
                new_symbol->type_info = typeManager::find(id->getType());
                push_symbol(id->getLiteral(), new_symbol);
            }
        }
        auto cr = check_expression(condtion);
        if(_SEMANTIC_ERROR(cr)){
            MARK_ERROR;
        }
        for(auto & action : actions){
            auto ar = check_expression(action.get());
            if(_SEMANTIC_ERROR(ar)){
                MARK_ERROR;
            }
        }
        auto br = check_block_statement(body);
        if(_SEMANTIC_ERROR(br)){
            MARK_ERROR;
        }
        exit_scope(); // 离开 for-loop 局部作用域
        return typeManager::find("void");
    }
    int SemanticAnalyzer::check(Program *program)
    {
        auto stmts = program->stmts;
        enter_scope();
        init_builtin_funcs(); // 载入内置函数，内置函数的作用域最高
        // const TypeInfo * const check_result = collect_and_check_all_import_statement(program);
        // if(_SEMANTIC_ERROR(check_result)){
        //     return 0;
        // }
        for (auto &stmt : stmts)
        {
            // 是import语句， 不再处理， 因为已经处理过了
            if (auto *ims = dynamic_cast<ImportStatement *>(stmt))
            {
                continue;
            }
            auto _ = check_statement(stmt);
            if (_SEMANTIC_ERROR(_))
                return 0;
        }
        exit_scope();
        return 1;
    }
    TypeInfo *SemanticAnalyzer::check_declare_statement(DeclareStatement *stmt)
    {
        auto vars = stmt->getVars();
        bool hasError = false;
        for (auto it = vars.begin(); it != vars.end(); it++)
        {
            // 获取标识符名称
            auto name = it->first->getLiteral();
            // 变量已经被申明了
            if (find_symbol(name))
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "redefined symbol: " + name, it->first->getToken().getLocation());
                hasError = true;
            }

            // 获取标识符声明的类型
            auto type = it->first->getType();
            // 符号的初始化表达式（如果有的话）
            auto expr = it->second;
            TypeInfo *right_type = nullptr;

            // 有赋值，分析赋值表达式，检查类型是否匹配，可能时进行强制转换
            if (expr)
            {
                // 尝试获取右侧表达式的类型
                right_type = check_expression(expr);
                // 解析赋值表达式出错
                if (right_type == _errorType)
                    hasError = true;
                else if (type_to_string(right_type->baseType) != type)
                {
                    // 变量类型未显式定义，自动推导，然后固定
                    if (type == "")
                    {
                    }
                    // 都是数字，可以尝试强制转化
                    else if (is_number(right_type->baseType) && is_number(string_to_type(type)))
                    {
                        // std::string vt = right_type->baseType->alias;
                        // if()
                    }
                    // 类型不匹配,但是也无法数字兼容
                    else
                    {
                        auto t = it->second->getToken();
                        raiseError(ErrorType::SEMANTIC_ERROR,
                                   "can't convert type '" + right_type->baseType->alias + "' to type '" + type + "'",
                                   it->second->getToken().getLocation());
                        hasError = true;
                    }
                }
            }
            // 没初始化，但显式声明了类型
            else if (type != "")
            {
                right_type = typeManager::find(type);
            }
            // 没显式声明类型，也没进行初始化，此时无法获取符号类型
            else
            {
                right_type = typeManager::find("undefined");
            }
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->type_info = right_type;
            it->first->setType(right_type->baseType);
            push_symbol(name, new_symbol);
        }
        if (hasError)
        {
            MARK_ERROR;
        }
        return typeManager::find("void");
    }
}