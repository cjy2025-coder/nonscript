#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "./error/error_manager.h"
#include "ast.h"
#include "lexer.h"
#include "./util/util.h"
#include <map>
#include "type.h"
#include "builtin.h"
// #include "compiler.h"
namespace ns
{
#define _SEMANTIC_ERROR(r) (!(r)) || ((r) == (_errorType))
#define MARK_ERROR return (_errorType)
    typedef struct _Symbol
    {
        TypeInfo *type_info;
        std::string name;
    } Symbol;
    struct Scope
    {
        std::unordered_map<std::string, Symbol *> symbols_;
        Scope *parent_ = nullptr;
        ~Scope()
        {
            for (auto &pair : symbols_)
            {
                delete pair.second;
            }
        }
    };
    class SymbolTable
    {
    public:
        Scope *current_ = nullptr; // 当前作用域
    public:
        void merge(SymbolTable * st){
            Scope * top_scope = st->current_;
            while(1){
                if(!top_scope->parent_){
                    break;
                }
                top_scope = top_scope->parent_;
            }
            
        }
        void enter()
        {
            auto old_scope = current_;
            current_ = new Scope();
            current_->parent_ = old_scope;
        }
        void leave()
        {
            Scope *old_scope = current_;
            current_ = current_->parent_;
            delete old_scope;
        }
        void insert(Symbol *symbol)
        {
            current_->symbols_[symbol->name] = symbol;
        }
        Symbol *find(const std::string name) const
        {
            Scope *ps = current_;
            while (1)
            {
                const auto &sbs = ps->symbols_;
                const auto &it = sbs.find(name);
                if (it != sbs.end())
                {
                    return it->second;
                }
                ps = ps->parent_;
                if (!ps)
                {
                    break;
                }
            }
            return nullptr;
        }
    };

    class CplusplusCodeGen;
    std::string type_to_string(_type *type);
    _type *string_to_type(std::string type, typeManager *manager);
    class SemanticAnalyzer
    {
        friend CplusplusCodeGen;

    public:
        // 语法分析得到的ast树
        int check(Program *program);
        ErrorManager *what();
        SymbolTable *st = new SymbolTable();

    private:
        ErrorManager *em_ = new ErrorManager();
        Lexer *mLexer;
        // void merge_symbol_table(SemanticAnalyzer * sa){
        //     st->
        // }
        void add_builtin_func(const std::string &name,
                              const std::string &ret_type,
                              const std::vector<_type *> &args = {},
                              bool is_unlimited_args_func = true)
        { // 把内置函数加入到符号表
            Symbol *sb = new Symbol();
            sb->name = name;
            TypeInfo *ti = typeManager::find("func");
            FuncDetail fd = {};
            fd.return_type = typeManager::find(ret_type)->baseType;
            fd.is_builtin_func = true;
            fd.is_unlimited_args_func = is_unlimited_args_func;
            fd.param_types = args;
            ti->detail = fd;
            sb->type_info = ti;

            st->insert(sb);
        }
        void init_builtin_funcs()
        {
            add_builtin_func("print", "void");
            add_builtin_func("scan", "void");
        }
    public:
        SemanticAnalyzer(Lexer *_mLexer) :mLexer(_mLexer) {}

    private:
        void push_symbol(std::string name, Symbol *symbol)
        {
            st->insert(symbol);
        }

    private:
        void setErrorManager(ErrorManager *errorManager_) { em_ = errorManager_; }
        void enter_scope()
        {
            st->enter();
        }
        void raiseError(ErrorType type, const std::string &e, sourceLocation location)
        {
            std::string msg = SourceUtil::getLineText(mLexer->getSource(), location.line);
            msg += SourceUtil::getCaretPointer(location.col);
            em_->report(ComilerError(type, e + "\n" + msg, location));
        }

    private:
        void exit_scope()
        {
            st->leave();
        }
        Symbol *find_symbol(const std::string &name)
        {
            return st->find(name);
        }
        bool is_arithmetic_op(std::string op);
        bool is_logic_op(std::string op);
        bool is_int(_type *type);
        bool is_float(_type *type);
        bool is_number(_type *type);
        bool is_string(_type *type);
        bool is_bool(_type *type);
        TypeInfo *get_wider_numeric_type(TypeInfo *left, TypeInfo *right);

    private:
        TypeInfo *check_array_literal(ArrayLiteral *arr);
        TypeInfo *check_param(std::shared_ptr<FuncParam> param);
        TypeInfo *check_ident(Ident *ident);
        TypeInfo *check_index_expression(IndexExpression *expr);
        TypeInfo *check_expression(Expression *expr);
        TypeInfo *check_prefix_expression(PrefixExpression *expr);
        TypeInfo *check_infix_expression(InfixExpression *expr);
        TypeInfo *check_lambda_expression(LambdaExpr *expr);
        TypeInfo *check_block_statement(BlockStatement *stmt);
        TypeInfo *check_statement(Statement *stmt);
        TypeInfo *check_declare_statement(DeclareStatement *stmt);
        TypeInfo *check_return_statement(ReturnStatement *stmt);
        TypeInfo *check_func_statement(FuncDecl *stmt);
        TypeInfo *check_call_expression(CallExpression *expr);
        TypeInfo *check_expression_statement(ExpressionStatement *expr);
        TypeInfo *check_if_expression(IfExpression *expr);
        TypeInfo *check_while_expression(WhileExpression *expr);
        TypeInfo *check_class_statement(ClassLiteral *stmt);
        TypeInfo *check_new_expression(NewExpression *expr);
        TypeInfo *check_import_statement(ImportStatement * stmt);
        TypeInfo *collect_and_check_all_import_statement(Program * program);
        TypeInfo *check_for_loop_statement(ForLoop * stmt);
    };
}