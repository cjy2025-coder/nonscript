#pragma once
#include <string>
#include <vector>
#include <memory>
#include "token.h"
#include "../type.h"
#include <cfloat>
#include <optional>
#include <iostream>
#include <map>
namespace ns
{
    /*抽象语法树基础结点*/
    class AstNode
    {
    public:
        virtual std::string toString() const = 0;
        virtual std::string getLiteral() const = 0;
    };

    /*表达式结点*/
    class Expression : public AstNode
    {
    protected:
        Token token;
        DataType type; // 表达式的最终类型
    public:
        Expression(const Token &t) : token(t) {}
        Expression() {}
        std::string getLiteral() const override
        {
            return token.getLiteral();
        }
        int getCol() const
        {
            return token.getLocation().col;
        }
        int getRow() const
        {
            return token.getLocation().line;
        }
        Token getToken() const
        {
            return token;
        }
        DataType getType() const
        {
            return type;
        }
        void setType(DataType type_)
        {
            type = type_;
        }
    };
    /*语句结点*/
    class Statement : public AstNode
    {
    protected:
        Token token;

    public:
        Statement() {}
        Statement(const Token &t) : token(t) {}
        std::string getLiteral() const override
        {
            return token.getLiteral();
        }
        int getCol() const
        {
            return token.getLocation().col;
        }
        int getRow() const
        {
            return token.getLocation().line;
        }
        Token getToken() const
        {
            return token;
        }
    };

    class Ident : public Expression
    {
        std::string type = "any";

    public:
        Ident(const Token &t) : Expression(t) {}
        void setType(const std::string &type_)
        {
            type = type_;
        }
        std::string toString() const override
        {
            return getLiteral() + ": " + type;
        }
        std::string getType() const
        {
            return type;
        }
    };

    class NumericLiteral : public Expression
    {
    public:
        NumericLiteral(const Token &t)
            : Expression(t) {}

        virtual ~NumericLiteral() = default;
    };

    class Float32Literal;
    class Float64Literal;

    class Float32Literal : public NumericLiteral
    {
    private:
        float value;

    public:
        Float32Literal(const Token &t)
            : NumericLiteral(t),
              value(std::stof(t.getLiteral()))
        {
            validateRange(); // 校验范围
        }

        std::string toString() const override
        {
            return std::to_string(value) + "f32"; // 如 "3.14f32"
        }

        float getValue() const { return value; }

        std::unique_ptr<Float64Literal> convertToF64()
        {
            return std::make_unique<Float64Literal>(getToken());
        }

    private:
        void validateRange()
        {
            if (value < -FLT_MAX || value > FLT_MAX)
            {
                throw std::overflow_error("Float32 value out of range");
            }
        }
    };

    class Float64Literal : public NumericLiteral
    {
    private:
        double value;

    public:
        Float64Literal(const Token &t)
            : NumericLiteral(t),
              value(std::stod(t.getLiteral())) {}

        std::string toString() const override
        {
            return std::to_string(value); // 如 "3.1415926535"
        }

        double getValue() const { return value; }
        std::unique_ptr<Float32Literal> convertToF32()
        {
            return std::make_unique<Float32Literal>(getToken());
        }
    };

    class IntegerLiteral : public NumericLiteral
    {
    protected:
        explicit IntegerLiteral(const Token &t)
            : NumericLiteral(t) {}
    };

    class I8Literal;
    class I16Literal;
    class I32Literal;
    class I64Literal;

    class I8Literal : public IntegerLiteral
    {
    private:
        int8_t value;

    public:
        // 构造函数：先校验，通过后再初始化value
        I8Literal(const Token &t) : IntegerLiteral(t)
        {
            // 1. 解析并校验范围（校验失败会直接抛出异常，终止构造）
            int64_t parsed_val = parseAndValidate(t.getLiteral());
            // 2. 校验通过后，安全赋值（此时parsed_val一定在int8范围内）
            value = static_cast<int8_t>(parsed_val);
        }

        // 重写toString，返回正确的字符串表示
        std::string toString() const override
        {
            return std::to_string(static_cast<int>(value)) + "i8"; // 转换为int避免符号扩展问题
        }

        // 获取值（返回int8_t）
        int8_t getValue() const { return value; }

