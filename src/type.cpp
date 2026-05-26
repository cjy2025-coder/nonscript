#include "type.h"
namespace ns{
    std::map<std::string, TypeInfo> typeManager::tis_;
    int typeManager::id_ = 0;
}