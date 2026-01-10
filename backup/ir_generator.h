#pragma once
#include"ir.h"
#include"frontend/ast.h"
#include"frontend/semantic.h"
namespace ns{
    class IrGenerator{
    private:
          int  temp_counter=0;
          FuncIR current_func;
          std::unordered_map<std::string, Symbol *> symbol_table;
    private:
          std::string new_temp();
          std::string visit(const Expression * expr);
          std::string visit_infix_expression(const InfixExpression * expr);
          std::string visit_prefix_expression(const PrefixExpression * expr);
          std::string visit_simple_expression(const Expression * expr);
    private:
          
    public:
          void loadSymbols(std::unordered_map<std::string, Symbol *> symbols){symbol_table=symbols;}
          FuncIR generate(const Program& program); 
          FuncIR generate_declare_statement(const DeclareStatement * stmt);   
    };
}