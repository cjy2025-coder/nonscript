#pragma once
#include "frontend/ast.h"
#include <cfloat>
#include <functional>
#include <map>
namespace ns
{
    class Enviroment;
    enum OBJECT
    {
        OBJ_STRING,
        OBJ_FUNC,
        OBJ_BOOL,
        OBJ_INT8,
        OBJ_INT16,
        OBJ_INT32,
        OBJ_INT64,
        OBJ_FLOAT32,
        OBJ_FLOAT64,
        OBJ_ARRAY,
        OBJ_NULL,
        OBJ_CLASS,
        OBJ_INSTANCE,
        OBJ_ERROR,
        OBJ_BUILTIN,
        OBJ_RET,
        OBJ_SHORT,
        OBJ_EXCEPTION,
        OBJ_EXCP
    };

    class Object
    {
    protected:
        OBJECT type;
        
    public:
        Object(const OBJECT &type_) : type(type_) {}

    public:
        OBJECT getType() const
        {
            return type;
        }
        static bool equal(std::shared_ptr<Object> obj1, std::shared_ptr<Object> obj2)
        {
            return obj1->toBuf() == obj2->toBuf();
        }
        virtual std::string toBuf() const = 0;
    };

    class Null : public Object
    {
    public:
        Null() : Object(OBJ_NULL) {}
        std::string toBuf() const override
        {
            return "null";
        }
    };
    class Number : public Object
    {
    public:
        Number(OBJECT type_) : Object(type_) {}
        virtual std::string toBuf() const = 0;
        // OBJECT getNumberType() const {
        //     return type;
        // }
    };
    class Int8 : public Number
    {
    private:
        int8_t value;

    public:
        Int8(int8_t value_) : value(value_), Number(OBJ_INT8) {}

    public:
        int8_t getValue() const
        {
            return value;
        }
        void setValue(int8_t value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };
    class Int16 : public Number
    {
    private:
        int16_t value;

    public:
        Int16(int16_t value_) : value(value_), Number(OBJ_INT16) {}

    public:
        int16_t getValue() const
        {
            return value;
        }
        void setValue(int16_t value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };
    class Int32 : public Number
    {
    private:
        int32_t value;

    public:
        Int32(int32_t value_) : value(value_), Number(OBJ_INT32) {}

    public:
        int32_t getValue() const
        {
            return value;
        }
        void setValue(int32_t value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };
    class Int64 : public Number
    {
    private:
        int64_t value;

    public:
        Int64(int64_t value_) : value(value_), Number(OBJ_INT64) {}

    public:
        int64_t getValue() const
        {
            return value;
        }
        void setValue(int64_t value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };

    class Float32 : public Number
    {
    private:
        float value;

    public:
        Float32(float value_) : value(value_), Number(OBJ_FLOAT32) {}

    public:
        float getValue() const
        {
            return value;
        }
        void setValue(float value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };
    class Float64 : public Number
    {
    private:
        double value;

    public:
        Float64(double value_) : value(value_), Number(OBJ_FLOAT64) {}

    public:
        double getValue() const
        {
            return value;
        }
        void setValue(double value_)
        {
            value = value_;
        }
        std::string toBuf() const override
        {
            return std::to_string(value);
        }
    };

    class String : public Object
    {
    private:
        std::string value;

    private:
        bool isVariable;

    public:
        String(std::string value_, bool isVariable_ = true) : Object(OBJ_STRING), value(value_), isVariable(isVariable_) {}

    public:
        std::string getValue() const
        {
            return value;
        }
        std::string toBuf() const override
        {
            return value;
        }
        void setValue(std::string v)
        {
            value = v;
        }
        bool variable() const
        {
            return isVariable;
        }
    };

    class Boolean : public Object
    {
    private:
        bool value;

    private:
        bool isVariable;

    public:
        Boolean(bool value_, bool isVariable_ = true) : Object(OBJ_BOOL), value(value_), isVariable(isVariable_) {}

    public:
        bool getValue() const
        {
            return value;
        }
        void setValue(bool v)
        {
            value = v;
        }
        std::string toBuf() const override
        {
            return value ? "true" : "false";
        }
        bool variable() const
        {
            return isVariable;
        }
    };
    std::shared_ptr<Null> null = std::make_shared<Null>();
    class Array : public Object
    {
    private:
        std::vector<std::shared_ptr<Object>> elems;

    private:
        bool isVariable;

    public:
        Array(const std::vector<std::shared_ptr<Object>> &elems_, bool isVariable_ = true) : Object(OBJ_ARRAY), elems(elems_), isVariable(isVariable_) {}
        const std::vector<std::shared_ptr<Object>> &getElems()
        {
            return elems;
        }
        void setElem(std::shared_ptr<Object> e, int index)
        {
            if (index < 0 || index >= (int)elems.size())
                return;
            elems[index] = e;
        }
        void append(const std::shared_ptr<Object> &e)
        {
            elems.push_back(e);
        }
        int size()
        {
            return elems.size();
        }
        std::shared_ptr<Object> pop()
        {
            if (!elems.empty())
            {
                auto e = elems[elems.size() - 1];
                elems.pop_back();
                return e;
            }
            else
            {
                return null;
            }
        }
        std::string toBuf() const override
        {
            std::string ret = "[";
            for (size_t i = 0; i < elems.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret += elems[i]->toBuf();
            }
            ret += "]";
            return ret;
        }
        bool variable() const
        {
            return isVariable;
        }
    };

    class Error : public Object
    {
    private:
        std::string error;

    public:
        Error(const std::string &error_) : error(error_), Object(OBJ_ERROR) {}
        std::string getValue() const
        {
            return error;
        }
        std::string toBuf() const override
        {
            return error;
        }
    };

