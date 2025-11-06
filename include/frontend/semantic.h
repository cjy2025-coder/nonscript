#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "./error/error_manager.h"
#include "ast.h"
#include "lexer.h"
#include "./util/util.h"
#include <variant>
#include<map>
namespace ns
{

    typedef struct _FuncDetail
    {
        std::vector<DataType> param_types;
        DataType return_type;
    } FuncDetail;
    typedef struct _ArrayDetail
    {
        std::vector<DataType> elems_types;
        int size;
    }ArrayDetail;
    typedef struct _TypeInfo{
        DataType baseType;
        std::variant<FuncDetail,ArrayDetail> detail;
    }TypeInfo;

    TypeInfo * createTypeInfo(DataType baseType_);
    typedef struct Symbol
    {
        TypeInfo * type_info;
        int scope_level;
        std::string name;
    } Symbol;

    class SemanticAnalyzer
    {
    public:
        int check(Program * program);
        ErrorManager *what();
    private:
        std::unordered_map<std::string, Symbol *> symbol_table;
        int current_scope;
        ErrorManager *errorManager=new ErrorManager();
        Lexer *mLexer;
    public: 
        std::unordered_map<std::string, Symbol *> getSymbolTable() const{return symbol_table;}
    public:
        SemanticAnalyzer(Lexer *_mLexer) : current_scope(0), mLexer(_mLexer) {}
    private:
        void push_symbol(std::string name,Symbol * symbol);
    private:
        void setErrorManager(ErrorManager *errorManager_) { errorManager = errorManager_; }
        void enter_scope() { current_scope++; }
        void raiseError(ErrorType type, const std::string &e, sourceLocation location)
        {
            std::string msg = SourceUtil::getLineText(mLexer->getSource(), location.line);
            msg += SourceUtil::getCaretPointer(location.col);
            errorManager->report(ComilerError(type, e + "\n" + msg, location));
        }

    private:
        bool exit_scope();
        Symbol *find_symbol(const std::string &name);
        std::string type_to_string(DataType type);
        DataType string_to_type(std::string type);
        bool is_arithmetic_op(std::string op);
        bool is_logic_op(std::string op);
        bool is_int(DataType type);
        bool is_float(DataType type);
        bool is_number(DataType type);
        bool is_string(DataType type);
        bool is_bool(DataType type);
        TypeInfo *get_wider_numeric_type(TypeInfo * left,TypeInfo * right);
    private:
        TypeInfo *check_array_literal(ArrayLiteral * arr);
        TypeInfo *check_param(Expression * param);
        TypeInfo *check_ident(Ident *ident);
        TypeInfo *check_index_expression(IndexExpression *expr);
        TypeInfo *check_expression(Expression *expr);
        TypeInfo *check_prefix_expression(PrefixExpression *expr);
        TypeInfo *check_infix_expression(InfixExpression *expr);
        TypeInfo *check_lambda_expression(LambdaExpr * expr);
        TypeInfo *check_block_statement(BlockStatement * stmt);
        TypeInfo *check_statement(Statement * stmt);
        TypeInfo *check_declare_statement(DeclareStatement *stmt);
        TypeInfo *check_return_statement(ReturnStatement * stmt);
        TypeInfo *check_func_statement(FuncDecl * stmt);
        TypeInfo *check_call_expression(CallExpression * expr);
        TypeInfo *check_expression_statement(ExpressionStatement * expr);
        TypeInfo *check_if_expression(IfExpression * expr);
        TypeInfo *check_while_expression(WhileExpression * expr);
        TypeInfo *check_class_statement(ClassLiteral * stmt);
    };
}