#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
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
// 语义错误检查宏：如果 r 为空或等于 _errorType，返回 true（有错）
#define _SEMANTIC_ERROR(r) (!(r)) || ((r) == (_errorType))
#define MARK_ERROR return (_errorType)

// 统一语义错误宏：自动附带源码上下文
#define SEMANTIC_ERR(code, msg, loc) \
    do { \
        std::string _ctx = SourceUtil::getLineText(mLexer->getSource(), (loc).line) + \
                           SourceUtil::getCaretPointer((loc).col); \
        raiseError((code), (msg), (loc), _ctx); \
        return _errorType; \
    } while(0)

#define SEMANTIC_ERR_RET(code, msg, loc, ret) \
    do { \
        std::string _ctx = SourceUtil::getLineText(mLexer->getSource(), (loc).line) + \
                           SourceUtil::getCaretPointer((loc).col); \
        raiseError((code), (msg), (loc), _ctx); \
        return (ret); \
    } while(0)
    typedef struct _Symbol
    {
        TypeInfo *type_info;
        std::string name;
    } Symbol;
    struct Scope
    {
        // 普通符号（变量、类等）：一个名字只对应一个符号
        std::unordered_map<std::string, Symbol *> symbols_;
        // 重载函数：一个名字对应多个符号（函数重载）
        std::unordered_map<std::string, std::vector<Symbol *>> overloaded_funcs_;
        Scope *parent_ = nullptr;
        ~Scope()
        {
            for (auto &pair : symbols_)
            {
                delete pair.second;
            }
            for (auto &pair : overloaded_funcs_)
            {
                for (auto *sym : pair.second)
                    delete sym;
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
        // 插入普通符号（变量、类、非重载函数）
        void insert(Symbol *symbol)
        {
            current_->symbols_[symbol->name] = symbol;
        }
        // 插入重载函数：同名但不同参数类型的函数
        void insert_overloaded(Symbol *symbol)
        {
            current_->overloaded_funcs_[symbol->name].push_back(symbol);
        }
        // 查找普通符号（精确匹配名字）
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
                // 也在重载表中查找（仅在无精确匹配时返回第一个，用于检查重定义）
                const auto &of_it = ps->overloaded_funcs_.find(name);
                if (of_it != ps->overloaded_funcs_.end())
                {
                    // 返回第一个重载版本（调用方应知道这是一个重载函数）
                    return of_it->second[0];
                }
                ps = ps->parent_;
                if (!ps)
                {
                    break;
                }
            }
            return nullptr;
        }
        // 查找重载函数列表
        std::vector<Symbol *> *find_overloaded(const std::string name) const
        {
            Scope *ps = current_;
            while (1)
            {
                const auto &of_it = ps->overloaded_funcs_.find(name);
                if (of_it != ps->overloaded_funcs_.end())
                {
                    return const_cast<std::vector<Symbol *> *>(&(of_it->second));
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
        std::set<std::string> imported_files; // 已导入的文件（避免循环导入）
        std::vector<std::unique_ptr<Program>> imported_programs; // 保存导入的AST
        std::vector<std::string> link_libs; // cimport 链接库列表
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
        void raiseError(ErrorCode code, const std::string &e, sourceLocation location, const std::string &)
        {
            em_->semanticError(code, e, location);
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
        TypeInfo *check_switch_expression(SwitchExpression * expr);
        TypeInfo *check_block_statement(BlockStatement *stmt);
        TypeInfo *check_statement(Statement *stmt);
        TypeInfo *check_declare_statement(DeclareStatement *stmt);
        TypeInfo *check_return_statement(ReturnStatement *stmt);
        TypeInfo *check_func_statement(FuncDecl *stmt);
        TypeInfo *check_call_expression(CallExpression *expr);
        TypeInfo *check_expression_statement(ExpressionStatement *expr);
        TypeInfo *check_if_expression(IfExpression *expr);
        TypeInfo *check_while_expression(WhileExpression *expr);
        TypeInfo *check_until_expression(UntilExpression *expr);
        TypeInfo *check_class_statement(ClassLiteral *stmt);
        TypeInfo *check_new_expression(NewExpression *expr);
        TypeInfo *check_import_statement(ImportStatement * stmt);
        TypeInfo *collect_and_check_all_import_statement(Program * program);
        TypeInfo *check_for_loop_statement(ForLoop * stmt);
    };
}