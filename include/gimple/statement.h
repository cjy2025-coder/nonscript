#pragma once
#include"gimple/value.h"
#include"gimple/type.h"
namespace ns{
    // GIMPLE 语句基类
class GimpleStatement {
public:
    virtual ~GimpleStatement() = default;
    virtual std::string toGimple() const = 0;
};

// 声明语句
class GimpleDeclare : public GimpleStatement {
public:
    GimpleValue* variable;
    GimpleDeclare(GimpleValue* var) : variable(var) {}
    std::string toGimple() const override {
        return variable->getTypeString() + " " + variable->name + ";";
    }
};

// 赋值语句
class GimpleAssign : public GimpleStatement {
public:
    GimpleValue* lhs;
    GimpleOp op;
    GimpleValue* rhs1;
    GimpleValue* rhs2;  // 对于二元操作
    
    GimpleAssign(GimpleValue* l, GimpleOp o, GimpleValue* r1, GimpleValue* r2 = nullptr)
        : lhs(l), op(o), rhs1(r1), rhs2(r2) {} 
    std::string toGimple() const override {
        std::stringstream ss;
        ss << lhs->name << " = ";
        
        if (op == GimpleOp::ASSIGN) {
            ss << rhs1->toString();
        } else {
            // 二元操作
            ss << rhs1->toString() << " ";
            switch (op) {
                case GimpleOp::ADD: ss << "+"; break;
                case GimpleOp::SUB: ss << "-"; break;
                case GimpleOp::MUL: ss << "*"; break;
                case GimpleOp::DIV: ss << "/"; break;
                case GimpleOp::MOD: ss << "%"; break;
                case GimpleOp::AND: ss << "&&"; break;
                case GimpleOp::OR: ss << "||"; break;
                case GimpleOp::EQ: ss << "=="; break;
                case GimpleOp::NE: ss << "!="; break;
                case GimpleOp::LT: ss << "<"; break;
                case GimpleOp::LTE: ss << "<="; break;
                case GimpleOp::GT: ss << ">"; break;
                case GimpleOp::GTE: ss << ">="; break;
                default: ss << "?"; break;
            }
            ss << " " << rhs2->toString();
        }
        ss << ";";
        return ss.str();
    }
};

// 函数调用
class GimpleCall : public GimpleStatement {
public:
    GimpleValue* result;  // 可为空
    std::string function_name;
    std::vector<GimpleValue*> arguments;
    
    GimpleCall(const std::string& func, 
               const std::vector<GimpleValue*>& args = {},
               GimpleValue* res = nullptr)
        : function_name(func), arguments(args), result(res) {}
    
    std::string toGimple() const override {
        std::stringstream ss;
        if (result) {
            ss << result->name << " = ";
        }
        ss << function_name << "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << arguments[i]->toString();
        }
        ss << ");";
        return ss.str();
    }
};

// 返回语句
class GimpleReturn : public GimpleStatement {
public:
    GimpleValue* value;
    
    GimpleReturn(GimpleValue* v = nullptr) : value(v) {}
    
    std::string toGimple() const override {
        if (value) {
            return "return " + value->toString() + ";";
        }
        return "return;";
    }
};

// 跳转语句
class GimpleGoto : public GimpleStatement {
public:
    std::string label;
    
    GimpleGoto(const std::string& l) : label(l) {}
    
    std::string toGimple() const override {
        return "goto " + label + ";";
    }
};

// 条件跳转
class GimpleCondGoto : public GimpleStatement {
public:
    GimpleValue* condition;
    std::string true_label;
    std::string false_label;
    
    GimpleCondGoto(GimpleValue* cond, const std::string& t, const std::string& f)
        : condition(cond), true_label(t), false_label(f) {}
    
    std::string toGimple() const override {
        return "if (" + condition->toString() + ") goto " + true_label + 
               "; else goto " + false_label + ";";
    }
};

// 标签语句
class GimpleLabel : public GimpleStatement {
public:
    std::string label;
    
    GimpleLabel(const std::string& l) : label(l) {}
    
    std::string toGimple() const override {
        return label + ":";
    }
};

// 作用域块
class GimpleScope {
public:
    std::vector<GimpleStatement*> statements;
    
    void addStatement(GimpleStatement* stmt) {
        if(!stmt){
            exit(-2);
        }
        statements.push_back(stmt);
    }
    
    std::string toGimple(int indent = 0) const {
        std::stringstream ss;
        std::string indent_str(indent, ' ');
        
        ss << indent_str << "{\n";
        for (const auto& stmt : statements) {
            ss << indent_str << "  " << stmt->toGimple() << "\n";
        }
        ss << indent_str << "}";
        return ss.str();
    }
};

// 函数表示
class GimpleFunction {
public:
    std::string name;
    TypeInfo* return_type;
    std::vector<GimpleValue*> parameters;
    std::unique_ptr<GimpleScope> body;
    
    // 临时变量计数器
    int temp_counter;
    // 标签计数器
    int label_counter;
    
    GimpleFunction(const std::string& n, TypeInfo* ret_type)
        : name(n), return_type(ret_type), temp_counter(0), label_counter(0) {
            body=std::make_unique<GimpleScope>();
        }
    
    // 创建临时变量
    GimpleValue* createTemp(TypeInfo* type) {
        std::string name = "D." + std::to_string(temp_counter++);
        return new GimpleValue(name, type, true);
    }
    
    // 创建新标签
    std::string newLabel(const std::string& prefix = "L") {
        return "<" + prefix + std::to_string(label_counter++) + ">";
    }
    
    std::string toGimple() const {
        std::stringstream ss;
        
        // 函数签名
        auto _=std::get<FuncDetail>(return_type->detail);
        ss <<type_to_string(_.return_type)<<" "<< name << " (";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << parameters[i]->getTypeString() << " " << parameters[i]->name;
        }
        ss << ")\n";
        
        // 函数体
        if (body) {
            ss << body->toGimple() << "\n";
        } else {
            ss << "{\n}\n";
        }
        
        return ss.str();
    }
};
}