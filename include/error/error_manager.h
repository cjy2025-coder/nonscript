#pragma once
#include "error.h"
#include <vector>
#include <string>
#include <iostream>
namespace ns
{
    class ErrorManager
    {
    private:
        std::vector<ComilerError> errors; ///>存储错误信息的容器
    public:
        /**
         * @brief 添加错误信息
         * @param error 错误信息
         */
        void report(ComilerError error)
        {
            errors.push_back(error);
        }
        void report(ErrorType type_, const std::string &msg_, const sourceLocation &location_)
        {
            errors.push_back(ComilerError(type_, msg_, location_));
        }
        /**
         * @brief 获取所有错误信息
         * @return 错误信息列表
         */
        const std::vector<ComilerError> &getAll() const
        {
            return errors;
        }

        /**
         * @brief 清空错误信息
         */
        void clear()
        {
            errors.clear();
        }
        /**
         * @brief 检查是否有错误
         * @return 如果有错误返回 true，否则返回 false
         */
        bool has() const
        {
            return !errors.empty();
        }
        std::string whats() const
        {
            std::string result;
            for (int i = 0; i < errors.size(); i++)
            {
                result += errors[i].what();
                if (i != errors.size() - 1)
                    result += "\n";
            }
            return result;
        }

        int num() const
        {
            return errors.size();
        }
    };
}