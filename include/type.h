#pragma once
#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include<vector>
namespace ns
{
    // 访问级别定义
    enum class AccessLevel
    {
        Public,
        Private,
        Protected
    };
    // 成员类型分类
    enum class MemberType
    {
        Field,
        Method,
        Constructor,
        Static
    };
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
    typedef struct _FuncDetail
    {
        std::vector<_type *> param_types;
        _type *return_type;
        // 默认为false，[[noused]]
        bool is_external_func = false;
        // 默认为false, [[noused]]
        bool is_builtin_func = false;
        // 默认为false
        bool is_unlimited_args_func = false;
        // 访问级别（用于构造函数等访问控制）
        AccessLevel access = AccessLevel::Public;
    } FuncDetail;
    typedef struct _TypeInfo TypeInfo;
    typedef struct _ArrayDetail
    {
        std::vector<TypeInfo *> elems_types;
        int size;
    } ArrayDetail;
    typedef struct _MemDetail
    {
        TypeInfo *ti;
        AccessLevel level;
        MemberType type;
    } MemDetail;
    typedef struct _ClassDetail
    {
        // 表示父类
        std::vector<TypeInfo *> parents;
        // 类成员
        std::unordered_map<std::string, MemDetail> mems;
    } ClassDetail;
    typedef struct _TypeInfo
    {
        _type *baseType;
        std::variant<FuncDetail, ArrayDetail, ClassDetail> detail;
    } TypeInfo;
    extern TypeInfo *_errorType;

   class typeManager
   {
      static std::map<std::string,_type> tis_;
      static int id_ ;
   private:
      // 提交内置的类型
      static void defaultEmit()
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
      static void emit(std::string alias)
      {
         _type _ = {};
         _.alias = alias;
         _.id = id_++;
         tis_[alias] = _;
      }
      static void init(){
         defaultEmit();
      }
      static TypeInfo *find(std::string alias)
      {
         auto it = tis_.find(alias);
         if (it == tis_.end())
         {
            return nullptr;
         }
         TypeInfo * ti = new TypeInfo();
         ti->baseType = &(it->second);
         return ti;
      }
   };
}