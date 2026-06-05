#ifndef LEXER_H
#define LEXER_H
#include <codecvt>
#include <locale>
#include "token.h"
#include <string>

namespace ns
{
    // nonscript 词法分析器
    class Lexer
    {
    private:
        std::string fileName;  // 当前进行词法分析的文件名（包含完整路径）
        const std::string &source; // 当前进行词法分析的源代码
        char32_t ch = 0;
        int current = 0, next = 1;
        int row = 1, col = 1;
        int startRow = 1, startCol = 1;

    public:
        const std::string &getSource() const { return source; }
        const std::string &getSourceFilename() const { return fileName; }

    public:
        Lexer(const std::string &source_, const std::string &fileName_)
            : source(source_), fileName(fileName_)
        {
            // 若源代码非空，初始化第一个字符
            if (!source.empty())
                ch = source[0];
        }

    private:
        void skip();
        void skipCommand();
        void skipCommandBlock();
        void read();
        char peekChar() const;
        std::string readIdent();
        Token readNumber();
        Token readString(char end);

    public:
        Token scan();
    };
    inline bool isWhiteSpace(const char &ch)
    {
        return ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ';
    }
    inline bool isLetter(const char &ch)
    {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
            return true;
        // // Unicode字母范围
        // // 中文：0x4E00-0x9FFF, 日文：0x3040-0x30FF, 等
        // if ((ch >= 0x4E00 && ch <= 0x9FFF) || // 中文CJK统一表意文字
        //     (ch >= 0x3040 && ch <= 0x30FF) || // 日文假名
        //     (ch >= 0xAC00 && ch <= 0xD7AF))
        // { // 韩文
        //     return true;
        // }
        return false;
    }
    inline bool isNumber(const char &ch)
    {
        return ch >= '0' && ch <= '9';
    }
    inline std::u32string utf8ToUtf32(const std::string &utf8)
    {
        if (utf8.empty())
            return U"";
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        return converter.from_bytes(utf8);
    }
} // end of namespace ns

#endif // LEXER_H
