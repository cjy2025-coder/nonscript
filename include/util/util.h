#pragma once
#include <string>
#include <sstream>

namespace ns
{
    class SourceUtil
    {
    public:
        // 从源代码中提取指定行
        static std::string getLineText(const std::string &source, int line);
        static std::string getCaretPointer(int column);
    };
}