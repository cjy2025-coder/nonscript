#include "frontend/lexer.h"

namespace ns
{
    // 读取下一个字符并更新行列位置
    void Lexer::read()
    {
        if (next >= (int)source.size())
        {
            ch = 0; // 文件结束
            current = next;
        }
        else
        {
            if (ch == '\n') // 如果是换行符
            {
                row++;
                col = 1;
            }
            else
            {
                col++;
            }
            ch = source[next];
            current = next;
            next++;
        }
    }

    // 跳过空白字符
    void Lexer::skip()
    {
        while (isWhiteSpace(ch))
        {
            read();
        }
    }

    // 跳过单行注释
    void Lexer::skipCommand()
    {
        while (ch != '\n' && ch != 0)
        {
            read();
        }
    }

    // 跳过多行注释
    void Lexer::skipCommandBlock()
    {
        while (ch != 0 && (ch != '*' || peekChar() != '/'))
        {
            read();
        }
        if (ch == '*') // 跳过 "*/"
        {
            read();
            read();
        }
    }

    // 查看下一个字符
    char Lexer::peekChar() const
    {
        return next >= (int)source.size() ? 0 : source[next];
    }

    // 读取标识符
    std::string Lexer::readIdent()
    {
        int start = current;
        while (isLetter(ch) || isNumber(ch))
        {
            read();
        }
        return source.substr(start, current - start);
    }

