#pragma once
#include <map>
#include "ngcobject.hpp"
#include <cstdint>
namespace nls{
  class GC{
  private:
    std::map<GCObject*,bool> pool;
  public:
    void mark(GCObject*ptr){
      pool[ptr]=true;
    }
    void push(GCObject*ptr){
      pool[ptr]=false;
    }
    uint32_t PtrCount(){
      return pool.size();
    }
    void Collect(){
      for(auto i = pool.begin(),e=pool.end();i!=e;){
	if(i->second){
	  i->second=false;
	  ++i;
	}else{
	  delete i->first;
	  pool.erase(i++);
	}
      }
    }
    bool marked(GCObject*ptr){
      return pool[ptr];
    }
  };
}