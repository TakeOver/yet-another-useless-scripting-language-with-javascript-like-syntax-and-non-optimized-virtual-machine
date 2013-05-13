#pragma once
#include "ngcobject.hpp"
#include <unordered_map>
#include <sstream>
#include <ostream>
#include "ngc.hpp"
#include "ntypes.hpp"
#include <string>
namespace nls{
  template<typename T> class Table: public GCObject{
  public:
    std::unordered_map<std::string,T> table;
    ~Table(){
      table.clear();
    }
    Table<T>(){}
    Table<T>(decltype(table) &nt):table(nt){}
    Table<T>* Clone(GC*gc){
      decltype(table) nt;
      for(auto&x:table){
	if(x.second.type==Type::htable && x.second.t==this)
	  continue;
	else
	  nt[x.first]=x.second.Clone(gc);
      }
      return new Table<T>(nt);
    }
    bool exist(std::string off){
        if(table.find(off)==table.end()){
                auto prot = table.find("prototype");
                if(prot == table.end())
                        return false;
                if(prot->second.type==Type::htable && prot->second.t==this)
                        return false;
                if(prot->second.type!=Type::htable)
                        return false;
                return prot->second.t->exist(off);
        }
        return true;
    }
    T get(std::string off){
      auto iter = table.find(off);
      if(iter == table.end()){
	iter = table.find("prototype");
	if(iter == table.end())
	  return T();
	if(iter->second.type!=Type::htable)
	  return T();
	if(iter->second.t==this)
	  return T();
	return iter->second.t->get(off);
      }
      return iter->second;
    }
    void markAll(GC*gc){
      for(auto&x:table)
	x.second.markAll(gc);
    }
    bool set(std::string off,T what, bool first = true){
        #if RECURSIVE_SET
        auto iter = table.find(off);
        if(iter == table.end()){
                auto pro = table.find("prototype");
                if(pro == table.end() || pro->second.type!=Type::htable || pro->second.t==this){
                        if(first){
                                table[off]=what;
                                return true;
                        }else
                                return false;
                }else {
                        if(pro->second.t->set(off,what,false))
                                return true;
                        if(first){
                                table[off]=what;
                                return true;
                        }
                        return false;
                }
        }
        iter->second=what;
        return true;
        #else
        table[off]=what;
        return true;
        #endif
    }
    void print(std::ostream& out){
      if(table.size()==0){
	out<<"{}";
	return;
      }
      std::stringstream ss; ss<<"{";
      for(auto&x:table){
	if(x.first=="prototype"){
                continue;
        }
	if(x.second.type==Type::fun_t)
	  continue;
	if(x.second.type==Type::htable && x.second.t==this)
	  continue;
	ss<<'\"'<<x.first<<"\":";
	if(x.second.type==Type::str)ss<<"\"";
	x.second.print(ss);
	if(x.second.type==Type::str)ss<<"\"";
	ss<<",";
      }
      std::string _res = ss.str();
      if(_res.length()>0 && _res.back()==',')
	_res.pop_back();
      out<<_res<<"}";
    }
  };
}