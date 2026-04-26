#ifndef BUILTIN_H
#define BUILTIN_H
#include<string>
#include<unordered_map>
#include<vector>
struct builtin_unit{
    std::string name_;
    std::string ret_type_;
    std::vector<std::string> args_type_;
    bool is_unlimited_args_func_;
};

class builtin_manager{
public:
    static std::unordered_map<std::string,builtin_unit> builtins_;
private:
    builtin_manager(){
        builtins_["print"]=builtin_unit{"print","void",{},true};
        builtins_["scan"]=builtin_unit{"scan","void",{},true};
    }
};

#endif