#pragma once
#include "ngcobject.hpp"
#include <map>
#include <sstream>
#include "ntypes.hpp"
#include "ngc.hpp"
#include <ostream>
namespace nls{
  template<typename T> class Array: public GCObject{
  public:
    std::map<uint64_t,T> arr;
    virtual ~Array(){
      arr.clear();
    }
    Array<T>(){}
    Array<T>(decltype(arr)&na):arr(na){}
    Array<T>* Clone(GC*gc){
      decltype(arr) na;
      for(auto&x:arr)
	if(x.second.type==Type::array && x.second.a==this)
	  continue;
	else
	  na[x.first]=x.second.Clone(gc);
      return new Array<T>(na);
    }
    T get(uint64_t off){
      auto iter = arr.find(off);
      if(iter == arr.end())
	return T();
      return iter->second;
    }
    void set(uint64_t off,T what){
      arr[off]=what;
    }
    void markAll(GC*gc){
      for(auto&x:arr)
	x.second.markAll(gc);
    }
    void print(std::ostream& out){
      if(arr.size()==0){
	out<<"[]";
	return;
      }
      std::stringstream ss; ss<<"[";
      auto to = arr.rbegin()->first;
      for(auto i =0;i<=to;++i){
	auto x=arr.find(i);
	if(x==arr.end()){
	  ss<<"null,";
	  continue;
	}

	if(x->second.type==Type::array && x->second.a==this)
	  continue;
	if(x->second.type==Type::str)
	  ss<<"\"";
	x->second.print(ss);
	if(x->second.type==Type::str)
	  ss<<"\"";
	ss<<",";
      }
      out<<ss.str().substr(0,ss.str().length()-1)<<"]";
    }
  };
}