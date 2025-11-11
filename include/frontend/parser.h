#pragma once
#include "ast.h"
#include "./error/error_manager.h"
#include "./util/util.h"
#include "lexer.h"
namespace ns
{
    enum class Priority
    {
        UNKNOWN = 0,LOWEST,ASSIGN,EQUAL,
        LESSGREATER, //> <
        ADD_,        // + -
        PRODUCT,     // * / %
        PREFIX,      // 形如!x,--x的
        NEW,
        CALL,
        INDEX,
        POINT // 成员访问运算符.
    };
    class Parser
    {
    private:
        std::vector<Token> tokens;
        int idx = 0;
    private:
        ErrorManager *errorManager;
        Lexer *mLexer;

    public:
        ErrorManager *what()
        {
            return errorManager;
        }

    private:
        void raiseError(ErrorType type, const std::string &e, sourceLocation location)
        {
            std::string msg = SourceUtil::getLineText(mLexer->getSource(), location.line);
            msg += SourceUtil::getCaretPointer(location.col);
            errorManager->report(ComilerError(type, e + "\n" + msg, location));
        }
        Token current() const { return tokens[idx]; };
        Token peek(int offset = 1) const { return tokens[idx + offset]; };
        void advance() { idx++; };
        bool match(TokenType type)
        {
            if (current().getType() == type)
            {
                // advance();
                return true;
            }
            else
            {
                raiseError(ErrorType::SYNTAX_ERROR, "expect "+getName(type)+" instead of got " + current().toString(), current().getLocation());
                return false;
            }
        }
        // Token consume(TokenType type,const std::string & error_msg);
    public:
        Parser(std::vector<Token> tokens_) : tokens(tokens_)
        {
            errorManager = new ErrorManager();
        }

    public:
        void setLexer(Lexer *lexer);
    private:
        Priority get_precedence() const;
        bool currentIs(TokenType type) const;
    private:

        //对于语句的解析函数
        std::unique_ptr<ThrowStatement> parse_throw_statement();
        std::unique_ptr<TryCatchStatement> parse_try_catch_statement();
        std::unique_ptr<Statement> parse_statement();
        std::unique_ptr<DeclareStatement> parse_declare_statement();
        std::unique_ptr<ExpressionStatement> parse_expression_statement();
        std::vector<std::shared_ptr<Expression>> parse_func_params(int &error);
        std::unique_ptr<FuncDecl> parse_func_statement();
        std::unique_ptr<ClassLiteral> parse_class_statement();
        std::unique_ptr<BlockStatement> parse_blockstatement(int loop);
        std::unique_ptr<ReturnStatement> parse_return_statement();
        std::unique_ptr<ShortStatement> parse_short_statement();
        std::unique_ptr<ImportStatement> parse_import_statement();
        std::unique_ptr<ForLoop> parse_for_loop();

        //对于表达式的解析函数

        std::unique_ptr<SwitchExpression> parse_switch_expression();
        std::unique_ptr<LambdaExpr> parse_lambda_expression();
        std::unique_ptr<Expression> parse_expression(Priority priority);
        std::unique_ptr<Expression> parse_prefix_expression();
        std::unique_ptr<Expression> parse_infix_expression(std::unique_ptr<Expression> left, Token op);
        std::unique_ptr<StringLiteral> parse_string_expression();
        std::unique_ptr<BoolLiteral> parse_bool_expression();
        std::unique_ptr<Ident> parse_ident_expression();
        std::unique_ptr<I64Literal> parse_integer_expression();
        std::unique_ptr<Float64Literal> parse_float_expression();
        std::unique_ptr<ArrayLiteral> parse_array_expression();
        std::vector<std::unique_ptr<Expression>> parse_expression_list(TokenType end, int &error);
        std::unique_ptr<IndexExpression> parse_index_expression(Expression *left);
        std::unique_ptr<IfExpression> parse_if_expression();
        std::unique_ptr<WhileExpression> parse_while_expression();
        std::unique_ptr<UntilExpression> parse_until_expresssion();
    public:
        std::unique_ptr<Program> parse();
    };
}