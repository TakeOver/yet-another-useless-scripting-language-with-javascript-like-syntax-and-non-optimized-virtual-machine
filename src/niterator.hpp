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
                                        std::cerr<<"Missed\n";
                                        val.print(std::cerr);
                                }else{
                                        this->Construct(val.t->table);
                                }
                        }
                        std::string toString(){
                                return "{HTableIterator}";
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
                        Value next(){
                                return (current++)->second;
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
}