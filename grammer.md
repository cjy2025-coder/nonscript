# 基础数据类型
数字类型：
包括整数和浮点数这两大类：
整数：int8,int16,int32,int64;
浮点数：float32,float64;
串：
在NoScript中不进行字符和串的区分，所有的字符和串通过"、'，`的封闭语句来进行申明
布尔值：
true/false
数组：
可以包含不同的数据类型，由这些数据所组成的有序集合，最简单的数组是空数组 []
# 函数申明
使用关键字 func 来申明函数，例如：
func f(){}
# 变量申明
使用关键字 var 来申明变量，例如：
var a=1,b;
其中，可以为变量添加类型修饰，例如：
var a:int8=1;
对于没申明类型的变量，默认类型为 any
# 常量申明
使用关键字 const 来申明常量，例如：
const a:string = "hello world!";
# 类申明
使用关键字 class 来申明类，例如：
class Student extends Person{
    private var name;
    public getName(){
        return name;
    }
    public constructor(name_){
        name=name_;
    }
}
其中，Person类为Student类的父类，通过extends关键字来指定父类；
类的成员包括属性和函数，使用private、public,protected进行修饰，机制类似C++；
使用constructor来指定构造函数。
# 类的实例化
使用 new 关键字创建对应类实例，即对象，例如：
var student=new Student();
