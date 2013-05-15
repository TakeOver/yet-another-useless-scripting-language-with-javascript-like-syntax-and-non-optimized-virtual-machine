#pragma once
#include "ngcobject.hpp"
#include <unordered_map>
#include <sstream>
#include <ostream>
#include "ngc.hpp"
#include "ntypes.hpp"
#include "nfunction.hpp"
#include <typeinfo>
#include <string>
namespace nls{
        class AbstractUserdata : public GCObject{
        public:
                virtual ~AbstractUserdata(){}
                virtual Value get(std::string what, VirtualMachine*) =0;
                virtual void set(std::string off,Value what, VirtualMachine*)=0;
                virtual void del(std::string off) = 0;
                virtual void MarkAll(GC*gc) = 0;
                virtual void SetMethods(std::unordered_map<std::string, Value>)=0;
        };
}