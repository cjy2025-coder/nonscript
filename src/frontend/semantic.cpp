#include "./frontend/semantic.h"
#include "./frontend/parser.h"
#include "compiler.h"
#include <functional>
#include <fstream>

namespace ns
{
    TypeInfo *_errorType = new TypeInfo();
 
    // +, -, *, / , %
    bool SemanticAnalyzer::is_arithmetic_op(std::string op)
    {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "%";
    }
    // &&, ||
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
        if (!symbol)
        {
            raiseError(ErrorType::SEMANTIC_ERROR, "'" + ident->getLiteral() + "' is not defined", ident->getToken().getLocation());
            MARK_ERROR;
        }
        // 将语义分析得到的类型设置到 AST 节点上
        ident->setType(symbol->type_info->baseType);
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

        auto findMemberInClass = [&](const ClassDetail &detail, const std::string &member_name, auto &&self_ref) -> const MemDetail *
        {
            auto it = detail.mems.find(member_name);
            if (it != detail.mems.end())
                return &(it->second);
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

        if (is_logic_op(op))
        {
            if (is_bool(left_type->baseType) && is_bool(right_type->baseType))
                return left_type;
            raiseError(ErrorType::SEMANTIC_ERROR,
                "operator '" + op + "' requires 'bool' operands, but got '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
                right->getToken().getLocation());
            MARK_ERROR;
        }

        if(op == "==" || op == "!="){
            if(left_type->baseType == right_type->baseType){
                return typeManager::find("bool");
            }
            else if(is_number(left_type->baseType) && is_number(right_type->baseType)){
                return typeManager::find("bool");
            }
            raiseError(ErrorType::SEMANTIC_ERROR,
                "unsupported operator '" + op + "' between types '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
                right->getToken().getLocation());
            MARK_ERROR;
        }

        if(op == ">" || op == ">=" || op == "<" || op == "<="){
            if(is_number(left_type->baseType) && is_number(right_type->baseType)){
                return typeManager::find("bool");
            }
            raiseError(ErrorType::SEMANTIC_ERROR,
                "unsupported operator '" + op + "' between types '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
                right->getToken().getLocation());
            MARK_ERROR;
        }

        raiseError(ErrorType::SEMANTIC_ERROR,
            "unsupported operator '" + op + "' between types '" + left_type->baseType->alias + "' and '" + right_type->baseType->alias + "'",
            right->getToken().getLocation());
        MARK_ERROR;
    }
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
        int64_t _ = -1;
        bool static_check = true;
        if (auto *constant_idx = dynamic_cast<I8Literal *>(idx))
            _ = constant_idx->getValue();
        else if (auto *constant_idx = dynamic_cast<I16Literal *>(idx))
            _ = constant_idx->getValue();
        else if (auto *constant_idx = dynamic_cast<I32Literal *>(idx))
            _ = constant_idx->getValue();
        else if (auto *constant_idx = dynamic_cast<I64Literal *>(idx))
            _ = constant_idx->getValue();
        else
            static_check = false;
        if (!static_check)
            return typeManager::find("undefined");
        ArrayDetail arrayDetail = std::get<ArrayDetail>(type->detail);
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
            MARK_ERROR;
        ArrayDetail arrayDetail = {};
        arrayDetail.size = size;
        arrayDetail.elems_types = elemTypes;
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
            MARK_ERROR;
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_param(std::shared_ptr<FuncParam> param)
    {
        auto &ident = param->name;
        auto &val = param->init;
        if (!val)
        {
            TypeInfo *typeInfo = typeManager::find(ident->getType());
            Symbol *new_symbol = new Symbol();
            new_symbol->name = ident->getLiteral();
            new_symbol->type_info = typeInfo;
            push_symbol(ident->getLiteral(), new_symbol);
            return typeInfo;
        }
        else
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
            Symbol *new_symbol = new Symbol();
            new_symbol->name = ident->getLiteral();
            new_symbol->type_info = typeInfo;
            push_symbol(ident->getLiteral(), new_symbol);
            return typeInfo;
        }
    }
    TypeInfo *SemanticAnalyzer::check_call_expression(CallExpression *expr)
    {
        auto func = expr->getFunc();
        auto typeInfo = check_expression(expr->getFunc());
        if (_SEMANTIC_ERROR(typeInfo))
            MARK_ERROR;
        else if (typeInfo->baseType->alias != "func")
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "expect function " + func->getLiteral(),
                       func->getToken().getLocation());
            MARK_ERROR;
        }

        ns::FuncDetail &func_detail = std::get<ns::FuncDetail>(typeInfo->detail);
        std::vector<std::unique_ptr<Expression>> &args = expr->getArgs();
        if (func_detail.is_unlimited_args_func)
        {
            for (int i = 0; i < args.size(); i++)
            {
                auto type = check_expression(args[i].get());
                if (_SEMANTIC_ERROR(type))
                    MARK_ERROR;
                TypeInfo *ret_ti = typeManager::find(func_detail.return_type->alias);
                expr->setType(ret_ti->baseType);
                return ret_ti;
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
                MARK_ERROR;
            else if (type->baseType != arg_types[i])
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "unmatch type: " + args[i]->getLiteral(),
                           args[i]->getToken().getLocation());
                MARK_ERROR;
            }
        }

        TypeInfo *ret_ti = typeManager::find(func_detail.return_type->alias);
        expr->setType(ret_ti->baseType);
        return ret_ti;
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
        enter_scope();
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
            Symbol *new_symbol = new Symbol();
            new_symbol->name = param->name->getLiteral();
            new_symbol->type_info = info;
            push_symbol(param->name->getLiteral(), new_symbol);
        }
        if (!stmt->check_if_extern_func())
        {
            auto body = stmt->getBody();
            auto t = check_block_statement(body);
            if (_SEMANTIC_ERROR(t))
                hasError = true;
            else if (stmt->ret_type && t->baseType->id != stmt->ret_type->id)
            {
                if (is_number(stmt->ret_type) && is_number(t->baseType)) {}
                else
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "unmatch return type '" + t->baseType->alias + "' for function '" + name + "' ,expect type: '" + stmt->ret_type->alias + "'",
                               stmt->getToken().getLocation());
                    hasError = true;
                }
            }
            if (hasError)
                MARK_ERROR;
            if (!stmt->ret_type)
                stmt->ret_type = t->baseType;
        }
        funcDetail.return_type = stmt->ret_type;
        typeInfo->detail = funcDetail;
        exit_scope();
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
        enter_scope();
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
            hasError = true;
        else if (expr->ret_type && t->baseType->id != expr->ret_type->id)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "unmatch return type:" + t->baseType->alias + " . expect type: " + expr->ret_type->alias,
                       expr->getToken().getLocation());
            hasError = true;
        }
        if (hasError)
            MARK_ERROR;
        if (!expr->ret_type)
            expr->ret_type = t->baseType;
        funcDetail.return_type = t->baseType;
        typeInfo->detail = funcDetail;
        exit_scope();
        return typeInfo;
    }
    TypeInfo *SemanticAnalyzer::check_if_expression(IfExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError = false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
            hasError = true;
        auto consequence = expr->getConsequence();
        if (_SEMANTIC_ERROR(check_statement(consequence)))
            hasError = true;
        auto alternative = expr->getAlternative();
        if (alternative != NULL)
        {
            if (_SEMANTIC_ERROR(check_statement(alternative)))
                hasError = true;
        }
        if (hasError)
            MARK_ERROR;
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_while_expression(WhileExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError = false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
            hasError = true;
        auto body = expr->getBody();
        if (_SEMANTIC_ERROR(check_statement(body)))
            hasError = true;
        if (hasError)
            MARK_ERROR;
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_until_expression(UntilExpression *expr)
    {
        auto condition = expr->getCondition();
        bool hasError = false;
        if (_SEMANTIC_ERROR(check_expression(condition)))
            hasError = true;
        auto body = expr->getBody();
        if (_SEMANTIC_ERROR(check_statement(body)))
            hasError = true;
        if (hasError)
            MARK_ERROR;
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
            return check_call_expression((CallExpression *)expr);
        else if (typeid(*expr) == typeid(IfExpression))
            return check_if_expression((IfExpression *)expr);
        else if (typeid(*expr) == typeid(WhileExpression))
            return check_while_expression((WhileExpression *)expr);
        else if (auto * e = dynamic_cast<UntilExpression*>(expr)){
            return check_until_expression(e);
        }
        else if (typeid(*expr) == typeid(NewExpression))
            return check_new_expression((NewExpression *)(expr));
        else
            MARK_ERROR;
    }
    TypeInfo *SemanticAnalyzer::check_new_expression(NewExpression *expr)
    {
        auto temp = expr->getRight();
        if (auto *_ = dynamic_cast<Ident *>(temp))
        {
            auto type = _->getLiteral();
            if (!typeManager::find(type))
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
            if (!typeManager::find(type))
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
                hasError = true;
        }
        auto &members = stmt->getMembers();

        std::function<void(const std::string &)> importParentMems = [&](const std::string &parent_name)
        {
            Symbol *parent_s = st->find(parent_name);
            if (!parent_s || !std::holds_alternative<ClassDetail>(parent_s->type_info->detail))
                return;
            const auto &parent_detail = std::get<ClassDetail>(parent_s->type_info->detail);
            for (auto *grandparent_ti : parent_detail.parents)
                importParentMems(grandparent_ti->baseType->alias);
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
        for (auto &parent : parents)
            importParentMems(parent->getLiteral());
        for (auto &member : members)
        {
            auto stmt = member.declaration.get();
            if (auto *_stmt = dynamic_cast<DeclareStatement *>(stmt))
            {
                const auto &vars = _stmt->getVars();
                for (const auto &var : vars)
                {
                    auto name_ = var.first->getLiteral();
                    if (find_symbol(name_))
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR, "redefined symbol: " + name_, var.first->getToken().getLocation());
                        hasError = true;
                        continue;
                    }
                    auto type = var.first->getType();
                    auto expr = var.second;
                    TypeInfo *right_type = nullptr;
                    if (expr)
                    {
                        right_type = check_expression(expr);
                        if (right_type == _errorType)
                            hasError = true;
                        else if (type_to_string(right_type->baseType) != type)
                        {
                            if (type == "")
                                var.first->setType(right_type->baseType);
                            else if (is_number(right_type->baseType) && is_number(string_to_type(type))) {}
                            else
                            {
                                raiseError(ErrorType::SEMANTIC_ERROR, "unmatch type: " + type, var.second->getToken().getLocation());
                                hasError = true;
                            }
                        }
                    }
                    else if (type != "")
                        right_type = typeManager::find(type);
                    else
                        right_type = typeManager::find("undefined");
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
                    raiseError(ErrorType::SEMANTIC_ERROR, "redefine function: " + name_, _stmt->getToken().getLocation());
                    MARK_ERROR;
                }
                auto params = _stmt->getParams();
                enter_scope();
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
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = param->name->getLiteral();
                    new_symbol->type_info = info;
                    push_symbol(param->name->getLiteral(), new_symbol);
                }
                auto body = _stmt->getBody();
                auto t = check_block_statement(body);
                if (_SEMANTIC_ERROR(t))
                    hasError = true;
                else if (_stmt->ret_type && t->baseType->id != _stmt->ret_type->id)
                {
                    raiseError(ErrorType::SEMANTIC_ERROR,
                               "unmatch return type:" + t->baseType->alias + ".expect type: " + _stmt->ret_type->alias,
                               _stmt->getToken().getLocation());
                    hasError = true;
                }
                if (hasError)
                    continue;
                if (!_stmt->ret_type)
                    _stmt->ret_type = t->baseType;
                funcDetail.return_type = t->baseType;
                typeInfo->detail = funcDetail;
                exit_scope();
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
        }
        exit_scope();
        if (hasError)
            MARK_ERROR;
        auto ret = typeManager::find(name);
        ret->detail = clsd;
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
        const auto &exp = expr->expression();
        return check_expression(exp);
    }
    TypeInfo *SemanticAnalyzer::check_import_statement(ImportStatement *stmt)
    {
        std::string abs_path;
        std::vector<std::string> search_paths;
        {
            std::string src_file = mLexer->getSourceFilename();
            size_t sep = src_file.find_last_of("/\\");
            std::string source_dir;
            if (sep != std::string::npos)
                source_dir = src_file.substr(0, sep);
            if (!source_dir.empty())
                search_paths.push_back(source_dir + "/" + stmt->getPath());
        }
        search_paths.push_back(stmt->getPath());
        for (const auto &sp : search_paths)
        {
            std::ifstream f(sp.c_str());
            if (f.good())
            {
                abs_path = sp;
                break;
            }
        }
        if (abs_path.empty())
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "cannot find imported file: '" + stmt->getPath() + "'",
                       stmt->getToken().getLocation());
            MARK_ERROR;
        }
        if (imported_files.count(abs_path))
            return typeManager::find("void");
        std::string source = read_file(abs_path);
        if (source.empty())
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "cannot read imported file: '" + abs_path + "'",
                       stmt->getToken().getLocation());
            MARK_ERROR;
        }
        imported_files.insert(abs_path);
        Lexer import_lexer(source, abs_path);
        Parser import_parser(&import_lexer);
        auto imported_program = import_parser.parse();
        if (!imported_program)
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "syntax error in imported file: '" + abs_path + "'",
                       stmt->getToken().getLocation());
            MARK_ERROR;
        }
        imported_programs.push_back(std::move(imported_program));
        Program *saved_program = imported_programs.back().get();
        SemanticAnalyzer import_sa(&import_lexer);
        import_sa.imported_files = imported_files;
        auto import_check = import_sa.collect_and_check_all_import_statement(saved_program);
        if (_SEMANTIC_ERROR(import_check))
        {
            raiseError(ErrorType::SEMANTIC_ERROR,
                       "error in imported file: '" + abs_path + "'",
                       stmt->getToken().getLocation());
            MARK_ERROR;
        }
        import_sa.enter_scope();
        import_sa.init_builtin_funcs();
        for (auto &s : saved_program->stmts)
        {
            if (auto *ims = dynamic_cast<ImportStatement *>(s))
                continue;
            auto t = import_sa.check_statement(s);
            if (_SEMANTIC_ERROR(t))
            {
                raiseError(ErrorType::SEMANTIC_ERROR,
                           "semantic error in imported file: '" + abs_path + "'",
                           stmt->getToken().getLocation());
                MARK_ERROR;
            }
        }
        if (import_sa.st->current_)
        {
            for (auto &[sym_name, sym] : import_sa.st->current_->symbols_)
            {
                if (!find_symbol(sym_name))
                {
                    Symbol *new_sym = new Symbol();
                    new_sym->name = sym_name;
                    new_sym->type_info = sym->type_info;
                    push_symbol(sym_name, new_sym);
                }
            }
        }
        import_sa.exit_scope();
        imported_files = import_sa.imported_files;
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::collect_and_check_all_import_statement(Program *program)
    {
        auto &stmts = program->stmts;
        for (auto &stmt : stmts)
        {
            if (auto *ims = dynamic_cast<ImportStatement *>(stmt))
            {
                const TypeInfo *const cr = check_import_statement(ims);
                if (_SEMANTIC_ERROR(cr))
                    MARK_ERROR;
            }
        }
        return typeManager::find("void");
    }
    TypeInfo *SemanticAnalyzer::check_statement(Statement *stmt)
    {
        if (typeid(*stmt) == typeid(DeclareStatement))
            return check_declare_statement((DeclareStatement *)stmt);
        else if (typeid(*stmt) == typeid(BlockStatement))
            return check_block_statement((BlockStatement *)stmt);
        else if (typeid(*stmt) == typeid(ReturnStatement))
            return check_return_statement((ReturnStatement *)stmt);
        else if (typeid(*stmt) == typeid(FuncDecl))
            return check_func_statement((FuncDecl *)stmt);
        else if (typeid(*stmt) == typeid(ClassLiteral))
            return check_class_statement((ClassLiteral *)stmt);
        else if (typeid(*stmt) == typeid(ExpressionStatement))
            return check_expression_statement((ExpressionStatement *)stmt);
        else if (auto *st = dynamic_cast<ForLoop *>(stmt))
            return check_for_loop_statement(st);
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
        enter_scope();
        for (auto &var : vars)
        {
            auto &id = var->var;
            auto &val = var->value;
            if (!val)
            {
                if (var->external_variable)
                {
                    auto evt = check_ident(id.get());
                    if (_SEMANTIC_ERROR(evt))
                        MARK_ERROR;
                }
                else
                {
                    Symbol *new_symbol = new Symbol();
                    new_symbol->name = id->getLiteral();
                    new_symbol->type_info = typeManager::find("undefined");
                    push_symbol(id->getLiteral(), new_symbol);
                }
                continue;
            }
            auto vt = check_expression(val.get());
            if (_SEMANTIC_ERROR(vt))
                MARK_ERROR;
            if (var->external_variable)
            {
                auto evt = check_ident(id.get());
                if (_SEMANTIC_ERROR(evt))
                    MARK_ERROR;
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
                if (id_type == "")
                    id->setType(vt->baseType);
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
                Symbol *new_symbol = new Symbol();
                new_symbol->name = id->getLiteral();
                new_symbol->type_info = typeManager::find(id->getType());
                push_symbol(id->getLiteral(), new_symbol);
            }
        }
        if (condtion)
        {
            auto cr = check_expression(condtion);
            if (_SEMANTIC_ERROR(cr))
                MARK_ERROR;
        }
        for (auto &action : actions)
        {
            auto ar = check_expression(action.get());
            if (_SEMANTIC_ERROR(ar))
                MARK_ERROR;
        }
        auto br = check_block_statement(body);
        if (_SEMANTIC_ERROR(br))
            MARK_ERROR;
        exit_scope();
        return typeManager::find("void");
    }
    int SemanticAnalyzer::check(Program *program)
    {
        auto stmts = program->stmts;
        enter_scope();
        init_builtin_funcs();

        const TypeInfo *const import_check = collect_and_check_all_import_statement(program);
        if (_SEMANTIC_ERROR(import_check))
            return 0;

        for (auto &imported : imported_programs)
        {
            for (auto &s : imported->stmts)
            {
                if (dynamic_cast<ImportStatement *>(s))
                    continue;
                program->append(s);
            }
        }

        for (auto &stmt : stmts)
        {
            if (auto *ims = dynamic_cast<ImportStatement *>(stmt))
                continue;
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
            auto name = it->first->getLiteral();
            if (find_symbol(name))
            {
                raiseError(ErrorType::SEMANTIC_ERROR, "redefined symbol: " + name, it->first->getToken().getLocation());
                hasError = true;
            }
            auto type = it->first->getType();
            auto expr = it->second;
            TypeInfo *right_type = nullptr;
            if (expr)
            {
                right_type = check_expression(expr);
                if (right_type == _errorType)
                    hasError = true;
                else if (type_to_string(right_type->baseType) != type)
                {
                    if (type == "") {}
                    else if (is_number(right_type->baseType) && is_number(string_to_type(type))) {}
                    else
                    {
                        raiseError(ErrorType::SEMANTIC_ERROR,
                                   "can't convert type '" + right_type->baseType->alias + "' to type '" + type + "'",
                                   it->second->getToken().getLocation());
                        hasError = true;
                    }
                }
            }
            else if (type != "")
                right_type = typeManager::find(type);
            else
                right_type = typeManager::find("undefined");
            Symbol *new_symbol = new Symbol();
            new_symbol->name = name;
            new_symbol->type_info = right_type;
            it->first->setType(right_type->baseType);
            push_symbol(name, new_symbol);
        }
        if (hasError)
            MARK_ERROR;
        return typeManager::find("void");
    }
}