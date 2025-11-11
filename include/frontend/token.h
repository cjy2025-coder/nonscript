#pragma once
#include <string>
#include <unordered_map>
namespace ns
{
    enum class TokenType
    {
        /* 基础关键字 */
        FLOAT32,   ///< float32关键字
        FLOAT64,   ///< float64关键字
        INT8,      ///< int8关键字
        INT16,     ///< int16关键字
        INT32,     ///< int32关键字
        INT64,     ///< int64关键字
        ANY,       ///< any关键字
        STR,       ///< str关键字
        BOOL,      ///< bool关键字
        VAR,       ///< var 关键字
        CONST,     ///< const 关键字
        IMPORT,    ///< import 关键字
        FUNC,      ///< func 关键字
        CLASS,     ///< class 关键字
        RETURN,    ///< return 关键字
        PUBLIC,    ///< public 关键字
        PRIVATE,   ///< private 关键字
        PROTECTED, ///< protected 关键字
        EXTENDS,   ///< extends 关键字
        OPERATOR,  ///< operator 关键字
        NEW,       ///< new 关键字
        BREAK,     ///< break 关键字
        CONTINUE,  ///< continue 关键字
        IF,        ///< if 关键字
        ELSE,      ///< else 关键字
        DO,        ///< do 关键字
        UNTIL,     ///< until 关键字
        WHILE,     ///< while 关键字
        FOR,       ///< for 关键字
        CONSTRUCT, ///< construct 关键字
        STATIC,    ///< static 关键字
        PACKAGE,   ///< package 关键字
        SWITCH,    ///< switch关键字
        CASE,      ///< case关键字
        DEFAULT,   ///< default关键字
        TRY,       ///< try关键字
        CATCH,     ///< catch关键字
        THROW,     ///< throw关键字
        DEBUGGER,  ///< debugger关键字（用于调试）
        /* 基础数据类型 */
        BOOLEAN,     ///< 布尔类型
        STRING,      ///< 字符串类型
        INTEAGER,    ///< 整数类型
        NUMBER,      ///< 浮点数类型
        Scientific,  ///< 科学计数法
        Hexadecimal, ///< 十六进制数
        Binary,      ///< 二进制数
        /* 基础常量 */
        NONE,  ///< null 常量
        TRUE,  ///< true 常量
        FALSE, ///< false 常量
        PI,    ///< π 常量

        /* 基础二元运算符 */
        ADD,          ///< 加法运算符 +
        MINUS,        ///< 减法运算符 -
        MUL,          ///< 乘法运算符 *
        DIV,          ///< 除法运算符 /
        MOD,          ///< 取模运算符 %
        ADD_ASSIGN,   ///< +=
        MINUS_ASSIGN, ///< -=
        DIV_ASSIGN,   /// /=
        MUL_ASSIGN,   /// *=
        MOD_ASSIGN,   /// %=
        ASSIGN,       ///< 赋值运算符 =
        NEQ,          ///< 不等于运算符 !=
        EQ,           ///< 等于运算符 ==
        GT,           ///< 大于运算符 >
        GTE,          ///< 大于等于运算符 >=
        LT,           ///< 小于运算符 <
        LTE,          ///< 小于等于运算符 <=
        AND,          ///< 逻辑与 &&
        OR,           ///< 逻辑或 ||
        BIT_AND,      ///< 按位与 &
        BIT_OR,       ///< 按位或 |
        POINT,        ///< 访问成员运算符 .

        /* 基础一元运算符 */
        BANG,  ///< 逻辑非运算符 !
        LINC,  ///< 左自增运算符 ++
        LDEC,  ///< 左自减运算符 --
        /* 基础符号 */
        LPAREN,     ///< 左括号 (
        RPAREN,     ///< 右括号 )
        LBPAREN,    ///< 左大括号 {
        RBPAREN,    ///< 右大括号 }
        LBRACKET,   ///< 左中括号 [
        RBRACKET,   ///< 右中括号 ]
        COMMA,      ///< 逗号 ,
        SEMICOLON,  ///< 分号 ;
        COLON,      ///< 冒号 :
        ANNOTATION, ///< 注解符号 @
        /* 其它 */
        IDENT,      ///< 标识符
        ILLEGAL,    ///< 非法 Token
        END,        ///< 文件结束
        COMMAND,    ///< 注释
        PLACEHOLDER ///< 占位符
    };

