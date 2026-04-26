#pragma once
#include<string>
#include<map>
namespace ns
{
    typedef struct _type{
       std::string alias;
       int id; 
    }*_ptype;
    class typeManager{
       std::map<std::string,_type> types;
       int id = 0;
    private:
       void defaultEmit(){
          emit("i8");
          emit("i16");
          emit("i32");
          emit("i64");
          emit("f32");
          emit("f64");
          emit("bool");
          emit("array");
          emit("string");
          emit("object");
          emit("func");
          emit("undefined");
          emit("void");
          emit("excpetion");
          emit("error");
       }
    public:
       void emit(std::string alias){
         _type _={};
         _.alias=alias;
         _.id=id++;
         types[alias]=_;
       }
       typeManager(){
         defaultEmit();
       }
       _type * find(std::string alias){
           auto type=types.find(alias);
           if(type == types.end()){
              return nullptr;
           }
           return &(type->second);
       }
    //    _type get(std::string alias) const{
    //       return types.find(alias);
    //    }
    };
}