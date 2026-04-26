#pragma once
#include <string>
#include <unordered_map>
namespace ns{
    
    // Token 类型
    enum class TokenType{
        /* 基础关键字 */
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
        FINALLY,   ///< finally关键字
        FINAL,     ///< final关键字
        EXTERN,    ///< extern关键字
        OVERRIDE,  ///< override关键字
        THIS,      ///< this关键字
        DEBUGGER,  ///< debugger关键字（用于调试）
        /* 基础数据类型 */
        BOOLEAN,     ///< 布尔类型
        STRING,      ///< 字符串类型
        INTEAGER,    ///< 整数类型
        NUMBER,      ///< 浮点数类型

        Scientific,  ///< 科学计数法[unused]
        Hexadecimal, ///< 十六进制数[unused]
        Binary,      ///< 二进制数[unused]
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
    // 关键字映射表
    inline const std::unordered_map<std::string, TokenType> keywords = {
        {"default",TokenType::DEFAULT},{"throw",TokenType::THROW},{"switch", TokenType::SWITCH},{"case", TokenType::CASE},
        {"try", TokenType::TRY},{"catch", TokenType::CATCH},{"debugger", TokenType::DEBUGGER},{"return", TokenType::RETURN},
        {"var", TokenType::VAR},{"const", TokenType::CONST},{"import", TokenType::IMPORT},{"func", TokenType::FUNC},
        {"class", TokenType::CLASS},{"public", TokenType::PUBLIC},{"private", TokenType::PRIVATE},{"protected", TokenType::PROTECTED},
        {"extends", TokenType::EXTENDS},{"operator", TokenType::OPERATOR},{"new", TokenType::NEW},{"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},{"construct", TokenType::CONSTRUCT},{"if", TokenType::IF},{"else", TokenType::ELSE},
        {"do", TokenType::DO},{"until", TokenType::UNTIL},{"while", TokenType::WHILE},{"for", TokenType::FOR},
        {"false", TokenType::FALSE},{"true", TokenType::TRUE},{"null", TokenType::NONE},{"pi", TokenType::PI},
        {"package", TokenType::PACKAGE},{"static", TokenType::STATIC},{"final",TokenType::FINALLY},{"finally",TokenType::FINALLY},
        {"extern",TokenType::EXTERN},{"override",TokenType::OVERRIDE},{"this",TokenType::THIS}};
    // Token 类型到其名字的映射表
    inline const std::unordered_map<TokenType, std::string> names = {
        {TokenType::DEFAULT,"default"},{TokenType::THROW,"throw"},{TokenType::Scientific, "SCIENTIFIC"},{TokenType::Hexadecimal, "HEXADECIMAL"},
        {TokenType::Binary, "BINARY"},{TokenType::SWITCH, "switch"},{TokenType::CASE, "case"},{TokenType::TRY, "try"},
        {TokenType::CATCH, "catch"},{TokenType::DEBUGGER, "debugger"},{TokenType::BIT_AND, "&"},{TokenType::BIT_OR, "|"},
        {TokenType::ANNOTATION, "ANNOTATION"},{TokenType::PACKAGE, "package"},{TokenType::VAR, "var"},{TokenType::CONST, "const"},
        {TokenType::IMPORT, "import"},{TokenType::FUNC, "func"},{TokenType::RETURN, "return"},{TokenType::CLASS, "class"},
        {TokenType::PUBLIC, "public"},{TokenType::PRIVATE, "private"},{TokenType::PROTECTED, "protected"},{TokenType::EXTENDS, "extends"},
        {TokenType::OPERATOR, "operator"},{TokenType::NEW, "new"},{TokenType::BREAK, "break"},{TokenType::CONTINUE, "continue"},
        {TokenType::IF, "if"},{TokenType::ELSE, "else"},{TokenType::DO, "do"},{TokenType::UNTIL, "until"},
        {TokenType::WHILE, "while"},{TokenType::FOR, "for"},{TokenType::NONE, "null"},{TokenType::FALSE, "false"},
        {TokenType::TRUE, "true"},{TokenType::PI, "pi"},{TokenType::BOOLEAN, "bool"},{TokenType::NUMBER, "number"},
        {TokenType::INTEAGER, "inteager"},{TokenType::STRING, "string"},{TokenType::ADD, "+"},{TokenType::MINUS, "-"},
        {TokenType::MUL, "*"},{TokenType::DIV, "/"},{TokenType::MOD, "%"},{TokenType::ADD_ASSIGN, "+="},
        {TokenType::MINUS_ASSIGN, "-="},{TokenType::MUL_ASSIGN, "*="},{TokenType::DIV_ASSIGN, "/="},{TokenType::MOD_ASSIGN, "%="},
        {TokenType::ASSIGN, "="},{TokenType::POINT, "."},{TokenType::EQ, "=="},{TokenType::NEQ, "!="},
        {TokenType::GT, ">"},{TokenType::GTE, ">="},{TokenType::LT, "<"},{TokenType::LTE, "<="},
        {TokenType::AND, "&&"},{TokenType::OR, "or"},{TokenType::LPAREN, "("},{TokenType::RPAREN, ")"},
        {TokenType::LBPAREN, "{"},{TokenType::RBPAREN, "}"},{TokenType::LBRACKET, "["},{TokenType::RBRACKET, "]"},
        {TokenType::COMMA, ","},{TokenType::SEMICOLON, ";"},{TokenType::COLON, ":"},{TokenType::ILLEGAL, "ILLEGAL"},
        {TokenType::END, "END"},{TokenType::CONSTRUCT, ""},{TokenType::IDENT, "IDENT"},{TokenType::STATIC, "static"},
        {TokenType::UNTIL,"until"},{TokenType::COMMAND, "COMMAND"},{TokenType::OVERRIDE,"override"},{TokenType::EXTERN,"extern"},
        {TokenType::FINAL,"final"},{TokenType::FINALLY,"finally"},{TokenType::THIS,"this"}};
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
        Token(sourceLocation _location,const std::string & _literal, TokenType _type);
        sourceLocation getLocation() const;
        std::string getLiteral() const;
        TokenType getType() const;

    public:
        std::string toString() const;
    };
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
}// end of namespace ns