    /**
     * @brief 关键字及常量表，用于快速查找 Token 类型
     */
    static std::unordered_map<std::string, TokenType> keywords = {
        {"default",TokenType::DEFAULT},
        {"throw",TokenType::THROW},
        {"switch", TokenType::SWITCH},
        {"case", TokenType::CASE},
        {"try", TokenType::TRY},
        {"catch", TokenType::CATCH},
        {"debugger", TokenType::DEBUGGER},
        {"return", TokenType::RETURN},
        {"var", TokenType::VAR},
        {"const", TokenType::CONST},
        {"import", TokenType::IMPORT},
        {"func", TokenType::FUNC},
        {"class", TokenType::CLASS},
        {"public", TokenType::PUBLIC},
        {"private", TokenType::PRIVATE},
        {"protected", TokenType::PROTECTED},
        {"extends", TokenType::EXTENDS},
        {"operator", TokenType::OPERATOR},
        {"new", TokenType::NEW},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"construct", TokenType::CONSTRUCT},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"do", TokenType::DO},
        {"until", TokenType::UNTIL},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"false", TokenType::FALSE},
        {"true", TokenType::TRUE},
        {"null", TokenType::NONE},
        {"pi", TokenType::PI},
        {"package", TokenType::PACKAGE},
        {"static", TokenType::STATIC}};

    /**
     * @brief Token 类型到字符串的映射表
     */
    static std::unordered_map<TokenType, std::string> names = {
        {TokenType::DEFAULT,"default"},
        {TokenType::THROW,"THROW"},
        {TokenType::Scientific, "SCIENTIFIC"},
        {TokenType::Hexadecimal, "HEXADECIMAL"},
        {TokenType::Binary, "BINARY"},
        {TokenType::SWITCH, "SWITCH"},
        {TokenType::CASE, "CASE"},
        {TokenType::TRY, "TRY"},
        {TokenType::CATCH, "CATCH"},
        {TokenType::DEBUGGER, "DEBUGGER"},
        {TokenType::BIT_AND, "BIT_AND"},
        {TokenType::BIT_OR, "BIT_OR"},
        {TokenType::ANNOTATION, "ANNOTATION"},
        {TokenType::PACKAGE, "PACKAGE"},
        {TokenType::VAR, "VAR"},
        {TokenType::CONST, "CONST"},
        {TokenType::IMPORT, "IMPORT"},
        {TokenType::FUNC, "FUNC"},
        {TokenType::RETURN, "RETURN"},
        {TokenType::CLASS, "CLASS"},
        {TokenType::PUBLIC, "PUBLIC"},
        {TokenType::PRIVATE, "PRIVATE"},
        {TokenType::PROTECTED, "PROTECTED"},
        {TokenType::EXTENDS, "EXTENDS"},
        {TokenType::OPERATOR, "OPERATOR"},
        {TokenType::NEW, "NEW"},
        {TokenType::BREAK, "BREAK"},
        {TokenType::CONTINUE, "CONTINUE"},
        {TokenType::IF, "IF"},
        {TokenType::ELSE, "ELSE"},
        {TokenType::DO, "DO"},
        {TokenType::UNTIL, "UNTIL"},
        {TokenType::WHILE, "WHILE"},
        {TokenType::FOR, "FOR"},
        {TokenType::NONE, "NULL"},
        {TokenType::FALSE, "FALSE"},
        {TokenType::TRUE, "TRUE"},
        {TokenType::PI, "PI"},
        {TokenType::BOOLEAN, "BOOLEAN"},
        {TokenType::NUMBER, "NUMBER"},
        {TokenType::INTEAGER, "INTEAGER"},
        {TokenType::STRING, "STRING"},
        {TokenType::ADD, "ADD"},
        {TokenType::MINUS, "MINUS"},
        {TokenType::MUL, "MUL"},
        {TokenType::DIV, "DIV"},
        {TokenType::MOD, "MOD"},
        {TokenType::ADD_ASSIGN, "ADD_ASSIGN"},
        {TokenType::MINUS_ASSIGN, "MINUS_ASSIGN"},
        {TokenType::MUL_ASSIGN, "MUL_ASSIGN"},
        {TokenType::DIV_ASSIGN, "DIV_ASSIGN"},
        {TokenType::MOD_ASSIGN, "MOD_ASSIGN"},
        {TokenType::ASSIGN, "ASSIGN"},
        {TokenType::POINT, "POINT"},
        {TokenType::EQ, "EQ"},
        {TokenType::NEQ, "NEQ"},
        {TokenType::GT, "GT"},
        {TokenType::GTE, "GTE"},
        {TokenType::LT, "LT"},
        {TokenType::LTE, "LTE"},
        {TokenType::AND, "AND"},
        {TokenType::OR, "OR"},
        {TokenType::LPAREN, "LPAREN"},
        {TokenType::RPAREN, "RPAREN"},
        {TokenType::LBPAREN, "LBPAREN"},
        {TokenType::RBPAREN, "RBPAREN"},
        {TokenType::LBRACKET, "LBRACKET"},
        {TokenType::RBRACKET, "RBRACKET"},
        {TokenType::COMMA, "COMMA"},
        {TokenType::SEMICOLON, "SEMICOLON"},
        {TokenType::COLON, "COLON"},
        {TokenType::ILLEGAL, "ILLEGAL"},
        {TokenType::END, "END"},
        {TokenType::CONSTRUCT, "CONSTRUCT"},
        {TokenType::IDENT, "IDENT"},
        {TokenType::STATIC, "STATIC"},
        {TokenType::UNTIL,"UNTIL"},
        {TokenType::COMMAND, "COMMAND"}};
    inline std::string getName(TokenType type){
        auto name=names.find(type);
        if(name != names.end()){
            return name->second;
        }
        else return "";
    }
    typedef struct sourceLocation
    {
        std::string filename;
        int line, col;
    } sourceLocation;

    class Token
    {
    private:
        sourceLocation location;
        std::string literal;
        TokenType type;

    public:
        Token() {}
        Token(sourceLocation _location, std::string _literal, TokenType _type);
        sourceLocation getLocation() const;
        std::string getLiteral() const;
        TokenType getType() const;

    public:
        std::string toString() const;
    };

    /**
     * @brief 查找关键字或常量表，判断是否是关键字或常量
     * @param literal 字符串字面量
     * @return 对应的 TOKEN 类型
     */
    inline TokenType lookup(const std::string &literal)
    {
        if (literal == "or")
            return TokenType::OR;
        else if (literal == "and")
            return TokenType::AND;
        auto it = keywords.find(literal);
        if (it != keywords.end())
            return it->second;
        return TokenType::IDENT;
    }

}