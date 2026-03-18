#pragma once
#include "frontend/semantic.h"
namespace ns
{ 
    class GimpleValue
    {
        
    public:
        std::string name;
        TypeInfo *type;
        bool is_temp, is_constant;
        std::string constant_value;
        GimpleValue(const std::string &n, TypeInfo *t, bool temp = false)
            : name(n), type(t), is_temp(temp), is_constant(false) {}
        std::string toString() const
        {
            if (is_constant)
                return constant_value;
            return name;
        }
        std::string getTypeString() const
        {
            if (!type)
                return "unknown";
            switch (type->baseType)
            {
            case DataType::INT8:
                return "int8";
            case DataType::INT16:
                return "int16";
            case DataType::INT32:
                return "int32";
            case DataType::INT64:
                return "int64";
            case DataType::FLOAT32:
                return "float32";
            case DataType::FLOAT64:
                return "float64";
            case DataType::BOOLEAN:
                return "bool";
            case DataType::STRING:
                return "string";
            case DataType::NONE:
                return "void";
            default:
                return "unknown";
            }
        }

    };
}