    // 读取数字
    Token Lexer::readNumber()
    {
        int start = current;
        while (isNumber(ch))
        {
            read();
        }
        if (ch == '.')
        { // 是小数
            read();
            while (isNumber(ch))
            {
                read();
            }
            if (ch == 'e' || ch == 'E') // 是科学计数法的小数
            {
                read();
                if (ch == '-')
                    read();
                if (!isNumber(ch))
                { // 不符合科学计数法的格式
                    while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                        read();
                    return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
                }
                while (isNumber(ch))
                {
                    read();
                }
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::Scientific);
            }
            return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::NUMBER);
        }
        else if (ch == 'b' || ch == 'B')
        { // 是二进制数,其后至少有1位数字
            if (current - start != 1)
            { // 格式错误
                while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                    read();
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
            }
            read();
            if (!isNumber(ch))
            { // 格式错误
                while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                    read();
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
            }
            while (isNumber(ch))
            {
                read();
            }
            return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::Binary);
        }
        else if (ch == 'x' || ch == 'X')
        { // 是十六进制数,其后至少有1位数字
            if (current - start != 1)
            { // 格式错误
                while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                    read();
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
            }
            read();
            if (!(ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F'))
            { // 格式错误
                while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                    read();
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
            }
            while (ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F')
            {
                read();
            }
            return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::Hexadecimal);
        }
        else if (ch == 'e' || ch == 'E')
        {
            read();
            if (ch == '-')
                read();
            if (!isNumber(ch))
            {
                while (ch != 0 && ch != ' ' && ch != '\n' && ch != '\r')
                    read();
                return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::ILLEGAL);
            }
            while (isNumber(ch))
            {
                read();
            }
            return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::Scientific);
        }
        else
            return Token({fileName, startRow, startCol}, source.substr(start, current - start), TokenType::INTEAGER);
    }

    // 读取字符串
    Token Lexer::readString(char end)
    {
        read(); // 跳过起始引号
        std::string result;
        while (ch != 0 && ch != end && ch != '\n' && ch != '\r')
        {
            if (ch == '\\') // 处理转义字符
            {
                read();
                if (ch == 'n')
                    result += '\n';
                else if (ch == 'r')
                    result += '\r';
                else if (ch == 't')
                    result += '\t';
                else
                    result += ch;
            }
            else
            {
                result += ch;
            }
            read();
        }
        if (ch != end)
            return Token({fileName, startRow, startCol}, result, TokenType::ILLEGAL);
        read(); // 跳过结束引号
        return Token({fileName, startRow, startCol}, result, TokenType::STRING);
    }

    // 读取下一个 Token
    Token Lexer::scan()
    {
        skip(); // 跳过空白字符

        // 记录当前 Token 的起始位置（应该在真正开始解析 token 前设置）
        startRow = row;
        startCol = col;

        // 跳过注释
        while (ch == '#')
        {
            skipCommand();
            skip();
            // 跳过注释后需要重新设置起始位置
            startRow = row;
            startCol = col;
        }
        while (ch == '/' && peekChar() == '*')
        {
            read();
            read();
            skipCommandBlock();
            skip();
            // 跳过多行注释后需要重新设置起始位置
            startRow = row;
            startCol = col;
        }

        Token t;
        switch (ch)
        {
        case '\'':
        case '\"':
        case '`':
            t = readString(ch);
            return t;
        case '=':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "==", TokenType::EQ);
            }
            else
                t = Token({fileName, startRow, startCol}, "=", TokenType::ASSIGN);
            break;
        case '+':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "+=", TokenType::ADD_ASSIGN);
            }
            else if (peekChar() == '+')
            {
                read();
                t = Token({fileName, startRow, startCol}, "++", TokenType::LINC);
            }
            else
                t = Token({fileName, startRow, startCol}, "+", TokenType::ADD);
            break;
        case '-':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "-=", TokenType::MINUS_ASSIGN);
            }
            else if (peekChar() == '-')
            {
                read();
                t = Token({fileName, startRow, startCol}, "--", TokenType::LDEC);
            }
            else
                t = Token({fileName, startRow, startCol}, "-", TokenType::MINUS);
            break;
        case '*':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "*=", TokenType::MUL_ASSIGN);
            }
            else
                t = Token({fileName, startRow, startCol}, "*", TokenType::MUL);
            break;
        case '.':
            t = Token({fileName, startRow, startCol}, ".", TokenType::POINT);
            break;
        case '/':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "/=", TokenType::DIV_ASSIGN);
            }
            else
                t = Token({fileName, startRow, startCol}, "/", TokenType::DIV);
            break;
        case '%':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "%=", TokenType::MOD_ASSIGN);
            }
            else
                t = Token({fileName, startRow, startCol}, "%", TokenType::MOD);
            break;
        case '@':
            t = Token({fileName, startRow, startCol}, "@", TokenType::ANNOTATION);
            break;
        case '!':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "!=", TokenType::NEQ);
            }
            else
                t = Token({fileName, startRow, startCol}, "!", TokenType::BANG);
            break;
        case '&':
            if (peekChar() == '&')
            {
                read();
                t = Token({fileName, startRow, startCol}, "&&", TokenType::AND);
            }
            else
                t = Token({fileName, startRow, startCol}, "&", TokenType::BIT_AND);
            break;
        case '|':
            if (peekChar() == '|')
            {
                read();
                t = Token({fileName, startRow, startCol}, "||", TokenType::OR);
            }
            else
                t = Token({fileName, startRow, startCol}, "|", TokenType::BIT_OR);
            break;
        case '<':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, "<=", TokenType::LTE);
            }
            else
                t = Token({fileName, startRow, startCol}, "<", TokenType::LT);
            break;
        case '>':
            if (peekChar() == '=')
            {
                read();
                t = Token({fileName, startRow, startCol}, ">=", TokenType::GTE);
            }
            else
                t = Token({fileName, startRow, startCol}, ">", TokenType::GT);
            break;
        case 0:
            t = Token({fileName, startRow, startCol}, "", TokenType::END);
            return t;
        case '{':
            t = Token({fileName, startRow, startCol}, "{", TokenType::LBPAREN);
            break;
        case '}':
            t = Token({fileName, startRow, startCol}, "}", TokenType::RBPAREN);
            break;
        case '(':
            t = Token({fileName, startRow, startCol}, "(", TokenType::LPAREN);
            break;
        case ')':
            t = Token({fileName, startRow, startCol}, ")", TokenType::RPAREN);
            break;
        case '[':
            t = Token({fileName, startRow, startCol}, "[", TokenType::LBRACKET);
            break;
        case ']':
            t = Token({fileName, startRow, startCol}, "]", TokenType::RBRACKET);
            break;
        case ',':
            t = Token({fileName, startRow, startCol}, ",", TokenType::COMMA);
            break;
        case ';':
            t = Token({fileName, startRow, startCol}, ";", TokenType::SEMICOLON);
            break;
        case ':':
            t = Token({fileName, startRow, startCol}, ":", TokenType::COLON);
            break;
        default:
            if (isLetter(ch))
            {
                std::string ident = readIdent();
                t = Token({fileName, startRow, startCol}, ident, lookup(ident));
            }
            else if (isNumber(ch))
            {
                t = readNumber();
            }
            else
            {
                t = Token({fileName, startRow, startCol}, std::string(1, ch), TokenType::ILLEGAL);
                read();
            }
            return t;
        }
        read();
        return t;
    }
}