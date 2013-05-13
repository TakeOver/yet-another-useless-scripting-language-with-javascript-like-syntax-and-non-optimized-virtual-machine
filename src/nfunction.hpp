#pragma once
#include "ngcobject.hpp"
#include <ostream>
namespace nls{
  class Function:public GCObject{
  public:
    bool is_native;
    union{
    	void*ptr;
	    uint32_t offset;
    };
    Function(void*ptr):is_native(true),ptr(ptr){}
    Function(uint32_t offset):is_native(false),offset(offset){}
    ~Function(){}
    void print(std::ostream &out){
      out<<"\"function\"";
    }
    uint32_t getCall(){return offset;}
    void* getNativeCall(){return ptr;}
  };
}