#pragma once
#include "object.h"

namespace ns
{
    std::shared_ptr<Object> print(std::vector<std::shared_ptr<Object>> &args)
    {
        if (args.empty())
            return std::make_shared<Error>("Expected more arguments instead of got 0!");
        for (size_t i = 0; i < args.size(); i++)
            std::cout << args[i]->toBuf();
        return null;
    }

    std::shared_ptr<Object> scan(std::vector<std::shared_ptr<Object>> &args)
    {
        if (args.empty())
            return std::make_shared<Error>("Expected more arguments instead of got 0!");
        for (size_t i = 0; i < args.size(); i++)
        {
            if (typeid(*(args[i].get())) == typeid(Int8))
            {
                int8_t temp;
                std::cin >> temp;
                ((Int8 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Int16))
            {
                int16_t temp;
                std::cin >> temp;
                ((Int16 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Int32))
            {
                int32_t temp;
                std::cin >> temp;
                ((Int32 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Int64))
            {
                int64_t temp;
                std::cin >> temp;
                ((Int64 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Float32))
            {
                float temp;
                std::cin >> temp;
                ((Float32 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Float64))
            {
                double temp;
                std::cin >> temp;
                ((Float64 *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(Boolean))
            {
                bool temp;
                std::cin >> temp;
                ((Boolean *)(args[i].get()))->setValue(temp);
            }
            else if (typeid(*(args[i].get())) == typeid(String))
            {
                std::string temp;
                std::cin >> temp;
                ((String *)(args[i].get()))->setValue(temp);
            }
            else
            {
                return std::make_shared<Error>("Unexpected in-stream Object type!");
            }
        }
        return null;
    }

    std::shared_ptr<Object> __push(std::vector<std::shared_ptr<Object>> &args)
    {
        if (args.size() != 2 || typeid(*(args[0])) != typeid(Array))
        {
            return std::make_shared<Error>("syntax error,usage push(<Array name>,<Object name>)");
        }
        Array *arr = (Array *)(args[0].get());
        if (typeid(*(args[1].get())) == typeid(String))
        {
            arr->append(std::make_shared<String>(*((String *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Int8))
        {
            arr->append(std::make_shared<Int8>(*((Int8 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Int16))
        {
            arr->append(std::make_shared<Int16>(*((Int16 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Int32))
        {
            arr->append(std::make_shared<Int32>(*((Int32 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Number))
        {
            arr->append(std::make_shared<Int64>(*((Int64 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Float32))
        {
            arr->append(std::make_shared<Float32>(*((Float32 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Float64))
        {
            arr->append(std::make_shared<Float64>(*((Float64 *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Boolean))
        {
            arr->append(std::make_shared<Boolean>(*((Boolean *)(args[1].get()))));
        }
        else if (typeid(*(args[1].get())) == typeid(Array))
        {
            arr->append(args[1]);
        }
        else
        {
            return std::make_shared<Error>("compile error,unknown type, cannot add to array.");
        }
        return null;
    }

    std::shared_ptr<Object> __length(std::vector<std::shared_ptr<Object>> &args)
    {
        if (args.size() == 1)
        {
            auto at = args[0].get();
            if (typeid(*at) == typeid(String))
            {
                return std::make_shared<Int64>(at->toBuf().length());
            }
            else if (typeid(*at) == typeid(Array))
            {
                Array *arr = (Array *)at;
                return std::make_shared<Int64>(arr->size());
            }
            return std::make_shared<Error>("Unexpected type of arg 1,expected `Array` or `String` instead.");
        }
        else
            return std::make_shared<Error>("Expected 1 argument for builtin function _length instead of got "+std::to_string(args.size())+" !");
    }

    std::shared_ptr<Object> __pop(std::vector<std::shared_ptr<Object>> &args)
    {
        if (args.size() == 1)
        {
            auto at = args[0].get();
            if (typeid(*at) != typeid(Array))
                return std::make_shared<Error>("Unexpected type of arg 1,expected `Array` instead.");
            Array *arr = (Array *)(at);
            return arr->pop();
        }
        else
            return std::make_shared<Error>("Expected 1 argument for builtin function _pop instead of got "+std::to_string(args.size())+" !");
    }

    std::map<std::string, BuiltinFunction> BUILTINS = {
        {"__pop", __pop},
        {"__length", __length},
        {"__push", __push},
        {"print", print},
        {"scan", scan}};
}
