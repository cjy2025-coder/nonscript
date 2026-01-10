#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "type.h"
#include <iomanip>
#include<fstream>
namespace ns
{
   enum class TACOp
   {
      // 算术运算
      ADD,
      SUB,
      MUL,
      DIV,
      MOD,
      NEG, // 一元负号

      // 逻辑运算
      AND,
      OR,
      NOT,
      EQ,
      NE,
      LT,
      LE,
      GT,
      GE,

      // 内存访问
      LOAD,   // t = *addr
      STORE,  // *addr = value
      ALLOCA, // 栈分配
      GLOBAL, // 全局变量

      // 控制流
      LABEL,      // 标签定义
      JUMP,       // 无条件跳转
      JUMP_IF,    // 条件跳转（if true）
      JUMP_IFNOT, // 条件跳转（if false）

      // 函数相关
      CALL,      // 函数调用
      RET,       // 返回值
      PARAM,     // 设置参数
      GET_PARAM, // 获取参数

      // 数据移动
      MOVE,     // 复制：t1 = t2
      LOAD_IMM, // 加载立即数

      // 特殊
      PHI, // SSA Phi函数（可选）
      NOP, // 空操作
   };
   TACOp convertOp(std::string op){
      if(op == "+"){
         return TACOp::ADD;
      }
      else if(op == "-"){
         return TACOp::SUB;
      }
      else if(op == "*"){
         return TACOp::MUL;
      }
      else if(op == "/"){
         return TACOp::DIV;
      }
      else if(op == "%"){
         return TACOp::MOD;
      }
      else{
         return TACOp::NOP;
      }
   }
   std::string tacOpToString(TACOp op)
   {
      switch (op)
      {
      // 算术运算
      case TACOp::ADD:
         return "ADD";
      case TACOp::SUB:
         return "SUB";
      case TACOp::MUL:
         return "MUL";
      case TACOp::DIV:
         return "DIV";
      case TACOp::MOD:
         return "MOD";
      case TACOp::NEG:
         return "NEG";

      // 逻辑运算
      case TACOp::AND:
         return "AND";
      case TACOp::OR:
         return "OR";
      case TACOp::NOT:
         return "NOT";
      case TACOp::EQ:
         return "EQ";
      case TACOp::NE:
         return "NE";
      case TACOp::LT:
         return "LT";
      case TACOp::LE:
         return "LE";
      case TACOp::GT:
         return "GT";
      case TACOp::GE:
         return "GE";

      // 内存访问
      case TACOp::LOAD:
         return "LOAD";
      case TACOp::STORE:
         return "STORE";
      case TACOp::ALLOCA:
         return "ALLOCA";
      case TACOp::GLOBAL:
         return "GLOBAL";

      // 控制流
      case TACOp::LABEL:
         return "LABEL";
      case TACOp::JUMP:
         return "JUMP";
      case TACOp::JUMP_IF:
         return "JUMP_IF";
      case TACOp::JUMP_IFNOT:
         return "JUMP_IFNOT";

      // 函数相关
      case TACOp::CALL:
         return "CALL";
      case TACOp::RET:
         return "RET";
      case TACOp::PARAM:
         return "PARAM";
      case TACOp::GET_PARAM:
         return "GET_PARAM";

      // 数据移动
      case TACOp::MOVE:
         return "MOVE";
      case TACOp::LOAD_IMM:
         return "LOAD_IMM";

      // 特殊
      case TACOp::PHI:
         return "PHI";
      case TACOp::NOP:
         return "NOP";

      default:
         return "UNKNOWN";
      }
   }
   // 三地址码
   class TACInstruction
   {
   public:
      TACOp op;
      std::string result; // 结果操作数
      std::string arg1;   // 第一个参数
      std::string arg2;   // 第二个参数
      // TypeInfo *type;     // 类型信息（可选）
      // sourceLocation loc; // 源代码位置（调试用）

      std::string toString() const
      {
         std::stringstream ss;
         // 根据指令类型格式化
         switch (op)
         {
         case TACOp::LABEL:
            // LABEL:  L1:
            ss << arg1 << ":";
            break;

         case TACOp::JUMP:
            // JUMP:   JUMP L1
            ss << arg1;
            break;

         case TACOp::JUMP_IF:
         case TACOp::JUMP_IFNOT:
            // JUMP_IF: JUMP_IF t1, L1
            ss << arg1 << ", " << arg2;
            break;

         case TACOp::RET:
            // RET:    RET
            if (!result.empty())
            {
               ss << result;
            }
            break;

         case TACOp::CALL:
            // CALL:   t1 = CALL @func
            if (!result.empty())
            {
               ss << result << " = ";
            }
            ss << "CALL " << arg1;
            break;

         case TACOp::PARAM:
            // PARAM:  PARAM #0, x
            ss << arg1 << ", " << arg2;
            break;

         case TACOp::MOVE:
            // MOVE:   t1 = t2
            ss << result << " = " << arg1;
            break;

         case TACOp::LOAD_IMM:
            // LOAD_IMM: t1 = #42
            ss << result << " = #" << arg1;
            break;

         case TACOp::NEG:
         case TACOp::NOT:
            // 一元操作: t1 = NEG t2
            ss << result << " = " << tacOpToString(op) << " " << arg1;
            break;

         case TACOp::NOP:
            // NOP:    NOP
            break;
         case TACOp::ALLOCA:
            ss << tacOpToString(op) << " "<<result;
            break;
         default:
            // 二元操作: t1 = t2 + t3
            if (!result.empty())
            {
               ss << result << " = ";
            }
            if (!arg1.empty())
            {
               ss << arg1;
               if (!arg2.empty())
               {
                  ss << " " << tacOpToString(op) << " " << arg2;
               }
            }
            break;
         }
         return ss.str();
      }
   };
   class TempManager
   {
   private:
      int temp_counter = 0;
      int label_counter = 0;

   public:
      std::string newTemp()
      {
         return "T" + std::to_string(temp_counter++);
      }

      std::string newLabel()
      {
         return "L" + std::to_string(label_counter++);
      }

      void reset()
      {
         temp_counter = 0;
         label_counter = 0;
      }
   };
   class TACContext
   {
   public:
      void save(std::string path){
         std::ofstream stream;
         stream.open(path,std::ios::out);
         for(auto & instruction : instructions){
            stream<<instruction->toString()<<std::endl;
         }
         stream.close();
      }
   public:
      std::vector<TACInstruction *> instructions;
      TempManager temps;

      // 当前函数信息
      std::string current_function;
      std::map<std::string, std::string> var_to_temp; // 变量名 → 临时变量

      // 添加指令
      std::string emit(TACOp op,
                       const std::string &result = "",
                       const std::string &arg1 = "",
                       const std::string &arg2 = "")
      {
         TACInstruction *tac=new TACInstruction();
         tac->arg1=arg1;
         tac->arg2=arg2;
         tac->result=result;
         tac->op=op;
         instructions.push_back(tac);
         return tac->toString();
      }
   };
}