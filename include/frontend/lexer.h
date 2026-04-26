#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>

namespace ns
{
    // nonscript 词法分析器
    class Lexer
    {
    private:
        const std::string & fileName; //当前进行词法分析的文件名（包含完整路径）    
        const std::string & source; // 当前进行词法分析的源代码      
        char ch = 0;                    
        int current = 0, next = 1;      
        int row = 1, col = 1;           
        int startRow = 1, startCol = 1;
    public:
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
         * @brief
         * 跳过空白字符，如空格、制表符、换行符等
         * 同时更新行号和列号信息
         */
        void skip();

        /**
         * @brief 
         * 跳过单行注释，在 nonscript 中使用 # 进行单行注释
         */
        void skipCommand();
        /**
         * @brief 跳过多行注释，在 nonscript 中使用 cpp 风格的块注释
         */
        void skipCommandBlock();
        /**
         * @brief 读取下一个字符，并更新 current、next 索引及行列号
         * 若已到达源代码末尾，ch 会被设置为 0（空字符）
         */
        void read();

        /**
         * @brief 查看下一个字符（不移动 current 索引）
         */
        char peekChar() const;

        /**
         * @brief 读取标识符
         */
        std::string readIdent();

        /**
         * @brief 读取数字字面量
         */
        Token readNumber();

        /**
         * @brief 读取字符串字面量
         */
        Token readString(char end);

    public:
        /**
         * @brief 扫描并返回下一个 Token
         */
        Token scan();
    };

    /**
     * @brief 判断字符是否为空白字符，如空格、\t、\n、\r
     */
    inline bool isWhiteSpace(const char &ch){
        return ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ';
    }

    /**
     * @brief 判断字符是否为字母，含下划线
     */
    inline bool isLetter(const char &ch){
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
    }

    /**
     * @brief 判断字符是否为数字
     */
    inline bool isNumber(const char &ch){
        return ch >= '0' && ch <= '9';
    }
} // end of namespace ns

#endif // LEXER_H
