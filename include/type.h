#pragma once
#include <string>
#include <map>
#include <unordered_map>
namespace ns
{
   class CplusplusTypeManager
   {
      std::unordered_map<std::string, std::string> type_mapper = {
          {"i8", "signed char"}, {"i16", "short"}, {"i32", "int"}, {"i64", "long long"}, {"f32", "float"}, {"f64", "double"}, {"string", "std::string"}, {"func", "auto"}, {"bool", "bool"}, {"void", "void"}, {"exception", "exception"}};

   public:
      std::string getCplusplusType(std::string _ns_type) const
      {
         auto it = type_mapper.find(_ns_type);
         if (it != type_mapper.end())
         {
            return it->second;
         }
         return _ns_type + "*";
      }
   };
   typedef struct _type
   {
      std::string alias;
      int id;
   } *_ptype;
   class typeManager
   {
      std::map<std::string, _type> types;
      int id = 0;

   private:
      void defaultEmit()
      {
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
      void emit(std::string alias)
      {
         _type _ = {};
         _.alias = alias;
         _.id = id++;
         types[alias] = _;
      }
      typeManager()
      {
         defaultEmit();
      }
      _type *find(std::string alias)
      {
         auto type = types.find(alias);
         if (type == types.end())
         {
            return nullptr;
         }
         return &(type->second);
      }
      //    _type get(std::string alias) const{
      //       return types.find(alias);
      //    }
   };
}