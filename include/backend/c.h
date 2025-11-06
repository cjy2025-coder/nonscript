#pragma once
#include"frontend/ast.h"
#include"frontend/semantic.h"
namespace ns{
    class CGenerator{
      private:
      std::unordered_map<std::string, Symbol *> symbol_table;
      std::string ccode;
      public:
      void generate(Program * program);
      private:
      void add_basic_lib();
      void generate_declare_statement(DeclareStatement * stmt);

    };
}