    class Exception : public Object
    {
    private:
        std::string exception;

    public:
        Exception(const std::string &exception_) : exception(exception_), Object(OBJ_EXCEPTION) {}
        std::string getValue() const
        {
            return exception;
        }
        std::string toBuf() const override
        {
            return exception;
        }
    };

    class Func : public Object
    {
    private:
        std::vector<std::shared_ptr<Expression>> params;
        std::shared_ptr<BlockStatement> body;
        mutable std::shared_ptr<Enviroment> env_;

    public:
        Func(std::vector<std::shared_ptr<Expression>> params_,
             BlockStatement *body_,
             std::shared_ptr<Enviroment> env) : Object(OBJECT::OBJ_FUNC),
                                                params(params_),
                                                body(body_),
                                                env_(env) {}
        std::shared_ptr<Enviroment> &getEnv()
        {
            return env_;
        }
        void setEnv(std::shared_ptr<Enviroment> env)
        {
            env_ = env;
        }
        BlockStatement *getBody()
        {
            return body.get();
        }
        int params_num()
        {
            return params.size();
        }
        std::vector<std::shared_ptr<Expression>> getParams()
        {
            return params;
        }
        std::string toBuf() const override
        {
            std::string ret = "func (";
            for (size_t i = 0; i < params.size(); i++)
            {
                if (i != 0)
                    ret.append(", ");
                ret.append(params[i]->toString());
            }
            ret += "){\n";
            ret += body->toString();
            ret += "}\n";
            return ret;
        }
    };

    using BuiltinFunction = std::function<std::shared_ptr<Object>(std::vector<std::shared_ptr<Object>> &)>;
    class Builtin : public Object
    {
    public:
        Builtin(const BuiltinFunction &fn) : Object(OBJECT::OBJ_BUILTIN),
                                             _fn(fn)
        {
        }

        std::shared_ptr<Object> run(std::vector<std::shared_ptr<Object>> &args) const
        {
            return _fn(args);
        }
        std::string toBuf() const override
        {
            return "bulitin func";
        }

    private:
        BuiltinFunction _fn;
    };

    class ReturnValue : public Object
    {
    private:
        std::shared_ptr<Object> value;

    public:
        ReturnValue(std::shared_ptr<Object> &obj) : Object(OBJECT::OBJ_RET), value(obj) {}
        std::shared_ptr<Object> get_value()
        {
            return value;
        }
        std::string toBuf() const override
        {
            return value->toBuf();
        }
    };
        class ExceptionValue : public Object
    {
    private:
        std::shared_ptr<Object> value;

    public:
        ExceptionValue(std::shared_ptr<Object> &obj) : Object(OBJECT::OBJ_EXCP), value(obj) {}
        std::shared_ptr<Object> get_value() const
        {
            return value;
        }
        std::string toBuf() const override
        {
            return value->toBuf();
        }
    };
#define SHORT_CONTINUE 0
#define SHORT_BREAK 1
    class Short : public Object
    {
    private:
        int value;

    public:
        Short(int value_) : value(value_), Object(OBJ_SHORT) {}
        std::string toBuf() const override
        {
            if (value == SHORT_BREAK)
            {
                return "break";
            }
            else if (value == SHORT_CONTINUE)
            {
                return "continue";
            }
            else
            {
                return "unknown";
            }
        }
    };

    class Class : public Object
    {
    private:
        mutable std::shared_ptr<Enviroment> staticEnv;         // 静态成员环境
        std::map<std::string, AccessLevel> staticMemberAccess; // 类静态成员访问权限
    public:
        ClassLiteral *classLiteral; // 类的信息
    public:
        Class(ClassLiteral *cl, std::shared_ptr<Enviroment> env_)
            : Object(OBJ_CLASS)
        {
            staticEnv = env_;
            // 处理类的静态成员
            // 类的非静态成员
            classLiteral = cl;
        }

    public:
        std::shared_ptr<Enviroment> getEnv() const
        {
            return staticEnv;
        }
        int getMemberType(std::string name) const
        {
            auto it = staticMemberAccess.find(name);
            if (it != staticMemberAccess.end())
            {
                return it->second;
            }
            return -1; // 不是类静态成员
        }

    public:
        std::string toBuf() const override
        {
            std::string ret = "class {\n";
            return ret;
        }
    };

    class ClassInstance : public Object
    {
    private:
        std::shared_ptr<Enviroment> env;                 // 实例的环境
        std::map<std::string, AccessLevel> memberAccess; // 类成员访问权限
    public:
        ClassInstance(std::shared_ptr<Enviroment> env_) : Object(OBJ_INSTANCE)
        {
            env = env_;
        }

    public:
        std::shared_ptr<Enviroment> getEnv() const
        {
            return env;
        }
        void setMemberAccess(std::string name, AccessLevel access)
        {
            memberAccess[name] = access;
        }
        int getMemberType(std::string name) const
        {
            auto it = memberAccess.find(name);
            if (it != memberAccess.end())
            {
                return it->second;
            }
            return -1; // 不是类成员
        }
        std::string toBuf() const override
        {
            std::string ret = "instance {\n";
            for (auto &pair : memberAccess)
            {
                ret += pair.first + ": " + std::to_string(pair.second) + "\n";
            }
            return ret + "}\n";
        }
    };

    std::shared_ptr<Boolean> True = std::make_shared<Boolean>(true);
    std::shared_ptr<Boolean> False = std::make_shared<Boolean>(false);
    std::shared_ptr<Short> Break = std::make_shared<Short>(SHORT_BREAK);
    std::shared_ptr<Short> Continue = std::make_shared<Short>(SHORT_CONTINUE);
}
