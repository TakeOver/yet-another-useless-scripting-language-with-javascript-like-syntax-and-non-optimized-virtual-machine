#pragma once
#include "ngcobject.hpp"
#include "stdint.h"
#include "string.h"
#include <string>
#include <ostream>
namespace nls{
  class String:public GCObject{
  public:
    char* str;
    uint32_t len;
    ~String(){
      delete []str;
    }
    String * Clone(){
      auto ptr = new char[strlen(str)+1];
      strcpy(ptr,str);
      return new String(ptr);
    }
    String(const char* _str):len(strlen(_str)){
        this->str = new char[len+1];
        strcpy(this->str,_str);
    }
    String(char* str):str(str),len(strlen(str)){}
    void print(std::ostream&out){
      out<<std::string(str);
    }
    String(const std::string&s):String(s.c_str()){ }
    String(std::string&s):String(s.c_str()){ }
  };
}