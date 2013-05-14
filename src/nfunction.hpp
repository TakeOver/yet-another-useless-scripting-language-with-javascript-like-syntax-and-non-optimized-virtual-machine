#pragma once
#include "ngcobject.hpp"
#include <ostream>
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
    union{
    	AbstractNativeFunction* abstract;
        void*ptr;
	uint32_t offset;
    };
    Function(void*ptr):is_native(true),is_abstract(false),ptr(ptr){}
    Function(uint32_t offset):is_native(false),is_abstract(false),offset(offset){}
    Function(AbstractNativeFunction* aptr):is_native(true),is_abstract(true), abstract(aptr){}
    ~Function(){}
    void print(std::ostream &out){
      out<<"\"function\"";
    }
    uint32_t getCall(){return offset;}
    void* getNativeCall(){return ptr;}
    void createCall(VirtualMachine*v,Value*s){
            abstract->call(v, s);
    }
  };
}