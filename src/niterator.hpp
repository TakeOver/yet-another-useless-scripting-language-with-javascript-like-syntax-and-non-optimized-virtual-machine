#pragma once
#include <unordered_map>
#include "nvalue.hpp"
#include <map>
#include <string>
#include "nlogger.hpp"
namespace nls{
        class HTableIterator{
                using iter = std::unordered_map<std::string, Value>::iterator;
                iter end, current;
                public:
                        HTableIterator(){}
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
                        void create(Value val){
                                if( val.type!=Type::htable){
                                        this->current = iter(),
                                        this->end = this->current;
                                }else{
                                        this->Construct(val.t->table);
                                }
                        }
        };
        class ArrayIterator{
                using iter = std::map<uint64_t, Value>::iterator;
                iter end, current;
                public:
                        ArrayIterator(){}
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
                        static void create(ArrayIterator* it,Value val){
                                if( val.type!=Type::array){
                                        it->current = iter(),
                                        it->end = it->current;
                                }else{
                                        it->Construct(val.a->arr);
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