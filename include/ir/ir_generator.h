#pragma once
#include "ir.h"
#include "frontend/ast.h"
#include "frontend/semantic.h"
namespace ns
{
    class TACGenerator
    {
    private:
        TACContext ctx;

    private:
        int alloc_size(std::string type)
        {
            if (type == "int8")
            {
                return 1;
            }
            else if (type == "int16")
            {
                return 2;
            }
            else if (type == "int32")
            {
                return 4;
            }
            else if (type == "int64")
            {
                return 8;
            }
            else if (type == "float32")
            {
                return 4;
            }
            else if (type == "float64")
            {
                return 8;
            }
            else
                return 8; // 其余情况，统一分配8字节
        }

    public:
        void save(std::string path)
        {
            ctx.save(path);
        }

    public:
        std::string genExpr(Expression *expr)
        {
            if (auto *_ = dynamic_cast<Ident *>(expr))
            {
                return "$" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<I8Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<I16Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<I32Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<I64Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<Float32Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<Float64Literal *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            else if (auto *_ = dynamic_cast<BoolLiteral *>(expr))
            {
                return "#" + expr->getLiteral();
            }
            if (auto *infix = dynamic_cast<InfixExpression *>(expr))
            {
                if (infix->getOperator() == "=")
                {
                    Expression *leftExpr = infix->getLeft();
                    if (auto *ident = dynamic_cast<Ident *>(leftExpr))
                    {
                        std::string varName = ident->getLiteral();
                        std::string right = genExpr(infix->getRight());
                        ctx.emit(TACOp::MOVE, varName, right);
                        return varName;
                    }
                }
                else
                {
                    std::string left = genExpr(infix->getLeft());
                    std::string right = genExpr(infix->getRight());
                    std::string result = ctx.temps.newTemp();
                    TACOp op = convertOp(infix->getOperator());
                    ctx.emit(op, result, left, right);
                    return result;
                }
            }
        }
        void genBlockStatement(BlockStatement *stmt)
        {
            auto stmts = stmt->value();
            for (auto &st : stmts)
            {
                genStmt(st);
            }
        }
        // 翻译语句
        void genStmt(Statement *stmt)
        {
            if (auto *_ = dynamic_cast<DeclareStatement *>(stmt))
            {
                auto symbols = _->getVars();
                for (auto &symbol : symbols)
                {
                    ctx.emit(TACOp::ALLOCA, "", symbol.first->getLiteral(), std::to_string(alloc_size(symbol.first->getType())));
                    auto value = symbol.second;
                    if (value)
                    {
                        auto val = genExpr(value);
                        ctx.emit(TACOp::MOVE, symbol.first->getLiteral(), val);
                    }
                }
            }
            else if (auto *_ = dynamic_cast<FuncDecl *>(stmt))
            {
                auto name = _->getLiteral();
                auto params = _->getParams();
                auto body = _->getBody();
                ctx.emit(TACOp::LABEL, "", name);
                for (auto &param : params)
                {
                    ctx.emit(TACOp::PARAM, "", param->getLiteral());
                }
                genStmt(body);
            }
            else if (auto *_ = dynamic_cast<BlockStatement *>(stmt))
            {
                genBlockStatement(_);
            }
            else if (auto *_ = dynamic_cast<ReturnStatement *>(stmt))
            {
                auto value = _->getExpression();
                ctx.emit(TACOp::RET, "", genExpr(value));
            }
            else if (auto *_ = dynamic_cast<ExpressionStatement *>(stmt))
            {
                genExpr(_->expression());
            }
        }
        void gen(Program *program)
        {
            auto stmts = program->stmts;
            for (auto &stmt : stmts)
            {
                genStmt(stmt);
            }
        }
    };
}