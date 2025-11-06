#include<fstream>
#include<iostream>
#include<string>
#include<sstream>
#include"frontend/lexer.h"
int main(){
     std::ifstream stream("././samples/example.ss");
     if(!stream){
        std::cout<<"无法找到此文件！"<<std::endl;
        exit(-1);
     }
     std::stringstream ss;
     ss<<stream.rdbuf();
     std::string source=ss.str();
     ns::Lexer lexer(source,"example.ss");
     ns::Token token;
     while ((token = lexer.scan()).getType() != ns::TokenType::END)
    {
        std::cout << token.toString() <<"at "<<std::endl;
    }
    return 0;
}