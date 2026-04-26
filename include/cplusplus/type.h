#pragma once
#include<string>
#include<unordered_map>
namespace ns{
    class CplusplusTypeManager{
      std::unordered_map<std::string,std::string> type_mapper={
          {"i8","signed char"},{"i16","short"},{"i32","int"},{"i64","long long"},
          {"f32","float"},{"f64","double"},
          {"string","std::string"},
          {"func","auto"},
          {"bool","bool"},
          {"void","void"},
          {"exception","exception"}
       };
   public:
       std::string getCplusplusType(std::string _ns_type) const{
          auto it=type_mapper.find(_ns_type);
          if(it != type_mapper.end()){
             return it->second;
          }
          return _ns_type+"*";
       }
    };
}