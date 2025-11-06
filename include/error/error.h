#pragma once
#include <string>
#include "./frontend/token.h"

namespace ns
{
    enum class ErrorType
    {
        SYNTAX_ERROR,       // 语法错误
        RUNTIME_ERROR,      // 运行时错误
        TYPE_ERROR,         // 类型错误
        UNDEFINED_VARIABLE, // 未定义变量
        DIVISION_BY_ZERO,   // 除以零错误
        FILE_NOT_FOUND,     // 文件未找到
        SEMANTIC_ERROR,     // 语义错误
        UNKNOWN_ERROR       // 未知错误
    };
    class ComilerError
    {
    private:
        ErrorType type;  ///>错误类型
        std::string msg; ///>错误信息
        sourceLocation location;
    public:
        ComilerError(ErrorType type_,const std::string &msg_,const sourceLocation & location_ )
            : type(type_), msg(msg_),location(location_) {}
        ErrorType getType() const { return type; }
        std::string getFile() const { return location.filename; }
        std::string what() const
        {
            return "Error in file " + location.filename + " at line " + std::to_string(location.line) + ", column " + std::to_string(location.col) + ": " + msg;
        }
    };
}
