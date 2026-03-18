#pragma once
#include "builtin.h"
#include<map>
#define ENV_OUTER 0
#define ENV_STORE 1
namespace ns
{
    class Enviroment
    {
    private:
        std::map<std::string, std::shared_ptr<Object>> stores;
        std::shared_ptr<Enviroment> outer;

    public:
        int type = ENV_STORE;

    public:
        Enviroment() {}
        Enviroment(std::shared_ptr<Enviroment> outer_) : outer(outer_) {}
    public:
        std::shared_ptr<Object> get(std::string name)
        {
            auto it = stores.find(name);
            if (it != stores.end())
            {
                type = ENV_STORE;
                return it->second;
            }
            if (outer == NULL)
                return NULL;
            type = ENV_OUTER;
            return outer->get(name);
        }
        std::shared_ptr<Object> getInside(std::string name)
        {
            auto it = stores.find(name);
            if (it != stores.end())
            {
                type = ENV_STORE;
                return it->second;
            }
            type = ENV_OUTER;
            return NULL;
        }
        std::map<std::string, std::shared_ptr<Object>> getStores() const{
            return stores;
        }
        void set(std::string name, std::shared_ptr<Object> item)
        {
            stores[name] = item;
        }
        void setOuter(std::string name, std::shared_ptr<Object> item)
        {
            outer->set(name, item);
        }
        int getSize() const
        {
            return stores.size();
        }
        void setOuter(std::shared_ptr<Enviroment> outer_)
        {
            outer = outer_;
        }
        void merge(std::shared_ptr<Enviroment> env){
            auto  stores_=env->getStores();
            for(auto it=stores_.begin();it!=stores_.end();it++){
                stores[it->first]=it->second;
            }
        }
    };
}