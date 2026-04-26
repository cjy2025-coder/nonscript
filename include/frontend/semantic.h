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
#include "type.h"
#include "builtin.h"
namespace ns
{

    typedef struct _FuncDetail
    {
        std::vector<_type *> param_types;
        _type * return_type;
        //默认为false，[[noused]]
        bool is_external_func = false;
        //默认为false, [[noused]]
        bool is_builtin_func = false; 
        //默认为false
        bool is_unlimited_args_func = false;
    } FuncDetail;
    typedef struct _TypeInfo TypeInfo;
    typedef struct _ArrayDetail
    {
        std::vector<TypeInfo*> elems_types;
        int size;
    }ArrayDetail;
    typedef struct _MemDetail{
        TypeInfo * ti;
        AccessLevel level;
        MemberType type;
    }MemDetail;
    typedef struct _ClassDetail{
        //表示父类
        std::vector<TypeInfo*> parents;
        //类成员
        std::unordered_map<std::string,MemDetail> mems;
    }ClassDetail;
    typedef struct _TypeInfo{
        _type * baseType;
        std::variant<FuncDetail,ArrayDetail,ClassDetail> detail;
    }TypeInfo;
    TypeInfo * createTypeInfo(std::string alias,typeManager * _typeManager);
    extern TypeInfo * _errorType;
    #define _SEMANTIC_ERROR(r) (!(r)) || ((r)==(_errorType))
    #define _MARK_ERROR return (_errorType);
    typedef struct _Symbol
    {
        TypeInfo * type_info;
        int scope_level;
        std::string name;
    } Symbol;
    typedef struct _Scope{
       std::unordered_map<std::string,Symbol*> symbols;
       _Scope* parent = nullptr;
       
    }Scope;
    class SymbolTable{
        Scope * current = nullptr;//当前作用域
    public:
        void enter(){
            current=new Scope();
        }
    };
    // class GimpleCodeGen;
    // class TACCodeGen;
    class CplusplusCodeGen;
    std::string type_to_string(_type * type);
    _type * string_to_type(std::string type,typeManager * manager);
    class SemanticAnalyzer
    {
    // friend GimpleCodeGen;
    // friend TACCodeGen;
       friend CplusplusCodeGen;
    public:
        int check(Program * program);
        ErrorManager *what();
    private:
        std::unordered_map<std::string, Symbol *> symbol_table;
        int current_scope;
        ErrorManager *errorManager=new ErrorManager();
        typeManager * types;
        Lexer *mLexer;
        void add_builtin_func(const std::string & name,
                              const std::string & ret_type,
                              const std::vector<_type*> & args = {},
                              bool is_unlimited_args_func = true){ //把内置函数加入到符号表    
            Symbol * sb = new Symbol();
            sb->name = name;
            sb->scope_level = 0;
            TypeInfo * ti = createTypeInfo("func",types);
            FuncDetail fd={};
            fd.return_type = createTypeInfo(ret_type,types)->baseType;
            fd.is_builtin_func = true;
            fd.is_unlimited_args_func = is_unlimited_args_func;
            fd.param_types =args;
            ti->detail=fd;
            sb->type_info=ti;
            
            symbol_table[name]=sb;
        }
        void init_builtin_funcs(){
            // const auto & btis = builtin_manager::builtins_;
            // for(const auto & it : btis){
            //     //  add_builtin_func(it.second.name_,it.second.ret_type_,it.second.args_type_,it.second.is_unlimited_args_func_)
            // }
            add_builtin_func("print","void");
            add_builtin_func("scan","void");
        }
    public:
        void setTypeManager(typeManager * _types){
            types=_types;
            init_builtin_funcs();
        }
    public: 
        std::unordered_map<std::string, Symbol *> getSymbolTable() const{return symbol_table;}
        typeManager* getTypeManager() const{return types;}
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
        bool is_arithmetic_op(std::string op);
        bool is_logic_op(std::string op);
        bool is_int(_type *type);
        bool is_float(_type *type);
        bool is_number(_type *type);
        bool is_string(_type *type);
        bool is_bool(_type *type);
        TypeInfo *get_wider_numeric_type(TypeInfo * left,TypeInfo * right);
    private:
        TypeInfo *check_array_literal(ArrayLiteral * arr);
        TypeInfo *check_param(std::shared_ptr<FuncParam> param);
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
        TypeInfo *check_new_expression(NewExpression * expr);
    };
}