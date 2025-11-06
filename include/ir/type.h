#pragma once
//此处定义IR中操作数可能会出现的类型
namespace ns{
   enum class IRType{
       BOOLEAN,
       STRING,
       FUNCTION,
       INT8,
       INT16,
       INT32,
       INT64,
       FLOAT32,
       FLOAT64
   };
}