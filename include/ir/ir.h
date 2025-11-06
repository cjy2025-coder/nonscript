#pragma once
#include<string>
#include<vector>
#include<unordered_map>
#include"type.h"
namespace ns{
    enum class Op{
       ADD,SUB,MUL,DIV,MOD,MOV,LDC,CALL,RET
    };

   std::string lookup(Op op);
    //三地址码
    struct TAC{
       Op op;
       std::string result,arg1,arg2;  
       IRType type;
       std::string toString() const{
         return result + " = "+lookup(op)+" "+arg1+" "+arg2;
       }
       std::string get_asm_prefix() const{
          switch(type){
            case IRType::BOOLEAN: return "byte";
            case IRType::INT8: return "byte";
            case IRType::INT16: return "word";
            case IRType::INT32: return "dword";
            default: return "qword";
          }
       }
    };
 
    //函数级IR,由若干三地址码组成
    struct FuncIR{
       std::string name;
       std::vector<TAC>  instructions;
       std::unordered_map<std::string,int> var_offset;//变量在堆栈上的偏移
       std::string toString() const{
          std::string ret="";
          for(size_t i=0;i<instructions.size();i++){
              if(i != 0) ret+="\n";
              ret+=instructions[i].toString();
          }
          return ret;
       }
    };
}