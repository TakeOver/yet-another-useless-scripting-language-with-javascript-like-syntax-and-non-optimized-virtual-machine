#pragma once
#include "ngcobject.hpp"
#include <ostream>
#include <functional>
namespace nls{
        class VirtualMachine;
        struct Value;
  struct AbstractNativeFunction{
        virtual ~AbstractNativeFunction(){}
        virtual void call(VirtualMachine*,Value*)=0;
  };
  class Function:public GCObject{
  public:
    bool is_native, is_abstract;
    AbstractNativeFunction* abstract;
    std::function<void(VirtualMachine*,Value*)>  stlfunc;
    uint32_t offset;
    Function(std::function<void(VirtualMachine*,Value*)> stlfunc):is_native(true),is_abstract(false),stlfunc(stlfunc){}
    Function(uint32_t offset):is_native(false),is_abstract(false),offset(offset){}
    Function(AbstractNativeFunction* abstract):is_native(true),is_abstract(true), abstract(abstract){}
    ~Function(){}
    void print(std::ostream &out){
      out<<"\"function\"";
    }
    uint32_t getCall(){return offset;}
    std::function<void(VirtualMachine*,Value*)> getNativeCall(){return stlfunc;}
    void createCall(VirtualMachine*v,Value*s){
            abstract->call(v, s);
    }
  };
}