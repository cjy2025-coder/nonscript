#include<string>
#include<iostream>
#include<stdint.h>
#include<sstream>
std::string ns_to_string(const std::string& v){return v;}
std::string ns_to_string(int8_t v)  { return std::to_string(v); }
std::string ns_to_string(const char * v){return v; }
std::string ns_to_string(int16_t v) { return std::to_string(v); }
std::string ns_to_string(int32_t v) { return std::to_string(v); }
std::string ns_to_string(int64_t v) { return std::to_string(v); }
std::string ns_to_string(uint8_t v)  { return std::to_string(v); }
std::string ns_to_string(uint16_t v) { return std::to_string(v); }
std::string ns_to_string(uint32_t v) { return std::to_string(v); }
std::string ns_to_string(uint64_t v) { return std::to_string(v); }
std::string ns_to_string(float v)  { return std::to_string(v); }
std::string ns_to_string(double v) { return std::to_string(v); }
std::string ns_to_string(bool v) { return v ? "true" : "false"; }
template<typename Ret, typename... Args>
std::string ns_to_string(Ret(*func)(Args...)) {
   std::ostringstream oss;
   oss << "<function at 0x" << std::hex << (void*)func << ">";
   return oss.str(); }
template<typename T>
std::string ns_to_string(const T& obj) {return obj->__toString__();}
class Student {
private:
    signed char score;
protected:
    std::string name;
public:
void __construct__(){
}
public:
std::string __toString__(){
    return "Here is object info";
}
};
int main() {
Student* s = new Student();
}
