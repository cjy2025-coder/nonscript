#pragma once
#include "./frontend/semantic.h"
#include <fstream>
#include "type.h"

namespace ns
{
    class CplusplusCodeGen
    {
    private:
        CplusplusTypeManager* cpptm = nullptr;
        std::string code;        // main 函数内部代码
        std::string header;      // 头文件
        std::string outer_code;  // 类、函数定义（main 外）
    public:
        CplusplusCodeGen()
        {
            init();
            cpptm = new CplusplusTypeManager();
        }

        ~CplusplusCodeGen()
        {
            delete cpptm;
        }
        std::string get() const
        {
            return header + outer_code + code;
        }

        void save(std::string path) const
        {
            std::ofstream of(path);
            of << header + outer_code + code;
            of.close();
        }

        void run(Program* program)
        {
            for (auto& stmt : program->stmts)
            {
                genStatement(stmt);
            }
            code = "int main() {\n" + code + "}\n";
        }

    private:
        void init(){
            header += "#include<string>\n";
            header += "#include<iostream>\n";
            header += "#include<stdint.h>\n";
            header += "#include<sstream>\n";
            outer_code +="std::string ns_to_string(const std::string& v){return v;}\n"
                        "std::string ns_to_string(int8_t v)  { return std::to_string(v); }\n"
                        "std::string ns_to_string(const char * v){return v; }\n"
                        "std::string ns_to_string(int16_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(int32_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(int64_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(uint8_t v)  { return std::to_string(v); }\n"
                        "std::string ns_to_string(uint16_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(uint32_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(uint64_t v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(float v)  { return std::to_string(v); }\n"
                        "std::string ns_to_string(double v) { return std::to_string(v); }\n"
                        "std::string ns_to_string(bool v) { return v ? \"true\" : \"false\"; }\n"
                        "template<typename Ret, typename... Args>\n"
                        "std::string ns_to_string(Ret(*func)(Args...)) {\n"
                        "   std::ostringstream oss;\n"
                        "   oss << \"<function at 0x\" << std::hex << (void*)func << \">\";\n"
                        "   return oss.str(); "
                        "}\n"
                        "template<typename T>\n"
                        "std::string ns_to_string(const T& obj) {return obj->__toString__();}\n";

        }
        void genStatement(Statement* stmt)
        {
            if (!stmt) return;
            if (auto* s = dynamic_cast<DeclareStatement*>(stmt))
                genDeclareStatement(s);
            else if (auto* s = dynamic_cast<ExpressionStatement*>(stmt))
                genExpressionStatement(s);
            else if (auto* s = dynamic_cast<BlockStatement*>(stmt))
                genBlockStatement(s);
            else if (auto* s = dynamic_cast<ClassLiteral*>(stmt))
                genClassStatement(s);
            else if (auto* s = dynamic_cast<FuncDecl*>(stmt))
                genFuncStatement(s);
            else if (auto* s = dynamic_cast<ShortStatement*>(stmt))
                genShortStatement(s);
            else if (auto* s = dynamic_cast<ReturnStatement*>(stmt))
                genRetStatement(s);
            else if (auto* s = dynamic_cast<ImportStatement*>(stmt))
                genImportStatement(s);
            else if (auto* s = dynamic_cast<TryCatchStatement*>(stmt))
                genTryCatchStatement(s);         
        }
        void genTryCatchStatement(TryCatchStatement * stmt){
            const auto & try_body = stmt->getTryBody();
            const auto & e = stmt->getException();
            const auto & body = stmt->getExceptionBody();

            code += "try{\n";
            genBlockBody(try_body,code);
            code += "\n}\ncatch("+cpptm->getCplusplusType(e->getType())+" "+e->getLiteral()+"){\n";
            genBlockBody(body,code);
            code += "\n}\n";
        }
        //undo
        void genImportStatement(ImportStatement * stmt){
        }
        void genRetStatement(ReturnStatement * stmt){
            code += "return "+genExpression(stmt->getExpression())+";\n";
        }
        void genShortStatement(ShortStatement * stmt){
            code += stmt->getLiteral()+";\n";
        }
        void genExpressionStatement(ExpressionStatement* stmt)
        {
            if (!stmt) return;
            code += genExpression(stmt->expression()) + ";\n";
        }
        std::string escapeString(const std::string &input)
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
                case '\r': output += "\\r";  
                    break;
                default:
                    output += c;
                    break;
                }
            }
            return output;
        }
        std::string genExpression(Expression* expr)
        {
            if (!expr) return "";

            if (auto* e = dynamic_cast<I8Literal*>(expr))    return e->getLiteral();
            if (auto* e = dynamic_cast<I16Literal*>(expr))   return e->getLiteral();
            if (auto* e = dynamic_cast<I32Literal*>(expr))   return e->getLiteral();
            if (auto* e = dynamic_cast<I64Literal*>(expr))   return e->getLiteral();
            if (auto* e = dynamic_cast<Float32Literal*>(expr)) return e->getLiteral();
            if (auto* e = dynamic_cast<Float64Literal*>(expr)) return e->getLiteral();
            if (auto* e = dynamic_cast<BoolLiteral*>(expr))  return e->getLiteral();
            if (auto* e = dynamic_cast<NullLiteral*>(expr))  return "nullptr";

            if (auto* e = dynamic_cast<StringLiteral*>(expr))
            {
                return "\""+escapeString(e->getLiteral())+"\"";
            }

            if (auto* e = dynamic_cast<Ident*>(expr))
                return e->getLiteral();

            if (auto* e = dynamic_cast<NewExpression*>(expr))
            {
                auto right = e->getRight();
                if(auto * r = dynamic_cast<Ident*>(right)){
                    return "new " + r->getLiteral()+"()";
                }
                else if(auto * r = dynamic_cast<CallExpression*>(right)){
                    return "new " + r->getFunc()->getLiteral()+"()";
                }
                return "";
            }

            if (auto* e = dynamic_cast<PrefixExpression*>(expr))
                return e->getOperator() + genExpression(e->getRight());

            if (auto* e = dynamic_cast<InfixExpression*>(expr)){
                std::string op = e->getOperator();
                if(op == "."){
                    op = "->";
                }
                return genExpression(e->getLeft())+ op+ genExpression(e->getRight());
            }
            if (auto* e = dynamic_cast<CallExpression*>(expr))
                return  genCallExpression(e);

            if (auto* e = dynamic_cast<LambdaExpr*>(expr))
                return genLambdaExpression(e);
            if (auto* e = dynamic_cast<IndexExpression*>(expr))
                return genExpression(e->getLeft())+"["+genExpression(e->getIndex())+"]";
            if (auto* e = dynamic_cast<IfExpression*>(expr)){
                return genIfExpression(e);
            }
            if (auto* e = dynamic_cast<WhileExpression*>(expr)){
                return genWhileExpression(e);
            }
            if (auto* e = dynamic_cast<UntilExpression*>(expr)){
                return genUntilExpression(e);
            }
            if (auto* e = dynamic_cast<SwitchExpression*>(expr)){
                return genSwitchExpression(e);
            }
            return expr->toString();
        }
        std::string genSwitchExpression(SwitchExpression * expr){
            const auto & condition = expr->getExpr();
            const auto & cases = expr->getCases();
            const auto & default_case = expr->getDefaultCase();
            std::string ret="switch("+genExpression(condition)+"){\n";
            for(const auto & cs : cases){
                ret += "case "+genExpression(cs->mcase.get())+":{\n";
                genBlockBody(cs->mbody.get(),ret);
                ret += "\n}\n";
            }
            if(default_case){
                ret += "default:{\n";
                genBlockBody(default_case->mbody.get(),ret);
                ret += "\n}\n";
            }
            ret += "}";
            return ret;
        }
        std::string genUntilExpression(UntilExpression * expr){
            const auto & condition = expr->getCondition();
            const auto & body = expr->getBody();
      
            std::string ret="while(!("+genExpression(condition)+")){\n";
            genBlockBody(body,ret);
            ret+="}";
            return ret;     
        }
        std::string genWhileExpression(WhileExpression * expr){
            const auto & condition = expr->getCondition();
            const auto & body = expr->getBody();
      
            std::string ret="while("+genExpression(condition)+"){\n";
            genBlockBody(body,ret);
            ret+="}";
            return ret;
        }
        std::string genIfExpression(IfExpression * expr){
            const auto & condition = expr->getCondition();
            const auto & consequence = expr->getConsequence();
            const auto & alternative = expr->getAlternative();

            std::string ret="if("+genExpression(condition)+"){\n";
            genBlockBody(consequence,ret);
            ret+="\n}\nelse{\n";
            genBlockBody(alternative,ret);
            ret+="}";
            return ret;
        }
    std::string genCallExpression(CallExpression * expr) {
        // 拿到函数名
        auto* ident = dynamic_cast<Ident*>(expr->getFunc());
        if (!ident) {
            // 不是标识符，走普通调用
            std::string ret = genExpression(expr->getFunc());
            ret += "(";
            auto& args = expr->getArgs();
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) ret += ", ";
                ret += genExpression(args[i].get());
            }
            ret += ")";
            return ret;
        }

        std::string func_name = ident->getLiteral();
        auto& args = expr->getArgs();

        // ==============================
        // 内置函数：print
        // ==============================
        if (func_name == "print") {
            std::string code = "std::cout << ";
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) {
                    code += " << ";
                }
                code += "ns_to_string(" + genExpression(args[i].get()) + ")";
            }
            return code; 
        }

        if(func_name == "scan"){
            std::string code = "std::cin >> ";
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) {
                    code += " << ";
                }
                code += genExpression(args[i].get()); 
            }
            return code; 
        }

        // // ==============================
        // // 内置函数：println
        // // ==============================
        // if (func_name == "println") {
        //     std::string code = "std::cout << ";
        //     for (size_t i = 0; i < args.size(); ++i) {
        //         if (i > 0) {
        //             code += " << ";
        //         }
        //         code += "ns_to_string(" + genExpression(args[i].get()) + ")";
        //     }
        //     code += " << std::endl";
        //     return code; // 直接 return！
        // }

        // ==============================
        // 普通函数调用
        // ==============================
        std::string ret = func_name + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) ret += ", ";
            ret += genExpression(args[i].get());
        }
        ret += ")";
        return ret;
    }
        std::string genLambdaExpression(LambdaExpr * expr){
            std::string ret = "[](";
            const auto& args = expr->getParams();
            const auto& body = expr->getBody();
            auto ret_type = expr->ret_type->alias;

            auto cpp_ret = cpptm->getCplusplusType(ret_type);
            for (size_t i = 0; i < args.size(); ++i)
            {
                const auto& arg = args[i];

                if (i > 0)
                    ret += ", ";

                auto arg_type = arg->name->getType();
                auto cpp_arg = cpptm->getCplusplusType(arg_type);
                ret += cpp_arg + " " + arg->name->getLiteral();

                if (arg->init)
                    ret += " = " + genExpression(arg->init.get());
            }

            ret += ") -> ";
            ret += cpp_ret;
            ret += "{\n";
            genBlockBody(body, ret);
            ret += "}";
            return ret;
        }
        void genFuncStatement(FuncDecl* stmt)
        {
            auto func_name = stmt->getLiteral();
            const auto& args = stmt->getParams();
            const auto& body = stmt->getBody();
            auto ret_type = stmt->ret_type->alias;

            auto cpp_ret = cpptm->getCplusplusType(ret_type);
            if(stmt->check_if_extern_func()){
                outer_code += "extern ";
            }
            outer_code += cpp_ret + " " + func_name + "(";

            for (size_t i = 0; i < args.size(); ++i)
            {
                const auto& arg = args[i];

                if (i > 0)
                    outer_code += ", ";

                auto arg_type = arg->name->getType();
                auto cpp_arg = cpptm->getCplusplusType(arg_type);
                outer_code += cpp_arg + " " + arg->name->getLiteral();

                if (arg->init)
                    outer_code += " = " + genExpression(arg->init.get());
            }
            outer_code += ")";
            if(stmt->check_if_extern_func()){
                outer_code += ";\n";
                return ;
            }
            outer_code += "{\n";
            genBlockBody(body, outer_code);
            outer_code += "}\n";
        }
        void genClassStatement(ClassLiteral* stmt)
        {
            auto& parents = stmt->getBaseClasses();
            auto& mems = stmt->getMembers();
            auto class_name = stmt->getLiteral();

            outer_code += "class " + class_name;

            if (!parents.empty())
            {
                outer_code += " : public ";
                for (size_t i = 0; i < parents.size(); ++i)
                {
                    if (i > 0) outer_code += ", public ";
                    outer_code += parents[i]->getLiteral();
                }
            }

            outer_code += " {\n";
            bool use_default_to_string_func = true;
            for (const auto& mem : mems)
            {
                if (mem.access == AccessLevel::Public)
                    outer_code += "public:\n";
                else if (mem.access == AccessLevel::Protected)
                    outer_code += "protected:\n";
                else
                    outer_code += "private:\n";

                if (mem.type == MemberType::Field)
                {
                    if (auto* field = dynamic_cast<DeclareStatement*>(mem.declaration.get()))
                    {
                        for (const auto& var : field->getVars())
                        {
                            const auto& ident = var.first;
                            auto type = ident->getType(); // 空串?
                            auto cpp_type = cpptm->getCplusplusType(type);
                            outer_code += "    " + cpp_type + " " + ident->getLiteral();
                            if(var.second){
                                outer_code += " = "+ genExpression(var.second);
                            }
                            outer_code += ";\n";
                        }
                    }
                }
                else if (mem.type == MemberType::Method)
                {
                    if (auto* func = dynamic_cast<FuncDecl*>(mem.declaration.get()))
                    {
                        if(func->getLiteral() == "__toString__"){ //自定义了类的序列化函数
                            use_default_to_string_func = false;
                        }
                        genFuncStatement(func);
                    }
                }
            }
            if(use_default_to_string_func){ // 生成默认的类序列化函数
               outer_code += "public:\n";
               outer_code += "std::string __toString__() {\n";
               outer_code += "  return \"{\\n";
               outer_code += "  instance of: " + class_name+" \\n";
               outer_code +="}\";\n";
               outer_code += "}\n";
            }
            outer_code += "};\n";
        }

        void genBlockStatement(BlockStatement* stmt)
        {
            code += "{\n";
            genBlockBody(stmt, code);
            code += "}\n";
        }

        void genBlockBody(BlockStatement* stmt, std::string& out)
        {
            if (!stmt) return;
            for (auto& s : stmt->value())
            {
                auto old = code;
                code.clear();
                genStatement(s);
                out += "    " + code;
                code = old;
            }
        }

        void genDeclareStatement(DeclareStatement* stmt)
        {
            if (!stmt) return;

            bool is_const = !stmt->isVariable;
            for (const auto& var : stmt->getVars())
            {
                const auto& ident = var.first;
                const auto& expr = var.second;

                auto type = ident->getType();
                auto cpp_type = cpptm->getCplusplusType(type);

                if (is_const) code += "const ";
                code += cpp_type + " " + ident->getLiteral();

                if (expr)
                    code += " = " + genExpression(expr);

                code += ";\n";
            }
        }
    };
}