        // 转换为其他整数类型
        std::unique_ptr<I16Literal> convertToI16()
        {
            return std::make_unique<I16Literal>(getToken());
        }
        std::unique_ptr<I32Literal> convertToI32()
        {
            return std::make_unique<I32Literal>(getToken());
        }
        std::unique_ptr<I64Literal> convertToI64()
        {
            return std::make_unique<I64Literal>(getToken());
        }

    private:
        // 单独的解析和校验函数：负责将字符串转换为int64_t并校验范围
        int64_t parseAndValidate(const std::string &literal)
        {
            int64_t val;
            try
            {
                // 使用stoll解析为int64_t，支持更大范围的整数（避免int范围限制）
                val = std::stoll(literal);
            }
            catch (const std::invalid_argument &e)
            {
                // 字符串无法解析为整数（如"abc"、"12.34"）
                throw std::invalid_argument("无法将 '" + literal + "' 解析为整数（int8）");
            }
            catch (const std::out_of_range &e)
            {
                // 字符串数值超出int64_t范围（极端大值）
                throw std::overflow_error("数值 '" + literal + "' 超出整数范围（int64），无法转换为int8");
            }

            // 校验是否在int8范围内（-128 到 127）
            if (val < INT8_MIN || val > INT8_MAX)
            {
                throw std::overflow_error("数值 '" + literal + "' 超出int8范围（" +
                                          std::to_string(INT8_MIN) + " 到 " +
                                          std::to_string(INT8_MAX) + "）");
            }

            return val;
        }
    };

    class I16Literal : public IntegerLiteral
    {
    private:
        int16_t value;

    public:
        I16Literal(const Token &t)
            : IntegerLiteral(t),
              value(static_cast<int16_t>(std::stoi(t.getLiteral())))
        {
            validateRange();
        }

        std::string toString() const override
        {
            return std::to_string(value) + "i16"; // 如 "42i8"
        }
        int16_t getValue() const { return value; }
        std::unique_ptr<I8Literal> convertToI8()
        {
            return std::make_unique<I8Literal>(getToken());
        }
        std::unique_ptr<I32Literal> convertToI32()
        {
            return std::make_unique<I32Literal>(getToken());
        }
        std::unique_ptr<I64Literal> convertToI64()
        {
            return std::make_unique<I64Literal>(getToken());
        }

    private:
        void validateRange()
        {
            int val = std::stoi(token.getLiteral());
            if (val < INT16_MIN || val > INT16_MAX)
            {
                throw std::overflow_error("i16 value out of range");
            }
        }
    };

    class I32Literal : public IntegerLiteral
    {
    private:
        int32_t value;

    public:
        I32Literal(const Token &t)
            : IntegerLiteral(t),
              value(static_cast<int32_t>(std::stoi(t.getLiteral())))
        {
            validateRange();
        }

        std::string toString() const override
        {
            return std::to_string(value) + "i32"; // 如 "42i8"
        }
        std::unique_ptr<I8Literal> convertToI8()
        {
            return std::make_unique<I8Literal>(getToken());
        }
        std::unique_ptr<I16Literal> convertToI16()
        {
            return std::make_unique<I16Literal>(getToken());
        }
        std::unique_ptr<I64Literal> convertToI64()
        {
            return std::make_unique<I64Literal>(getToken());
        }
        int32_t getValue() const { return value; }

    private:
        void validateRange()
        {
            int val = std::stoi(token.getLiteral());
            if (val < INT32_MIN || val > INT32_MAX)
            {
                throw std::overflow_error("i32 value out of range");
            }
        }
    };

    class I64Literal : public IntegerLiteral
    {
    private:
        int64_t value;

    public:
        I64Literal(const Token &t)
            : IntegerLiteral(t)
        {
            auto value_ = t.getLiteral();
            try
            {
                value = static_cast<int64_t>(std::stoi(value_));
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }

            validateRange();
        }

        std::string toString() const override
        {
            return std::to_string(value) + "i64"; // 如 "42i8"
        }

        int64_t getValue() const { return value; }
        std::unique_ptr<I16Literal> convertToI16()
        {
            return std::make_unique<I16Literal>(getToken());
        }
        std::unique_ptr<I32Literal> convertToI32()
        {
            return std::make_unique<I32Literal>(getToken());
        }
        std::unique_ptr<I8Literal> convertToI8()
        {
            return std::make_unique<I8Literal>(getToken());
        }

    private:
        void validateRange()
        {
            int val = std::stoi(token.getLiteral());
            if (val < INT64_MIN || val > INT64_MAX)
            {
                throw std::overflow_error("i64 value out of range");
            }
        }
    };

