#pragma once
#include<string>
#include<memory>
#include<vector>
namespace ns{
    /*
    *Spring Script的gimple中间代码包含的基本类型
    */
    enum class TypeKind{
      BOOL,OBJECT,ARRAY,PAIR,STRING,VOID,I8,I16,I32,I64,F32,F64,ANY
    };
    /*
    *Spring Script的gimple中间代码包含的基本操作符
    */ 
    typedef enum class OpKind{
        ADD,SUB,MUL,DIV,MOD,BIT_AND,BIT_OR,AND,OR,NOT,EQ,NE,LT,LTE,GT,GTE,ASSIGN,CALL,RETURN,GOTO
    }GimpleOp;
    //Spring Script的gimple中间代码语句类型
    enum class GimpleStmtType {
        DECLARE,ASSIGN,CALL,RETURN,GOTO,COND_GOTO,LABEL,PHI
    };
}