#pragma once
#include <unordered_map>
#include "nvalue.hpp"
#include <map>
#include <string>
#include "nlogger.hpp"
namespace nls{
        class HTableIterator{
                using iter = std::unordered_map<std::string, Value>::iterator;
                iter current,end;
                public:
                        HTableIterator(){}
                        HTableIterator(iter current, iter end):current(current),end(end){}
                        void Construct(std::unordered_map<std::string, Value>& obj){
                                end = obj.end();
                                current = obj.begin();
                        }
                        ~HTableIterator(){}
                        std::pair<std::string, Value> next(){
                                std::cerr<<"called next "<<valid()<<'\n';
                                return *(current++);
                        }
                        bool valid(){
                                return current != end;
                        }
                        static HTableIterator* create(Value val){
                                if( val.type!=Type::htable){
                                        return new HTableIterator(iter(), iter());
                                }else{
                                        return new HTableIterator(val.t->table.begin(),val.t->table.end());
                                }
                        }
        };
        class ArrayIterator{
                using iter = std::map<uint64_t, Value>::iterator;
                iter current,end;
                public:
                        ArrayIterator(){}
                        ArrayIterator(iter current,iter end):current(current),end(end){}
                        void Construct(std::map<uint64_t, Value>& arr){
                                end = arr.end();
                                current = arr.begin();
                        }
                        ~ArrayIterator(){}
                        std::pair<uint64_t, Value> next(){
                                return *(current++);
                        }
                        bool valid(){
                                return current != end;
                        }
                        static ArrayIterator* create(Value val){
                                if( val.type!=Type::array){
                                        return new ArrayIterator(iter(), iter());
                                }else{
                                        return new ArrayIterator(val.a->arr.begin(),val.a->arr.end());
                                }
                        }
        };
        Value MarshalType(GC*gc,std::pair<std::string, Value> it){
                auto obj = new Table<Value>();
                obj->set("key",Value(gc,new String(it.first)));
                obj->set("value", it.second);
                return Value(gc,obj);
        }
        Value MarshalType(GC*gc,std::pair<uint64_t, Value> it){
                auto obj = new Table<Value>();
                obj->set( "key" , Value( static_cast < long double > ( it.first ) ) );
                obj->set("value", it.second);
                return Value(gc,obj);
        }
}