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

    // 错误严重等级（使用下划线前缀避免与 windows.h 宏冲突）
    enum class Severity
    {
        _ERROR,
        _WARNING,
        _NOTE,
        _HELP
    };

    // 唯一错误码
    enum class ErrorCode
    {
        // === Parser 语法错误 (1xxx) ===
        MISS_TOKEN = 1001,
        UNEXPECTED_TOKEN = 1002,
        MISS_EXPRESSION = 1003,
        EXPECT_BUT_GOT = 1004,
        IDENT_MISSING = 1005,
        IDENT_TYPE_MISSING = 1006,
        FUNC_NAME_MISSING = 1007,
        FUNC_RETURN_TYPE_MISSING = 1008,
        FUNC_PARAM_MISS_TYPE = 1009,
        FUNC_PARAM_MISS_NAME = 1010,
        CLASS_NAME_MISSING = 1011,
        PARENT_CLASS_NAME_MISSING = 1012,
        CLASS_CONSTRUCTOR_ERROR = 1013,
        INVALID_CLASS_MEMBER = 1014,
        INVALID_TOKEN_AFTER_CONTROLLER = 1015,
        MISS_OBJECT_MEMBER = 1016,
        CONST_NOT_INIT = 1017,
        EXTERNAL_FUNC_MISS_RETURN = 1018,

        // === Semantic 语义错误 (2xxx) ===
        UNDEFINED_SYMBOL = 2001,
        UNKNOWN_TYPE = 2002,
        REDEFINED_SYMBOL = 2003,
        TYPE_NOT_MATCH = 2004,
        INVALID_EXPRESSION = 2005,
        NOT_A_CLASS = 2006,
        NO_SUCH_MEMBER = 2007,
        MEMBER_NOT_ACCESSIBLE = 2008,
        ARRAY_INDEX_OUT_OF_BOUNDS = 2009,
        EXPECT_ARRAY = 2010,
        EXPECT_INTEGER_INDEX = 2011,
        UNMATCH_RETURN_TYPE = 2012,
        NO_MATCH_FUNCTION = 2013,
        UNMATCH_ARG_TYPE = 2014,
        INVALID_ALLOC = 2015,
        UNKNOWN_CLASS = 2016,
        REDEFINE_CLASS = 2017,
        MODULO_NON_INT = 2018,
    };

    // 获取错误码对应的简短描述标签
    inline std::string error_code_label(ErrorCode code)
    {
        switch (code)
        {
        case ErrorCode::MISS_TOKEN: return "missing token";
        case ErrorCode::UNEXPECTED_TOKEN: return "unexpected token";
        case ErrorCode::MISS_EXPRESSION: return "missing expression";
        case ErrorCode::EXPECT_BUT_GOT: return "unexpected token";
        case ErrorCode::IDENT_MISSING: return "missing identifier";
        case ErrorCode::IDENT_TYPE_MISSING: return "missing type annotation";
        case ErrorCode::FUNC_NAME_MISSING: return "missing function name";
        case ErrorCode::FUNC_RETURN_TYPE_MISSING: return "missing return type";
        case ErrorCode::FUNC_PARAM_MISS_TYPE: return "missing parameter type";
        case ErrorCode::FUNC_PARAM_MISS_NAME: return "missing parameter name";
        case ErrorCode::CLASS_NAME_MISSING: return "missing class name";
        case ErrorCode::PARENT_CLASS_NAME_MISSING: return "missing parent class name";
        case ErrorCode::CLASS_CONSTRUCTOR_ERROR: return "invalid constructor";
        case ErrorCode::INVALID_CLASS_MEMBER: return "invalid class member";
        case ErrorCode::INVALID_TOKEN_AFTER_CONTROLLER: return "invalid member after access specifier";
        case ErrorCode::MISS_OBJECT_MEMBER: return "missing member name";
        case ErrorCode::CONST_NOT_INIT: return "uninitialized constant";
        case ErrorCode::EXTERNAL_FUNC_MISS_RETURN: return "external function needs return type";
        case ErrorCode::UNDEFINED_SYMBOL: return "undefined symbol";
        case ErrorCode::UNKNOWN_TYPE: return "unknown type";
        case ErrorCode::REDEFINED_SYMBOL: return "redefined symbol";
        case ErrorCode::TYPE_NOT_MATCH: return "type mismatch";
        case ErrorCode::INVALID_EXPRESSION: return "invalid expression";
        case ErrorCode::NOT_A_CLASS: return "not a class";
        case ErrorCode::NO_SUCH_MEMBER: return "no such member";
        case ErrorCode::MEMBER_NOT_ACCESSIBLE: return "inaccessible member";
        case ErrorCode::ARRAY_INDEX_OUT_OF_BOUNDS: return "index out of bounds";
        case ErrorCode::EXPECT_ARRAY: return "expected array";
        case ErrorCode::EXPECT_INTEGER_INDEX: return "expected integer index";
        case ErrorCode::UNMATCH_RETURN_TYPE: return "mismatched return type";
        case ErrorCode::NO_MATCH_FUNCTION: return "no matching function";
        case ErrorCode::UNMATCH_ARG_TYPE: return "argument type mismatch";
        case ErrorCode::INVALID_ALLOC: return "invalid allocation";
        case ErrorCode::UNKNOWN_CLASS: return "unknown class";
        case ErrorCode::REDEFINE_CLASS: return "redefined class";
        case ErrorCode::MODULO_NON_INT: return "modulo requires integer";
        default: return "error";
        }
    }

    class ComilerError
    {
    private:
        ErrorType type = ErrorType::SYNTAX_ERROR;
        std::string msg;
        sourceLocation location;
        ErrorCode code = ErrorCode::UNEXPECTED_TOKEN;
        Severity severity = Severity::_ERROR;

    public:
        ComilerError(ErrorType type_, const std::string &msg_, const sourceLocation &location_, 
                     ErrorCode code_ = ErrorCode::UNEXPECTED_TOKEN, Severity severity_ = Severity::_ERROR)
            : type(type_), msg(msg_), location(location_), code(code_), severity(severity_) {}

        ComilerError(const std::string &msg_, const sourceLocation &location_, 
                     ErrorCode code_ = ErrorCode::UNEXPECTED_TOKEN)
            : msg(msg_), location(location_), code(code_) {}

        ErrorType getType() const { return type; }
        ErrorCode getCode() const { return code; }
        Severity getSeverity() const { return severity; }
        std::string getFile() const { return location.filename; }
        sourceLocation getLocation() const { return location; }
        const std::string &getMessage() const { return msg; }

        void setSeverity(Severity s) { severity = s; }

        std::string what() const
        {
            return "Error in file " + location.filename + " at line " + std::to_string(location.line) + 
                   ", column " + std::to_string(location.col) + ": " + msg;
        }
    };
}