    // class FloatLiteral : public Expression
    // {
    // private:
    //     double value;
    // public:
    //     FloatLiteral(const Token &t, bool isVariable_ = true) : Expression(t), value(std::stof(t.getLiteral())), isVariable(isVariable_) {}
    //     std::string toString() const override
    //     {
    //         return getLiteral();
    //     }
    //     float get_value()
    //     {
    //         return value;
    //     }
    // };

    // class IntegerLiteral : public Expression
    // {
    // private:
    //     long long int value;

    // private:
    //     bool isVariable;

    // public:
    //     IntegerLiteral(const Token &t, bool isVariable_ = true) : Expression(t), value(std::stoi(t.getLiteral())), isVariable(isVariable_) {}
    //     std::string toString() const override
    //     {
    //         return getLiteral();
    //     }

    //     long long int get_value()
    //     {
    //         return value;
    //     }
    //     bool variable() const
    //     {
    //         return isVariable;
    //     }
    // };

    class StringLiteral : public Expression
    {
    private:
        bool isVariable;

    public:
        StringLiteral(const Token &t, bool isVariable_ = true) : Expression(t), isVariable(isVariable_) {}
        std::string toString() const override
        {
            return "\"" + getLiteral() + "\"";
        }
        std::string value()
        {
            return getLiteral();
        }
        bool variable() const
        {
            return isVariable;
        }
    };

    class BoolLiteral : public Expression
    {
    private:
        bool isVariable;

    public:
        BoolLiteral(const Token &t, bool isVariable_ = true) : Expression(t), isVariable(isVariable_) {}
        std::string toString() const override
        {
            return getLiteral();
        }
        bool value() const
        {
            return getLiteral() == "true";
        }
        bool variable() const
        {
            return isVariable;
        }
    };

    class NullLiteral : public Expression
    {
    public:
        NullLiteral(const Token &t) : Expression(t) {}
        NullLiteral() {}
        std::string toString() const override
        {
            return "null";
        }
    };

    // std::unique_ptr<NullLiteral>  AstNull;

    class PrefixExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> right;

