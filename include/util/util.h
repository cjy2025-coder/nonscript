#pragma once
#include <string>
#include <sstream>

namespace ns
{
    class SourceUtil
    {
    public:
        static std::string getLineText(const std::string &source, int line);
        static std::string getCaretPointer(int column);
    };
    inline std::string escapeString(const std::string &input)
    {
        std::string output;
        for (char c : input)
        {
            switch (c)
            {
            case '\n':
                output += "\\n";
                break;
            case '\t':
                output += "\\t";
                break;
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\r':
                output += "\\r";
                break;
            default:
                output += c;
                break;
            }
        }
        return output;
    }
}
