#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>

namespace ns
{
    /**
     * 词法分析器（Lexer）类
     * 负责将源代码字符串转换为一系列 Token（词法单元），
     * 是编译过程的第一个阶段，为语法分析器提供输入。
     */
    class Lexer
    {
    private:
        const std::string fileName;     // 源代码文件名（用于错误提示）
        const std::string source;       // 待分析的源代码字符串
        char ch = 0;                    // 当前正在处理的字符
        int current = 0, next = 1;      // current：当前字符在 source 中的索引；next：下一个字符的索引
        int row = 1, col = 1;           // 当前字符的行号和列号（用于定位错误位置）
        int startRow = 1, startCol = 1; // 当前 Token 的起始行号和列号（记录 Token 在源代码中的位置）

    public:
        /**
         * @brief 获取源代码字符串
         * @return 常量引用返回源代码，避免复制开销
         */
        const std::string &getSource() const { return source; }

    public:
        /**
         * @brief 构造函数，初始化词法分析器
         * @param source_ 源代码字符串
         * @param fileName_ 源代码文件名
         */
        Lexer(const std::string &source_, const std::string &fileName_)
            : source(source_), fileName(fileName_)
        {
            // 若源代码非空，初始化第一个字符
            if (!source.empty())
                ch = source[0];
        }

    private:
        /**
         * @brief 跳过空白字符（空格、制表符、换行符等）
         * 同时更新行号和列号信息
         */
        void skip();

        /**
         * @brief 跳过单行注释（如#注释内容）
         * 从 # 开始，直到换行符结束
         */
        void skipCommand();

        /**
         * @brief 跳过多行注释（如 /* ... */
            void skipCommandBlock();

        /**
         * @brief 读取下一个字符，并更新 current、next 索引及行列号
         * 若已到达源代码末尾，ch 会被设置为 0（空字符）
         */
        void read();

        /**
         * @brief 查看下一个字符（不移动 current 索引）
         * @return 下一个字符；若已到末尾，返回 0
         */
        char peekChar() const;

        /**
         * @brief 读取标识符（变量名、关键字等）
         * 标识符规则：以字母或下划线开头，后跟字母、数字或下划线
         * @return 读取到的标识符字符串
         */
        std::string readIdent();

        /**
         * @brief 读取数字字面量（整数、浮点数等）
         * 支持十进制数字，可能包含小数点（.）或科学计数法（暂未实现）
         * @return 包含数字值和类型的 Token
         */
        Token readNumber();

        /**
         * @brief 读取字符串字面量（如 "hello" 或 'world'）
         * @param end 字符串结束符（" 或 '）
         * @return 包含字符串内容的 Token
         */
        Token readString(char end);

    public:
        /**
         * @brief 扫描并返回下一个 Token
         * 依次处理源代码，跳过空白和注释，识别关键字、标识符、数字、字符串等
         * @return 解析到的下一个 Token（包含类型、值、位置信息）
         */
        Token scan();
    };

    /**
     * @brief 判断字符是否为空白字符（空格、\t、\n、\r）
     * @param ch 待判断的字符
     * @return 是空白字符返回 true，否则返回 false
     */
    inline bool isWhiteSpace(const char &ch)
    {
        return ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ';
    }

    /**
     * @brief 判断字符是否为字母（含下划线，符合标识符命名规则）
     * @param ch 待判断的字符
     * @return 是字母或下划线返回 true，否则返回 false
     */
    inline bool isLetter(const char &ch)
    {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
    }

    /**
     * @brief 判断字符是否为数字（0-9）
     * @param ch 待判断的字符
     * @return 是数字返回 true，否则返回 false
     */
    inline bool isNumber(const char &ch)
    {
        return ch >= '0' && ch <= '9';
    }
}

#endif // LEXER_H