    public:
        PrefixExpression(const Token &t) : Expression(t) {}
        PrefixExpression(const Token &t, std::unique_ptr<Expression> right_) : Expression(t)
        {
            right = std::move(right_);
        }
        Expression *getRight()
        {
            return right.get();
        }
        void setRight(Expression *e)
        {
            right.reset(e);
        }
        std::string getOperator()
        {
            return getLiteral();
        }
        std::string toString() const override
        {
            if (right == NULL)
                return "()";
            else
                return "(" + getLiteral() + " " + right->toString() + ")";
        }
    };

    class InfixExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> left, right;

    public:
        InfixExpression(const Token &t) : Expression(t) {}
        Expression *getRight() const
        {
            return right.get();
        }
        Expression *getLeft() const
        {
            return left.get();
        }
        void setRight(Expression *e)
        {
            right.reset(e);
        }
        void setLeft(Expression *e)
        {
            left.reset(e);
        }
        std::string getOperator()
        {
            return getLiteral();
        }
        std::string toString() const override
        {
            if (right == NULL || left == NULL)
                return "()";
            else
                return "( " + left->toString() + getLiteral() + right->toString() + " )";
        }
    };

    class ImportStatement : public Statement
    {
    public:
        ImportStatement(const Token &t) : Statement(t) {}
        std::string getPath()
        {
            return getLiteral();
        }
        std::string toString() const override
        {
            return "import \"" + getLiteral() + "\";\n";
        }
    };

    class BlockStatement : public Statement
    {
    private:
        std::vector<Statement *> stmts;

    public:
        BlockStatement() : Statement() {}
        BlockStatement(const Token &t) : Statement(t) {}
        void append(Statement *stmt)
        {
            stmts.emplace_back(stmt);
        }
        std::vector<Statement *> value()
        {
            return stmts;
        }
        std::string toString() const override
        {
            std::string ret = "";
            for (auto &i : stmts)
            {
                ret += i->toString();
            }
            return ret;
        }
        bool Empty() const
        {
            return stmts.empty();
        }
    };

    class ArrayLiteral : public Expression
    {
    private:
        bool isVariable;

    private:
        std::vector<std::unique_ptr<Expression>> elems;

    public:
        ArrayLiteral(const Token &t, bool isVariable_ = true) : Expression(t), isVariable(isVariable_) {}
        void setElems(std::vector<std::unique_ptr<Expression>> &es)
        {
            elems = std::move(es);
        }
        std::vector<std::unique_ptr<Expression>> &getElems()
        {
            return elems;
        }
        std::string toString() const override
        {
            std::string ret = "[";
            for (size_t i = 0; i < elems.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret += elems[i]->toString();
            }
            ret += "]";
            return ret;
        }
        bool variable() const
        {
            return isVariable;
        }
    };
    // 访问级别定义
    enum AccessLevel
    {
        Public,
        Private,
        Protected
    };
    // 成员类型分类
    enum MemberType
    {
        Field,
        Method,
        Constructor,
        Static
    };
    // 成员结构
    struct Member
    {
        AccessLevel access;
        MemberType type;
        std::unique_ptr<Statement> declaration;
        bool isStatic = false;

        // 构造器特定字段
        std::vector<std::string> ctorParams;
        std::unique_ptr<BlockStatement> ctorBody;
    };
    class ClassLiteral : public Statement
    {
    private:
        std::vector<std::unique_ptr<Ident>> baseClasses; // 父类
        std::vector<Member> members;                     // 类成员
    public:
        ClassLiteral(const Token &t) : Statement(t) {}

    public:
        // 继承相关
        void addBaseClass(std::unique_ptr<Ident> base)
        {
            baseClasses.push_back(std::move(base));
        }
        std::vector<std::unique_ptr<Ident>> &getBaseClasses()
        {
            return baseClasses;
        }
        // 构造函数添加
        void addConstructor(
            AccessLevel access,
            std::vector<std::string> params,
            std::unique_ptr<BlockStatement> body)
        {
            members.push_back(Member{
                access,
                MemberType::Constructor,
                nullptr, // declaration留空
                false,
                params,
                std::move(body)});
        }

        // 常规成员添加
        void addMember(
            AccessLevel access,
            MemberType type,
            std::unique_ptr<Statement> decl,
            bool isStatic = false)
        {
            members.push_back(Member{
                access,
                type,
                std::move(decl),
                isStatic});
        }

        const std::vector<Member> &getMembers()
        {
            return members;
        }

        std::string toString() const override
        {
            std::string ret = "class " + getLiteral();

            // 处理继承
            if (!baseClasses.empty())
            {
                ret += " extends ";
                for (size_t i = 0; i < baseClasses.size(); ++i)
                {
                    if (i > 0)
                        ret += ", ";
                    ret += baseClasses[i]->toString();
                }
            }

            ret += " {\n";

            // 处理成员
            for (const auto &m : members)
            {
                // 访问修饰符
                switch (m.access)
                {
                case AccessLevel::Public:
                    ret += "public ";
                    break;
                case AccessLevel::Private:
                    ret += "private ";
                    break;
                case AccessLevel::Protected:
                    ret += "protected ";
                    break;
                }

                // 静态修饰符
                if (m.isStatic)
                    ret += "static ";

                // 构造器特殊处理
                if (m.type == MemberType::Constructor)
                {
                    ret += "construct(";
                    for (size_t i = 0; i < m.ctorParams.size(); ++i)
                    {
                        if (i > 0)
                            ret += ", ";
                        ret += m.ctorParams[i];
                    }
                    ret += ") " + m.ctorBody->toString();
                }
                else
                {
                    ret += m.declaration->toString();
                }

                if (ret.back() != '\n')
                    ret += "\n";
            }

            return ret + "}\n";
        }
    };
    class IndexExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> index;

    public:
        IndexExpression() {}
        IndexExpression(const Token &t) : Expression(t) {}
        void setLeft(Expression *left_)
        {
            left.reset(left_);
        }
        void setIndex(Expression *index_)
        {
            index.reset(index_);
        }
        Expression *getLeft()
        {
            return left.get();
        }
        Expression *getIndex()
        {
            return index.get();
        }
        std::string toString() const override
        {
            std::string ret = left->toString();
            ret += "[ ";
            ret += index->toString();
            ret += " ]";
            return ret;
        }
    };
    // 具名函数和匿名函数都具有的内容
    struct FuncBase
    {
        std::vector<std::shared_ptr<Expression>> params;
        std::shared_ptr<BlockStatement> body;
        void setParams(std::vector<std::shared_ptr<Expression>> &params_)
        {
            params = params_;
        }
        int params_num()
        {
            return params.size();
        }
        void setBody(BlockStatement *body_)
        {
            body.reset(body_);
        }
        std::vector<std::shared_ptr<Expression>> getParams()
        {
            return params;
        }
        BlockStatement *getBody()
        {
            return body.get();
        }
        void appendParams(Expression *id)
        {
            params.emplace_back(id);
        }
    };
    // 具名函数的申明
    class FuncDecl : public Statement, public FuncBase
    {
    public:
        FuncDecl(const Token &t) : Statement(t) {} // 将函数名通过token传入

        std::string toString() const override
        {
            std::string ret = "func ";
            ret += getLiteral() + "(";
            for (size_t i = 0; i < params.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret.append(params[i]->toString());
            }
            ret += "){\n";
            ret += body->toString();
            ret += "}\n";
            return ret;
        }
    };
    // 匿名函数
    class LambdaExpr : public Expression, public FuncBase
    {
    private:
        static int lambda_count;
        std::string internalName;                     // 内部表示
        std::vector<std::shared_ptr<Ident>> captures; // 捕获列表
    public:
        LambdaExpr()
        {
        }
        LambdaExpr(const Token &t) : Expression(t)
        {
            internalName = "lambda$" + std::to_string(lambda_count);
            lambda_count++;
        }

    public:
        void capture(const std::shared_ptr<Ident> &var)
        {
            captures.push_back(var);
        }
        std::string toString() const override
        {
            std::string ret = "func(";
            for (size_t i = 0; i < params.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret.append(params[i]->toString());
            }
            ret += ")";
            if (!captures.empty())
            {
                ret += " [";
                for (auto &cap : captures)
                    ret += cap->toString() + ",";
                ret.back() = ']';
            }
            ret += "{\n" + body->toString() + "}";
            return ret;
        }
    };

    /*函数调用表达式*/
    class CallExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> func;
        std::vector<std::unique_ptr<Expression>> args;

    public:
        CallExpression(const Token &t) : Expression(t) {}
        CallExpression() {}
        void setFunc(Expression *e)
        {
            func.reset(e);
        }
        void setArgs(std::vector<std::unique_ptr<Expression>> &args_)
        {
            args = std::move(args_);
        }
        Expression *getFunc()
        {
            return func.get();
        }

        std::vector<std::unique_ptr<Expression>> &getArgs()
        {
            return args;
        }
        std::string toString() const override
        {
            if (func == NULL)
                return std::string();
            std::string ret = func->toString();
            ret += "(";
            for (size_t i = 0; i < args.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret += args[i]->toString();
            }
            ret += ")";
            return ret;
        }
    };

    class IfExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> conditon;
        std::unique_ptr<BlockStatement> consequence, alternative;

    public:
        IfExpression() {}
        IfExpression(const Token &t) : Expression(t) {}
        void setCondition(Expression *e)
        {
            conditon.reset(e);
        }
        void setConsequence(BlockStatement *bs)
        {
            consequence.reset(bs);
        }
        void setAlternative(BlockStatement *bs)
        {
            alternative.reset(bs);
        }
        Expression *getCondition() const
        {
            return conditon.get();
        }
        BlockStatement *getConsequence() const
        {
            return consequence.get();
        }
        BlockStatement *getAlternative() const
        {
            return alternative.get();
        }
        std::string toString() const override
        {
            return "if (" + conditon->toString() + "){\n" + consequence->toString() + "}\n" + (alternative ? "else {\n" + alternative->toString() + "}\n" : "");
        }
    };

    /*短语句，如break, continue等*/
    class ShortStatement : public Statement
    {
    public:
        ShortStatement(const Token &t) : Statement(t) {}
        std::string toString() const override
        {
            if (token.getType() == TokenType::BREAK)
                return "break;\n";
            else if (token.getType() == TokenType::CONTINUE)
                return "continue;\n";
        }
    };

    class DeclareStatement : public Statement
    {
    public:
        std::vector<std::pair<Ident*,Expression*>> vars;
        bool isVariable;
    public:
        DeclareStatement(const Token &t, bool isVariable_) : Statement(t), isVariable(isVariable_) {}
        std::vector<std::pair<Ident*,Expression*>> getVars() const
        {
            return vars;
        }
        void add(Ident *id, Expression *e)
        {
            vars.push_back({id,e});
        }
        std::string toString() const override
        {
            if (vars.empty())
                return "";
            std::string ret = isVariable ? "var " : "const ";
            for (auto it = vars.begin(); it != vars.end(); it++)
            {
                if (it != vars.begin())
                    ret.append(", ");
                ret += it->first->toString();
                if (it->second != NULL)
                {
                    ret += "= " + it->second->toString();
                }
            }
            return ret + ";\n";
        }
        bool variable() const
        {
            return isVariable;
        }
    };

    class ExpressionStatement : public Statement
    {
    private:
        std::unique_ptr<Expression> e;

    public:
        ExpressionStatement(const Token &t) : Statement(t) {}
        void setExpression(Expression *e_)
        {
            e.reset(e_);
        }
        Expression *expression()
        {
            return e.get();
        }
        std::string toString() const override
        {
            return e == NULL ? std::string() : e->toString();
        }
    };

    class NewExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> right;

    public:
        NewExpression(const Token &t) : Expression(t) {}

    public:
        void setRight(Expression *right_)
        {
            right.reset(right_);
        }
        Expression *getRight() const
        {
            return right.get();
        }
        std::string toString() const override
        {
            return "new " + right->toString();
        }
    };

    class WhileExpression : public Expression
    {
    protected:
        std::unique_ptr<Expression> condition;
        std::unique_ptr<BlockStatement> body;

    public:
        WhileExpression() {}
        WhileExpression(const Token &t) : Expression(t) {}
        void setCondtion(Expression *e)
        {
            condition.reset(e);
        }
        void setBody(BlockStatement *body_)
        {
            body.reset(body_);
        }
        Expression *getCondition() const
        {
            return condition.get();
        }
        BlockStatement *getBody() const
        {
            return body.get();
        }
        std::string toString() const override
        {
            std::string ret = "while (";
            ret += condition->toString() + "){\n";
            if (body != NULL)
                ret += body->toString() + "}\n";
            return ret;
        }
    };
    class UntilExpression : public WhileExpression
    {
    public:
        UntilExpression() {}
        UntilExpression(const Token &t) : WhileExpression(t) {}

    public:
        std::string toString() const override
        {
            std::string ret = "until (";
            ret += condition->toString() + "){\n";
            if (body != NULL)
                ret += body->toString() + "}\n";
            return ret;
        }
    };
    struct SwitchCase{
       std::unique_ptr<Expression> mcase;
       std::unique_ptr<BlockStatement> mbody;
    };
    struct DefaultSwitchCase{
       std::unique_ptr<BlockStatement> mbody;
    };
    class SwitchExpression : public Expression
    {
    private:
        std::unique_ptr<Expression> expr;
        std::vector<std::unique_ptr<SwitchCase>> cases;
        std::unique_ptr<DefaultSwitchCase> defaultCase;
    public:
        SwitchExpression() {}
        SwitchExpression(const Token &t) : Expression(t) {}
    public:
        void setExpr(std::unique_ptr<Expression> expr_){
            expr=std::move(expr_);
        }
        void setDefaultCase(std::unique_ptr<DefaultSwitchCase> defaultCase_){
            defaultCase=std::move(defaultCase_);
        }
        DefaultSwitchCase * getDefaultCase() const{
            return defaultCase.get();
        }
        void addCase(std::unique_ptr<SwitchCase> case_){
            cases.push_back(std::move(case_));
        }
        const std::vector<std::unique_ptr<SwitchCase>> & getCases() const{
            return cases;
        }
        Expression * getExpr() const{
            return expr.get();
        }
    public:
        std::string toString() const override
        {
            std::string ret = "";
            ret += "switch (" + expr->toString() + ") {\n";
            for(const auto & _case : cases){
                ret+=" case ";
                ret+=_case->mcase->toString();
                ret+=" : {\n";
                ret+=_case->mbody->toString();
                ret+=" }\n";
            }
            if(defaultCase){
               ret+=" default:";
               ret+=" {\n";
               ret+=defaultCase->mbody->toString();
               ret+=" }\n";
            }
            ret += "}\n";
            return ret;
        }
    };
    typedef struct LoopVariable{
       std::unique_ptr<Ident> var;
       std::unique_ptr<Expression> value;
    }*PLoopVariable;
    // typedef struct LoopAction{
    //    std::unique_ptr<ExpressionStatement> action;
    // }*LoopAction;
    
    class ForLoop: public Statement{
    private:
        std::unique_ptr<Expression> condtion;
        std::vector<std::unique_ptr<LoopVariable>> vars;
        std::vector<std::unique_ptr<Expression>> actions;
        std::unique_ptr<BlockStatement> body;
    public:
        void setConditon(std::unique_ptr<Expression> condtion_){
            condtion=std::move(condtion_);
        }
        void setBody(std::unique_ptr<BlockStatement> body_){
            body=std::move(body_);
        }
        void addVariable(std::unique_ptr<LoopVariable> var){
            vars.push_back(std::move(var));
        }
        void addAction(std::unique_ptr<Expression> action){
            actions.push_back(std::move(action));
        }
        Expression * getCondtion()const{
            return condtion.get();
        }
        BlockStatement * getBody()const{
            return body.get();
        }
        const std::vector<std::unique_ptr<LoopVariable>> & getVariables()const{
            return vars;
        }
        const std::vector<std::unique_ptr<Expression>> & getActions()const{
            return actions;
        }
    public:
        std::string toString() const override{
            std::string ret="";
            ret+="for(";
            for(size_t i=0;i < vars.size() ; i++){
                if(i){
                    ret+=", ";
                }
                ret+=vars[i]->var->toString();
                ret+="= ";
                ret+=vars[i]->value->toString();
            }
            ret+="; ";
            ret+=condtion->toString();
            ret+="; ";
            for(size_t i=0;i < actions.size() ; i++){
                if(i){
                    ret+=", ";
                }
                ret+=actions[i]->toString();
            }
            ret+="){\n";
            ret+=body->toString();
            ret+="}\n";
            return ret;
        }
    };
    class TryCatchStatement : public Statement
    {
    private:
        std::unique_ptr<BlockStatement> try_body;
        std::unique_ptr<BlockStatement> exception_body;
        std::unique_ptr<Ident> exception;
    public:
        TryCatchStatement() {}
        TryCatchStatement(const Token &token) : Statement(token) {}

    public:
        void setTryBody(std::unique_ptr<BlockStatement> body_)
        {
            try_body = std::move(body_);
        }
        void setExceptionBody(std::unique_ptr<BlockStatement> body_)
        {
            exception_body = std::move(body_);
        }
        void setException(std::unique_ptr<Ident> exception_){
            exception=std::move(exception_);
        }
        BlockStatement * getTryBody() const
        {
            return try_body.get();
        }
        BlockStatement * getExceptionBody() const
        {
            return exception_body.get();
        }
        Ident * getException() const{
            return exception.get();
        }
    public:
       std::string toString() const override{
            std::string ret="";
            ret+="try{\n";
            ret+=try_body->toString();
            ret+="}\n";
            ret+="catch(";
            ret+=exception->toString();
            ret+="){\n";
            ret+=exception_body->toString();
            ret+="}";
            return ret;
       }
    };
    class ThrowStatement:public Statement{
    private:
        std::unique_ptr<Expression> exception;
    public:
        ThrowStatement(const Token & t):Statement(t){}
        ThrowStatement(){}
    public:
        Expression * getException() const{
            return exception.get();
        }
        void setException(std::unique_ptr<Expression> _exception){
            exception=std::move(exception);
        }
    public:
        std::string toString() const override{
            std::string ret="";
            ret+="throw ";
            ret+=exception->toString();
            ret+="\n";
            return ret;
        }
    };
    class ReturnStatement : public Statement
    {
    private:
        std::unique_ptr<Expression> expression;

    public:
        ReturnStatement(const Token &t) : Statement(t) {}
        void setExpression(Expression *e)
        {
            expression.reset(e);
        }
        Expression *getExpression()
        {
            return expression.get();
        }
        std::string toString() const override
        {
            std::string ret = "return ";
            if (expression != NULL)
                ret += expression->toString();
            return ret + ";\n";
        }
    };

    class Program : public AstNode
    {
    public:
        Program() {}

    public:
        std::vector<Statement *> stmts;

    public:
        void append(Statement *stmt)
        {
            stmts.emplace_back(stmt);
        }
        std::string toString() const override
        {
            std::string ret = "";
            for (auto &i : stmts)
            {
                ret += i->toString();
            }
            return ret;
        }

        std::string getLiteral() const override
        {
            std::string ret = "";
            if (stmts.size() > 0)
                ret = stmts[0]->getLiteral();
            return ret;
        }
        int statementsNum()
        {
            return stmts.size();
        }
        bool Empty() const
        {
            return stmts.empty();
        }
    };
}