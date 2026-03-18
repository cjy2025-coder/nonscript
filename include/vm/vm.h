#pragma once
#include "frontend/parser.h"
#include "frontend/ast.h"
#include "enviroment.h"
#include <sstream>
#include <fstream>
#include <filesystem>
namespace ns
{
    enum class TypeRank
    {
        Int8,
        Int16,
        Int32,
        Int64,
        Float32,
        Float64
    };
    TypeRank getTypeRank(const Number *num);
    std::shared_ptr<Number> promoteTo(const Number *num, TypeRank targetRank);
    std::pair<std::shared_ptr<Number>, std::shared_ptr<Number>>
    promoteTypes(const Number *left, const Number *right);
    class VM
    {
    private:
        Parser *parser = NULL;
        mutable std::shared_ptr<Enviroment> env;
        std::string dir = "";
    private:
        bool is_error(const Object *obj)
        {
            return typeid(*obj) == typeid(Error);
        }
        bool is_logic_operator(std::string op)
        {
            return op == "||" || op == "or" || op == "&&" || op == "and";
        }

        bool is_true(const Object *obj);

    public:
        VM() : env(new Enviroment()) {}
        std::shared_ptr<Object> eval(std::string source);
        std::shared_ptr<Object> eval(std::string source, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval(const AstNode *node, std::shared_ptr<Enviroment> &env_);

    private:
        bool isPrefixOp(std::string op)
        {
            return op == "+" || op == "-" || op == "++" || op == "--" || op == "!";
        }

    private:
        std::shared_ptr<Object> eval_program(std::vector<Statement *> stmts, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_statements(BlockStatement *bs, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_for_loop(const ForLoop *stmt, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_throw_statement(const ThrowStatement *stmt, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_try_catch_statement(const TryCatchStatement *stmt, std::shared_ptr<Enviroment> &env_);
        std::vector<std::shared_ptr<Object>> eval_expressions(const std::vector<std::unique_ptr<Expression>> &args, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> apply_func(const Object *f, std::vector<std::shared_ptr<Object>> &args);
        std::shared_ptr<Enviroment> extend_func_env(Func *fn, std::vector<std::shared_ptr<Object>> &args);
        std::shared_ptr<Object> eval_prefix_expression(const std::string &op, const Object *right, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_bang_operator_expression(const Object *right);
        std::shared_ptr<Object> eval_infix_expression(const Object *left, const std::string &op, const Object *right);
        std::shared_ptr<Object> eval_number_infix_expression(const Number *left, const std::string op, const Number *right);
        std::shared_ptr<Object> eval_boolean_infix_expression(const Object *left, const std::string op, const Object *right);
        std::shared_ptr<Object> eval_string_infix_expression(const Object *left, const std::string op, const Object *right);
        std::shared_ptr<Object> eval_assign_expression(Expression *left, Expression *right, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_index_expression(Object *arr, Object *index);
        std::shared_ptr<Object> eval_if_expression(IfExpression *ie, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_until_expression(const Expression *condition, const BlockStatement *body, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_while_expression(Expression *condition, BlockStatement *body, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_logic_expression(Expression *left, const std::string &op, Expression *right, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_new_expression(const NewExpression *e, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_point_expression(Expression *left, Expression *right, std::shared_ptr<Enviroment> &env_);
        std::shared_ptr<Object> eval_switch_expression(const SwitchExpression *expr, std::shared_ptr<Enviroment> &env_);
    };



    TypeRank getTypeRank(const Number *num)
    {
        switch (num->getType())
        {
        case OBJ_INT8:
            return TypeRank::Int8;
        case OBJ_INT16:
            return TypeRank::Int16;
        case OBJ_INT32:
            return TypeRank::Int32;
        case OBJ_INT64:
            return TypeRank::Int64;
        case OBJ_FLOAT32:
            return TypeRank::Float32;
        case OBJ_FLOAT64:
            return TypeRank::Float64;
        default:
            return TypeRank::Int32; // 默认
        }
    }
    std::pair<std::shared_ptr<Number>, std::shared_ptr<Number>>
    promoteTypes(const Number *left, const Number *right)
    {
        auto leftRank = getTypeRank(left);
        auto rightRank = getTypeRank(right);
        auto targetRank = std::max(leftRank, rightRank);

        return {promoteTo(left, targetRank), promoteTo(right, targetRank)};
    }
    bool VM::is_true(const Object *obj)
    {
        if (!obj)
            return false;
        else if (obj == null.get())
            return false;
        else if (obj == True.get())
            return true;
        else if (obj == False.get())
            return false;
        else if (auto *value = dynamic_cast<const Int8 *>(obj))
        {
            return value->getValue() != 0;
        }
        else if (auto *value = dynamic_cast<const Int16 *>(obj))
        {
            return value->getValue() != 0;
        }
        else if (auto *value = dynamic_cast<const Int32 *>(obj))
        {
            return value->getValue() != 0;
        }
        else if (auto *value = dynamic_cast<const Int64 *>(obj))
        {
            return value->getValue() != 0;
        }
        else if (auto *value = dynamic_cast<const Float32 *>(obj))
        {
            return value->getValue() != 0.0f;
        }
        else if (auto *value = dynamic_cast<const Float64 *>(obj))
        {
            return value->getValue() != 0.0f;
        }
        else
            return true;
    }
    std::shared_ptr<Number> promoteTo(const Number *num, TypeRank targetRank)
    {
        if (num == nullptr)
            return nullptr;

        // 如果已经是目标类型，直接转换返回
        if (getTypeRank(num) == targetRank)
        {
            // 需要根据具体类型进行转换
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Int8>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Int16>(static_cast<const Int16 *>(num)->getValue());
            case OBJ_INT32:
                return std::make_shared<Int32>(static_cast<const Int32 *>(num)->getValue());
            case OBJ_INT64:
                return std::make_shared<Int64>(static_cast<const Int64 *>(num)->getValue());
            case OBJ_FLOAT32:
                return std::make_shared<Float32>(static_cast<const Float32 *>(num)->getValue());
            case OBJ_FLOAT64:
                return std::make_shared<Float64>(static_cast<const Float64 *>(num)->getValue());
            default:
                return nullptr;
            }
        }

        // 类型提升逻辑
        switch (targetRank)
        {
        case TypeRank::Int8:
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Int8>(static_cast<const Int8 *>(num)->getValue());
            // 其他类型到Int8可能丢失精度，需要谨慎处理
            default:
                return nullptr; // 或者抛出错误
            }

        case TypeRank::Int16:
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Int16>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Int16>(static_cast<const Int16 *>(num)->getValue());
            default:
                return nullptr;
            }

        case TypeRank::Int32:
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Int32>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Int32>(static_cast<const Int16 *>(num)->getValue());
            case OBJ_INT32:
                return std::make_shared<Int32>(static_cast<const Int32 *>(num)->getValue());
            default:
                return nullptr;
            }

        case TypeRank::Int64:
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Int64>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Int64>(static_cast<const Int16 *>(num)->getValue());
            case OBJ_INT32:
                return std::make_shared<Int64>(static_cast<const Int32 *>(num)->getValue());
            case OBJ_INT64:
                return std::make_shared<Int64>(static_cast<const Int64 *>(num)->getValue());
            default:
                return nullptr;
            }

        case TypeRank::Float32:
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Float32>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Float32>(static_cast<const Int16 *>(num)->getValue());
            case OBJ_INT32:
                return std::make_shared<Float32>(static_cast<const Int32 *>(num)->getValue());
            case OBJ_INT64:
                return std::make_shared<Float32>(static_cast<const Int64 *>(num)->getValue());
            case OBJ_FLOAT32:
                return std::make_shared<Float32>(static_cast<const Float32 *>(num)->getValue());
            default:
                return nullptr;
            }

        case TypeRank::Float64:
            // 所有类型都可以提升到Float64
            switch (num->getType())
            {
            case OBJ_INT8:
                return std::make_shared<Float64>(static_cast<const Int8 *>(num)->getValue());
            case OBJ_INT16:
                return std::make_shared<Float64>(static_cast<const Int16 *>(num)->getValue());
            case OBJ_INT32:
                return std::make_shared<Float64>(static_cast<const Int32 *>(num)->getValue());
            case OBJ_INT64:
                return std::make_shared<Float64>(static_cast<const Int64 *>(num)->getValue());
            case OBJ_FLOAT32:
                return std::make_shared<Float64>(static_cast<const Float32 *>(num)->getValue());
            case OBJ_FLOAT64:
                return std::make_shared<Float64>(static_cast<const Float64 *>(num)->getValue());
            default:
                return nullptr;
            }
        }

        return nullptr;
    }
    std::shared_ptr<Object> VM::eval(std::string source, std::shared_ptr<Enviroment> &env_)
    {
        ns::Lexer lexer(source, "example.ss");
        ns::Token token;
        std::vector<ns::Token> tokens;
        while ((token = lexer.scan()).getType() != ns::TokenType::END)
        {
            tokens.push_back(token);
        }
        tokens.push_back(token);
        ns::Parser parser(tokens);
        parser.setLexer(&lexer);
        auto root = parser.parse();
        /*语法解析中出错*/
        if (root == NULL)
        {
            return std::make_shared<Error>(parser.what()->whats());
        }
        return eval(root.get(), env_);
    }
    std::shared_ptr<Object> VM::eval_for_loop(const ForLoop *stmt, std::shared_ptr<Enviroment> &env_)
    {
        auto &vars = stmt->getVariables();
        auto condition = stmt->getCondtion();
        auto &actions = stmt->getActions();
        auto body = stmt->getBody();
        auto extend_env = std::make_shared<Enviroment>(env_);
        for (const auto &var : vars)
        {
            auto obj = eval(var->value.get(), extend_env);
            if (is_error(obj.get()))
            {
                return obj;
            }
            extend_env->set(var->var->getLiteral(), obj);
        }
        while (true)
        {
            auto con = eval(condition, extend_env);
            if (is_error(con.get()))
            {
                return con;
            }
            try
            {
                if (!is_true(con.get()))
                {
                    break;
                }
            }
            catch (Exception e)
            {
            }
            auto temp = eval(body, extend_env);
            if (is_error(temp.get()))
            {
                return temp;
            }
            for (const auto &action : actions)
            {
                auto _ = eval(action.get(), extend_env);
                if (is_error(_.get()))
                {
                    return _;
                }
            }
        }
        return null;
    }
    std::shared_ptr<Object> VM::eval_throw_statement(const ThrowStatement *stmt, std::shared_ptr<Enviroment> &env_)
    {
        auto expr = stmt->getException();
        auto new_exception = eval(expr, env_);
        if (is_error(new_exception.get()))
        {
            return new_exception;
        }
        return std::make_shared<ExceptionValue>(new_exception);
    }
    std::shared_ptr<Object> VM::eval_try_catch_statement(const TryCatchStatement *stmt, std::shared_ptr<Enviroment> &env_)
    {
        auto try_body = stmt->getTryBody();
        auto exception = stmt->getException();
        auto exception_body = stmt->getExceptionBody();

        auto try_result = eval(try_body, env_);
        if (is_error(try_result.get()))
        {
            return try_result;
        }
        if (auto *_ = dynamic_cast<const ExceptionValue *>(try_result.get()))
        {
            auto extend_env = std::make_shared<Enviroment>(env_);
            extend_env->set(exception->getLiteral(), _->get_value());
            auto exception_result = eval(exception_body, extend_env);
            return exception_result;
        }
        else
        {
            return null;
        }
    }
    std::shared_ptr<Object> VM::eval_switch_expression(const SwitchExpression *expr, std::shared_ptr<Enviroment> &env_)
    {
        auto value = expr->getExpr();
        auto &cases = expr->getCases();
        auto default_case = expr->getDefaultCase();

        auto value_ = eval(value, env_);
        if (is_error(value_.get()))
        {
            return value_;
        }

        for (const auto &_ : cases)
        {
            auto &&cas = _->mcase;
            auto case_value = eval(cas.get(), env_);
            if (Object::equal(value_, case_value))
            {
                auto &&body = _->mbody;
                return eval(body.get(), env_);
            }
        }
        if (!default_case)
            return null;
        const auto &body_ = default_case->mbody;
        return eval(body_.get(), env_);
    }
    std::shared_ptr<Object> VM::eval(std::string source)
    {
        ns::Lexer lexer(source, "example.ss");
        ns::Token token;
        std::vector<ns::Token> tokens;
        while ((token = lexer.scan()).getType() != ns::TokenType::END)
        {
            tokens.push_back(token);
        }
        tokens.push_back(token);
        ns::Parser parser(tokens);
        parser.setLexer(&lexer);
        auto root = parser.parse();
        /*语法解析中出错*/
        if (root == NULL)
        {
            return std::make_shared<Error>(parser.what()->whats());
        }
        return eval(root.get(), env);
    }

    std::shared_ptr<Object> VM::eval(const AstNode *node, std::shared_ptr<Enviroment> &env_)
    {
        if (!node)
        {
            return null;
        }
        /*对基本数据类型的处理*/
        if (auto *val = dynamic_cast<const I8Literal *>(node))
        {
            return std::make_shared<Int8>(val->getValue());
        }
        else if (auto *val = dynamic_cast<const I16Literal *>(node))
        {
            return std::make_shared<Int16>(val->getValue());
        }
        else if (auto *val = dynamic_cast<const I32Literal *>(node))
        {
            return std::make_shared<Int32>(val->getValue());
        }
        else if (auto *val = dynamic_cast<const I64Literal *>(node))
        {
            return std::make_shared<Int64>(val->getValue());
        }
        else if (auto *val = dynamic_cast<const Float32Literal *>(node))
        {
            return std::make_shared<Float32>(val->getValue());
        }
        else if (auto *val = dynamic_cast<const Float64Literal *>(node))
        {
            return std::make_shared<Float64>(val->getValue());
        }
        else if (typeid(*node) == typeid(StringLiteral))
        {
            StringLiteral *val = (StringLiteral *)node;
            return std::make_shared<String>(val->value());
        }
        else if (typeid(*node) == typeid(NullLiteral))
        {
            return null;
        }
        else if (typeid(*node) == typeid(BoolLiteral))
        {
            BoolLiteral *val = (BoolLiteral *)node;
            return val->value() ? True : False;
        }
        else if (typeid(*node) == typeid(ClassLiteral))
        {
            ClassLiteral *cl = (ClassLiteral *)node;
            // std::cout<<cl->toString()<<std::endl;
            std::shared_ptr<Enviroment> class_env = std::make_shared<Enviroment>(env_);
            std::shared_ptr<Class> r = std::make_shared<Class>(cl, class_env);
            r->getEnv()->setOuter(cl->getLiteral(), r);
            return r;
        }
        else if (typeid(*node) == typeid(ArrayLiteral))
        {
            ArrayLiteral *al = (ArrayLiteral *)node;
            auto elems = eval_expressions(al->getElems(), env_);
            if (!elems.empty() && is_error(elems[0].get()))
                return elems[0];
            return std::make_shared<Array>(elems);
        }
        /*对简单结点的处理*/
        else if (typeid(*node) == typeid(Ident))
        {
            Ident *val = (Ident *)node;
            auto item = env_->get(val->getLiteral());
            if (item != NULL)
                return item;
            auto sys = BUILTINS.find(val->getLiteral());
            if (sys != BUILTINS.end())
                return std::make_shared<Builtin>(sys->second);
            return std::make_shared<Error>("compile error,unknown identifier `" + val->getLiteral() + "`");
        }

        else if (typeid(*node) == typeid(Program))
        {
            Program *val = (Program *)node;
            if (val->Empty())
                return null;
            return eval_program(val->stmts, env_);
        }

        else if (typeid(*node) == typeid(BlockStatement))
        {
            BlockStatement *val = (BlockStatement *)node;
            return eval_statements(val, env_);
        }
        else if (typeid(*node) == typeid(FuncDecl))
        {
            FuncDecl *val = (FuncDecl *)node;
            std::shared_ptr<Func> fn = NULL;
            fn = std::make_shared<Func>(val->getParams(), val->getBody(), env_);
            env_->set(val->getLiteral(), fn);
            return fn;
        }
        else if (typeid(*node) == typeid(ExpressionStatement))
        {
            ExpressionStatement *val = (ExpressionStatement *)node;
            return eval(val->expression(), env_);
        }

        else if (typeid(*node) == typeid(ReturnStatement))
        {
            ReturnStatement *val = (ReturnStatement *)node;
            auto e = eval(val->getExpression(), env_);
            if (is_error(e.get()))
                return e;
            return std::make_shared<ReturnValue>(e);
        }
        else if (auto *stmt = dynamic_cast<const ThrowStatement *>(node))
        {
            return eval_throw_statement(stmt, env_);
        }
        else if (auto *stmt = dynamic_cast<const TryCatchStatement *>(node))
        {
            return eval_try_catch_statement(stmt, env_);
        }
        else if (auto *stmt = dynamic_cast<const ForLoop *>(node))
        {
            return eval_for_loop(stmt, env_);
        }
        else if (typeid(*node) == typeid(DeclareStatement))
        {
            DeclareStatement *val = (DeclareStatement *)node;
            for (auto it = val->vars.begin(); it != val->vars.end(); it++)
            {
                if (it->second == NULL)
                {
                    env_->set(it->first->getLiteral(), null);
                }
                else
                {
                    auto e = eval(it->second, env_);
                    if (is_error(e.get()))
                        return e;
                    env_->set(it->first->getLiteral(), e);
                }
            }
            return null;
        }

        else if (typeid(*node) == typeid(IndexExpression))
        {
            IndexExpression *idxe = (IndexExpression *)node;
            auto arr = eval(idxe->getLeft(), env_);
            if (is_error(arr.get()))
                return arr;
            auto idx = eval(idxe->getIndex(), env_);
            if (is_error(idx.get()))
                return idx;
            return eval_index_expression(arr.get(), idx.get());
        }
        else if (typeid(*node) == typeid(LambdaExpr))
        {
            LambdaExpr *le = (LambdaExpr *)node;
            std::shared_ptr<Func> fn = std::make_shared<Func>(le->getParams(), le->getBody(), env_);
            return fn;
        }
        else if (typeid(*node) == typeid(CallExpression))
        {
            CallExpression *call = (CallExpression *)node;
            auto func_lite = call->getFunc();
            std::shared_ptr<Object> func = eval(func_lite, env_);
            if (is_error(func.get()))
                return func;
            auto args = eval_expressions(call->getArgs(), env_);
            if (!args.empty() && is_error(args[0].get()))
                return args[0];
            return apply_func(func.get(), args);
        }
        else if (typeid(*node) == typeid(InfixExpression))
        {
            InfixExpression *e = (InfixExpression *)node;
            Expression *left = e->getLeft();
            Expression *right = e->getRight();
            auto op = e->getOperator();
            if (op == ".")
                return eval_point_expression(left, right, env_);
            else if (op == "=")
            {
                return eval_assign_expression(left, right, env_);
            }
            else if (is_logic_operator(op))
            {
                return eval_logic_expression(left, op, right, env_);
            }
            else
            {
                auto right_ = eval(right, env_);
                if (is_error(right_.get()))
                    return right_;
                auto left_ = eval(left, env_);
                if (is_error(left_.get()))
                    return left_;
                return eval_infix_expression(left_.get(), op, right_.get());
            }
        }
        else if (typeid(*node) == typeid(PrefixExpression))
        {
            PrefixExpression *ps = (PrefixExpression *)node;
            auto right = eval(ps->getRight(), env_);
            auto op = ps->getOperator();
            if (is_error(right.get()))
                return right;
            return eval_prefix_expression(op, right.get(), env_);
        }
        else if (typeid(*node) == typeid(NewExpression))
        {
            NewExpression *expr = (NewExpression *)node;
            return eval_new_expression(expr, env_);
        }
        else if (typeid(*node) == typeid(WhileExpression))
        {
            WhileExpression *we = (WhileExpression *)node;
            auto condition = we->getCondition();
            auto body = we->getBody();
            return eval_while_expression(condition, body, env_);
        }
        else if (auto *val = dynamic_cast<const UntilExpression *>(node))
        {
            return eval_until_expression(val->getCondition(), val->getBody(), env_);
        }
        else if (auto *val = dynamic_cast<const SwitchExpression *>(node))
        {
            return eval_switch_expression(val, env_);
        }
        else if (typeid(*node) == typeid(IfExpression))
        {
            IfExpression *ife = (IfExpression *)node;
            return eval_if_expression(ife, env_);
        }
        else if (typeid(*node) == typeid(ImportStatement))
        {
            ImportStatement *is = (ImportStatement *)node;
            std::filesystem::path cp = std::filesystem::current_path();
            auto absolute_path = cp.concat("/samples/" + is->getPath());
            std::ifstream file(absolute_path);
            if (!file.is_open())
            {
                return std::make_shared<Error>("compile error,cannot find path `" + is->getPath() + "` .");
            }
            std::stringstream sstr;
            sstr << file.rdbuf();
            file.close();
            dir += is->getPath();
            auto rt = eval(sstr.str());
            if (is_error(rt.get()))
                return rt;
            dir = "";
            return null;
        }
        else if (typeid(*node) == typeid(ShortStatement))
        {
            ShortStatement *st = (ShortStatement *)node;
            if (st->getLiteral() == "break")
            {
                return Break;
            }
            else if (st->getLiteral() == "continue")
            {
                return Continue;
            }
            else
                return null;
        }

        else if (typeid(*node) == typeid(NewExpression))
        {
            NewExpression *ne = (NewExpression *)node;
            return eval_new_expression(ne, env_);
        }
        else
            return std::make_shared<Error>("unknown statement :" + node->toString());
    }

    std::shared_ptr<Object> VM::eval_point_expression(Expression *left, Expression *right, std::shared_ptr<Enviroment> &env_)
    {
        if (typeid(*right) != typeid(Ident))
        {
            return std::make_shared<Error>("compile error,usage <Object Name>.<Field name>");
        }
        auto left_ = eval(left, env_);
        if (is_error(left_.get()))
            return left_;
        else if (typeid(*left_) == typeid(Class))
        { // 访问类的静态成员
            Class *cls = (Class *)(left_.get());
            Ident *right_ = (Ident *)(right);
            std::string id = right_->getLiteral();
            auto temp = cls->getEnv()->getInside(id);
            if (temp == NULL)
            {
                return std::make_shared<Error>("compile error,unknown field name '" + id + "'");
            }
            else if (cls->getMemberType(id) != AccessLevel::Public)
            {
                return std::make_shared<Error>("compile error,un-public field name '" + id + "'");
            }
            return temp;
        }
        else if (typeid(*left_) == typeid(ClassInstance))
        {
            ClassInstance *cls = (ClassInstance *)(left_.get());
            Ident *right_ = (Ident *)(right);
            std::string id = right_->getLiteral();
            auto temp = cls->getEnv()->getInside(id);
            if (temp == NULL)
            {
                return std::make_shared<Error>("compile error,unknown field name '" + id + "'");
            }
            else if (cls->getMemberType(id) != AccessLevel::Public)
            {
                return std::make_shared<Error>("compile error,un-public field name '" + id + "'");
            }
            return temp;
        }
        else
        {
            return std::make_shared<Error>("compile error,usage <Object Name>.<Field name>");
        }
    }

    std::shared_ptr<Object> VM::eval_new_expression(const NewExpression *e, std::shared_ptr<Enviroment> &env_)
    {
        Expression *right = e->getRight();
        if (right == nullptr)
        {
            return std::make_shared<Error>("");
        }
        std::string id = "";
        if (typeid(*right) == typeid(Ident))
        {
            Ident *right_ = (Ident *)right;
            id = right_->getLiteral();
        }
        else if (typeid(*right) == typeid(CallExpression))
        {
            CallExpression *right_ = (CallExpression *)right;
            id = right_->getFunc()->getLiteral();
        }
        else
            return std::make_shared<Error>("compile error,expect an identifier or call-expresion after 'new'");

        auto cl = env_->get(id);
        if (cl == NULL)
        {
            return std::make_shared<Error>("compile error,unknown identifier '" + id + "'");
        }
        else if (typeid(*cl) != typeid(Class))
        {
            return std::make_shared<Error>("compile error,expect a class name after 'new'");
        }
        else // 对类进行实例化
        {
            Class *cls = (Class *)(cl.get());
            ClassLiteral *cl_lit = cls->classLiteral;

            std::shared_ptr<Enviroment> env = std::make_shared<Enviroment>(env_);
            //把父类的公有/保护成员加入环境
            // auto &parents=cl_lit->getBaseClasses();
            // for(const auto & parent : parents){
            //     auto _=parent->getLiteral();
            //     auto r=env_->get(_);
            //     if(!r || typeid(r) != typeid(Class*)){
            //         return std::make_shared<Error>("父类有错");
            //     }
            //     Class * p=(Class*)(r.get());
            //     env->merge(p->classLiteral->)
            // }
            std::shared_ptr<ClassInstance> r = std::make_shared<ClassInstance>(env);
            for (auto &mem : cl_lit->getMembers())
            {
                if (mem.type == MemberType::Method)
                    r->setMemberAccess(mem.declaration->getLiteral(), mem.access);
                else if (mem.type == MemberType::Field)
                {
                    DeclareStatement *ds = (DeclareStatement *)(mem.declaration.get());
                    for (auto &var : ds->vars)
                    {
                        r->setMemberAccess(var.first->getLiteral(), mem.access);
                    }
                }
                auto temp = eval(mem.declaration.get(), env);
                if (is_error(temp.get()))
                    return temp;
            }
            // std::cout<<r->toBuf();
            return r;
        }
    }

    std::shared_ptr<Object> VM::eval_logic_expression(Expression *left, const std::string &op, Expression *right, std::shared_ptr<Enviroment> &env_)
    {
        auto left_ = eval(left, env_);
        bool b1 = is_true(left_.get());
        if (op == "||" || op == "or")
        {
            if (b1)
                return True;
            auto right_ = eval(right, env_);
            bool b2 = is_true(right_.get());
            return b2 ? True : False;
        }
        else if (op == "&&" || op == "and")
        {
            if (!b1)
                return False;
            auto right_ = eval(right, env_);
            bool b2 = is_true(right_.get());
            return b2 ? True : False;
        }
        return std::make_shared<Error>("暂不支持的逻辑运算符：" + op);
    }
    std::shared_ptr<Object> VM::eval_until_expression(const Expression *condition, const BlockStatement *body, std::shared_ptr<Enviroment> &env_)
    {
        std::shared_ptr<Enviroment> extend_env = std::make_shared<Enviroment>(env_);
        while (1)
        {
            auto con = eval(condition, extend_env);
            if (is_error(con.get()))
                return con;
            if (is_true(con.get()))
                break;
            auto r = eval(body, extend_env);
            if (r == Break)
                break;
            else if (r == Continue)
                continue;
            else if ((typeid(*(r.get()))) == typeid(ReturnValue))
                return r;
        }
        return null;
    }
    std::shared_ptr<Object> VM::eval_while_expression(Expression *condition, BlockStatement *body, std::shared_ptr<Enviroment> &env_)
    {
        std::shared_ptr<Enviroment> extend_env = std::make_shared<Enviroment>(env_);
        while (1)
        {
            auto con = eval(condition, extend_env);
            if (is_error(con.get()))
                return con;
            if (!is_true(con.get()))
                break;
            auto r = eval(body, extend_env);
            if (r == Break)
                break;
            else if (r == Continue)
                continue;
            else if ((typeid(*(r.get()))) == typeid(ReturnValue))
                return r;
        }
        return null;
    }

    std::shared_ptr<Object> VM::eval_if_expression(IfExpression *ie, std::shared_ptr<Enviroment> &env_)
    {
        std::shared_ptr<Enviroment> extend_env = std::make_shared<Enviroment>(env_);
        if (ie->getCondition() == NULL)
            return null;
        auto condition = eval(ie->getCondition(), env_);
        if (is_error(condition.get()))
            return condition;
        if (is_true(condition.get()) && ie->getConsequence() != NULL)
        {
            return eval(ie->getConsequence(), extend_env);
        }
        else if (ie->getAlternative() != NULL)
        {
            extend_env = std::make_shared<Enviroment>(env_);
            return eval(ie->getAlternative(), extend_env);
        }

        return null;
    }

    std::shared_ptr<Object> VM::eval_index_expression(Object *arr, Object *index)
    {
        if (typeid(*arr) != typeid(Array) || typeid(*index) != typeid(Int64))
        {
            return std::make_shared<Error>("syntax error,it must be <Array Name><Integer> for an index expression!");
        }
        Array *ar = (Array *)arr;
        Int64 *i = (Int64 *)index;
        auto &elems = ar->getElems();
        int idx_ = i->getValue();
        if (idx_ < 0)
            idx_ += elems.size();
        if (idx_ < 0 || idx_ >= (int)elems.size())
        {
            return std::make_shared<Error>("compile error,Array index overflowed!");
        }
        return elems[idx_];
    }

    std::shared_ptr<Object> VM::eval_assign_expression(Expression *left, Expression *right, std::shared_ptr<Enviroment> &env_)
    {
        auto right_ = eval(right, env_);
        if (is_error(right_.get()))
            return right_;

        if (typeid(*left) == typeid(IndexExpression))
        {
            IndexExpression *ide = (IndexExpression *)left;
            std::string id = ide->getLeft()->getLiteral();
            auto e = env_->get(id);
            if (e == NULL)
            {
                return std::make_shared<Error>("compile error,unknown identifier '" + id + "' !");
            }
            Array *arr = (Array *)(e.get());
            auto idx = (Int64 *)eval(ide->getIndex(), env_).get();
            arr->setElem(right_, (int)idx->getValue());
        }
        else if (typeid(*left) == typeid(Ident))
        {
            Ident *id = (Ident *)left;
            std::string name = id->getLiteral();
            auto e = env_->get(name);
            if (e == NULL)
            {
                return std::make_shared<Error>("compile error,unknown identifier '" + name + "' !");
            }
            if (env_->type == ENV_STORE)
                env_->set(name, right_);
            else
                env_->setOuter(name, right_);
        }
        else if (typeid(*left) == typeid(InfixExpression))
        {
            InfixExpression *t = (InfixExpression *)left;
            if (t->getOperator() != ".")
                return std::make_shared<Error>("syntax error,usage for assign expression: <Ident>=<Expression> ,<PointExpression>=<Expression> or <IndexExpression>=<Expression> !");
            auto r = t->getRight();
            if (r == NULL || typeid(*r) != typeid(Ident))
            {
                return std::make_shared<Error>("compile error,usage <Object Name>.<Field name>");
            }
            auto l = eval(t->getLeft(), env_);
            if (is_error(l.get()))
                return l;
            else if (typeid(*l) == typeid(Class))
            {
                Class *obj = (Class *)(l.get());
                std::string id = r->getLiteral();
                auto temp = obj->getEnv()->getInside(id);
                if (temp == NULL)
                {
                    return std::make_shared<Error>("compile error,unknown field name '" + id + "'");
                }
                else if (obj->getMemberType(id) != AccessLevel::Public)
                {
                    return std::make_shared<Error>("compile error,un-public field name '" + id + "'");
                }
                obj->getEnv()->set(id, right_);
            }
            else if (typeid(*l) == typeid(ClassInstance))
            {
                ClassInstance *obj = (ClassInstance *)(l.get());
                // std::cout<<obj->toBuf();
                std::string id = r->getLiteral();
                auto temp = obj->getEnv()->getInside(id);
                if (temp == NULL)
                {
                    return std::make_shared<Error>("compile error,unknown field name '" + id + "'");
                }
                else if (obj->getMemberType(id) != AccessLevel::Public)
                {
                    return std::make_shared<Error>("compile error,un-public field name '" + id + "'");
                }
                obj->getEnv()->set(id, right_);
            }
            else
            {
                return std::make_shared<Error>("compile error,usage <Object Name>.<Field name>");
            }
        }
        else
        {
            return std::make_shared<Error>("syntax error,usage for assign expression: <Ident>=<Expression> ,<PointExpression>=<Expression> or <IndexExpression>=<Expression> !");
        }
        return right_;
    }

    std::shared_ptr<Object> VM::eval_string_infix_expression(const Object *left, const std::string op, const Object *right)
    {
        auto left_ = (String *)left;
        auto right_ = (String *)right;
        if (op == "+")
            return std::make_shared<String>(left_->getValue() + right_->getValue());
        else if (op == "==")
            return (left_->getValue() == right_->getValue() ? True : False);
        else if (op == "!=")
            return (left_->getValue() != right_->getValue() ? True : False);
        return std::make_shared<Error>("Unknown operator `" + op + "` between strings!");
    }

    std::shared_ptr<Object> VM::eval_boolean_infix_expression(const Object *left, const std::string op, const Object *right)
    {
        auto left_ = (Boolean *)left;
        auto right_ = (Boolean *)right;
        if (op == "+")
        {
            return left_->getValue() || right_->getValue() ? True : False;
        }
        else if (op == "*")
        {
            return left_->getValue() && right_->getValue() ? True : False;
        }
        else if (op == "==")
        {
            return left_->getValue() == right_->getValue() ? True : False;
        }
        else if (op == "!=")
        {
            return left_->getValue() != right_->getValue() ? True : False;
        }
        else
            return std::make_shared<Error>("compile error,Invalid operation between two boolean value !");
    }
    std::shared_ptr<Number> create_number(OBJECT type, auto value)
    {
        switch (type)
        {
        case OBJ_INT8:
            return std::make_shared<Int8>(static_cast<int8_t>(value));
        case OBJ_INT16:
            return std::make_shared<Int16>(static_cast<int16_t>(value));
        case OBJ_INT32:
            return std::make_shared<Int32>(static_cast<int32_t>(value));
        case OBJ_INT64:
            return std::make_shared<Int64>(static_cast<int64_t>(value));
        case OBJ_FLOAT32:
            return std::make_shared<Float32>(static_cast<float>(value));
        case OBJ_FLOAT64:
            return std::make_shared<Float64>(static_cast<double>(value));
        default:
            return nullptr;
        }
    }
    // 数值操作函数模板
    template <typename T>
    std::shared_ptr<Object> eval_numeric_operation(T left_val, const std::string &op, T right_val, OBJECT result_type)
    {
        using ValueType = T;

        // 算术运算
        if (op == "+")
        {
            return create_number(result_type, left_val + right_val);
        }
        else if (op == "-")
        {
            return create_number(result_type, left_val - right_val);
        }
        else if (op == "*")
        {
            return create_number(result_type, left_val * right_val);
        }
        else if (op == "/")
        {
            if (right_val == 0)
            {
                return std::make_shared<Error>("dividor is 0 !");
            }

            // 整数除法特殊处理：如果两个整数相除可能得到小数，自动提升为float64
            if constexpr (std::is_integral_v<ValueType>)
            {
                if (left_val % right_val != 0)
                {
                    // 有小数部分，提升为float64
                    double result = static_cast<double>(left_val) / static_cast<double>(right_val);
                    return std::make_shared<Float64>(result);
                }
                else
                {
                    // 整除，保持整数类型
                    return create_number(result_type, left_val / right_val);
                }
            }
            else
            {
                // 浮点数直接除
                return create_number(result_type, left_val / right_val);
            }
        }
        else if (op == "%")
        {
            if constexpr (std::is_integral_v<ValueType>)
            {
                if (right_val == 0)
                {
                    return std::make_shared<Error>("mod by 0 !");
                }
                return create_number(result_type, left_val % right_val);
            }
            else
            {
                return std::make_shared<Error>("mod operator not supported for float types");
            }
        }

        // 比较运算
        else if (op == ">")
        {
            return left_val > right_val ? True : False;
        }
        else if (op == ">=")
        {
            return left_val >= right_val ? True : False;
        }
        else if (op == "<")
        {
            return left_val < right_val ? True : False;
        }
        else if (op == "<=")
        {
            return left_val <= right_val ? True : False;
        }
        else if (op == "==")
        {
            return left_val == right_val ? True : False;
        }
        else if (op == "!=")
        {
            return left_val != right_val ? True : False;
        }

        return std::make_shared<Error>("Unknown operator `" + op + "`");
    }
    std::shared_ptr<Object> VM::eval_number_infix_expression(const Number *left, const std::string op, const Number *right)
    {
        auto [promotedLeft, _] = promoteTypes(left, right); // 处理过后，左右操作数类型相同
        switch (promotedLeft->getType())
        {
        case OBJ_INT8:
        {
            auto l_val = static_cast<const Int8 *>(left)->getValue();
            auto r_val = static_cast<const Int8 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_INT8);
        }
        case OBJ_INT16:
        {
            auto l_val = static_cast<const Int16 *>(left)->getValue();
            auto r_val = static_cast<const Int16 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_INT16);
        }
        case OBJ_INT32:
        {
            auto l_val = static_cast<const Int32 *>(left)->getValue();
            auto r_val = static_cast<const Int32 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_INT32);
        }
        case OBJ_INT64:
        {
            auto l_val = static_cast<const Int64 *>(left)->getValue();
            auto r_val = static_cast<const Int64 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_INT64);
        }
        case OBJ_FLOAT32:
        {
            auto l_val = static_cast<const Float32 *>(left)->getValue();
            auto r_val = static_cast<const Float32 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_FLOAT32);
        }
        case OBJ_FLOAT64:
        {
            auto l_val = static_cast<const Float64 *>(left)->getValue();
            auto r_val = static_cast<const Float64 *>(right)->getValue();
            return eval_numeric_operation(l_val, op, r_val, OBJ_FLOAT64);
        }
        default:
            return std::make_shared<Error>("Unsupported numeric type");
        }
    }

    std::shared_ptr<Object> VM::eval_infix_expression(const Object *left, const std::string &op, const Object *right)
    {
        if (dynamic_cast<const Number *>(left) != nullptr &&
            dynamic_cast<const Number *>(right) != nullptr)
        {
            auto left_num = dynamic_cast<const Number *>(left);
            auto right_num = dynamic_cast<const Number *>(right);
            return eval_number_infix_expression(left_num, op, right_num);
        }
        else if (typeid(*left) == typeid(Boolean) && typeid(*right) == typeid(Boolean))
        {
            return eval_boolean_infix_expression(left, op, right);
        }
        else if (typeid(*left) == typeid(String) && typeid(*right) == typeid(String))
        {
            return eval_string_infix_expression(left, op, right);
        }
        else
            return std::make_shared<Error>("Unknown infix expression!");
    }

    std::shared_ptr<Object> VM::eval_bang_operator_expression(const Object *right)
    {
        return is_true(right) ? False : True;
    }

    std::shared_ptr<Object> VM::eval_prefix_expression(
        const std::string &op,
        const Object *right,
        std::shared_ptr<Enviroment> &env_)
    {
        if (!isPrefixOp(op))
        {
            return std::make_shared<Error>("Unknown prefix operator `" + op + "` !");
        }
        if (op == "!")
            return eval_bang_operator_expression(right);
        int temp = 0;
        if (op == "-")
        {
            temp = -1;
        }
        else if (op == "+")
        {
            temp = 1;
        }
        else if (op == "++")
        {
            temp = 2;
        }
        else if (op == "--")
        {
            temp = -2;
        }
        if (auto *val = dynamic_cast<const Int8 *>(right))
        {
            if (temp == 2)
            {
                //    env_->set()
                return std::make_shared<Int8>(val->getValue() + 1);
            }
            else
                return std::make_shared<Int8>(temp * val->getValue());
        }
        else if (auto *val = dynamic_cast<const Int16 *>(right))
        {
            return std::make_shared<Int16>(temp * val->getValue());
        }
        else if (auto *val = dynamic_cast<const Int32 *>(right))
        {
            return std::make_shared<Int32>(temp * val->getValue());
        }
        else if (auto *val = dynamic_cast<const Int64 *>(right))
        {
            return std::make_shared<Int64>(temp * val->getValue());
        }
        else if (auto *val = dynamic_cast<const Float32 *>(right))
        {
            return std::make_shared<Float32>(temp * val->getValue());
        }
        else if (auto *val = dynamic_cast<const Float64 *>(right))
        {
            return std::make_shared<Float64>(temp * val->getValue());
        }
        else
            return std::make_shared<Error>("Unknown prefix operator `" + op + "` !");
    }

    std::shared_ptr<Enviroment> VM::extend_func_env(Func *fn, std::vector<std::shared_ptr<Object>> &args)
    {
        auto new_env = std::make_shared<Enviroment>(fn->getEnv());
        auto params = fn->getParams();
        for (size_t i = 0; i < params.size() && i < args.size(); i++)
        {
            new_env->set(params[i]->getLiteral(), args[i]);
        }
        return new_env;
    }

    std::shared_ptr<Object> VM::apply_func(const Object *f, std::vector<std::shared_ptr<Object>> &args)
    {
        std::shared_ptr<Object> obj = null;
        if (typeid(*f) == typeid(Func))
        {
            auto function = (Func *)f;
            auto extend_env = extend_func_env(function, args);
            obj = eval(function->getBody(), extend_env);
        }
        else if (typeid(*f) == typeid(Builtin))
        {
            Builtin *bt = (Builtin *)f;
            obj = bt->run(args);
        }

        if (obj == NULL)
            return NULL;
        if (typeid(*obj) == typeid(ReturnValue))
        {
            ReturnValue *rv = (ReturnValue *)(obj.get());
            return rv->get_value();
        }
        return obj;
    }

    std::vector<std::shared_ptr<Object>> VM::eval_expressions(const std::vector<std::unique_ptr<Expression>> &args, std::shared_ptr<Enviroment> &env_)
    {
        std::vector<std::shared_ptr<Object>> ret;
        for (auto &arg : args)
        {
            auto val = eval(arg.get(), env_);
            if (is_error(val.get()))
                return {val};
            ret.emplace_back(val);
        }
        return ret;
    }

    std::shared_ptr<Object> VM::eval_statements(BlockStatement *bs, std::shared_ptr<Enviroment> &env_)
    {
        auto stmts_ = bs->value();
        std::shared_ptr<Object> result = null;
        for (size_t i = 0; i < stmts_.size(); i++)
        {
            result = eval(stmts_[i], env_); // 解析语句
            if (result == NULL)
            {
                continue;
            }
            if (typeid(*result) == typeid(Error) || typeid(*result) == typeid(Short))
            {
                return result;
            }
            else if (typeid(*result) == typeid(ReturnValue))
            {
                ReturnValue *ret = (ReturnValue *)(result.get());
                return ret->get_value();
            }
            else if (auto *value = dynamic_cast<const ExceptionValue *>(result.get()))
            {
                return result;
            }
        }
        return result;
    }
    std::shared_ptr<Object> VM::eval_program(std::vector<Statement *> stmts, std::shared_ptr<Enviroment> &env_)
    {
        std::shared_ptr<Object> obj = NULL;
        for (auto &stmt : stmts)
        {
            obj = eval(stmt, env_);
            if (obj == NULL)
                continue;
            if (typeid(*obj) == typeid(ReturnValue))
            {
                ReturnValue *rv = (ReturnValue *)(obj.get());
                return rv->get_value();
            }
            else if (typeid(*obj) == typeid(Error))
                return obj;
        }
        return obj;
    }
}