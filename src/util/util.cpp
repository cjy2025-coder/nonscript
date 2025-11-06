#include "./util/util.h"

namespace ns
{
    std::string SourceUtil::getLineText(const std::string &source, int line)
    {
        std::istringstream stream(source);
        std::string lineText;
        for (int i = 0; i < line; ++i)
        {
            if (!std::getline(stream, lineText))
            {
                return ""; // 如果行数超过了源代码的行数，返回空字符串
            }
        }
        return lineText + "\n";
    }

    std::string SourceUtil::getCaretPointer(int column)
    {
        if (column < 1)
        {
            return ""; // 如果列数小于1，返回空字符串
        }
        return std::string(column - 1, ' ') + "^"; // 返回一个指向指定列的指针
    }
}