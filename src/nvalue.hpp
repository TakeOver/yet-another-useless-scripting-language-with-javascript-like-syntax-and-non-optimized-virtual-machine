#pragma once
#include "nstring.hpp"
#include "ntypes.hpp"
#include "narray.hpp"
#include "ntable.hpp"
#include "ngc.hpp"
#include "nfunction.hpp"
#include "nuserdata.hpp"
#include <iostream>
#include <stdint.h>
namespace nls{
        struct Value{
        public:
                Type::types type;
                union{
                        String * s;
                        Table<Value> * t;
                        Array<Value> * a;
                        Function * func;
                        uint64_t i;
                        long double f;
                        AbstractUserdata * u;
                };

                void print(std::ostream&out){
                        switch(type){
        	               case Type::str: s->print(out); break;
        	               case Type::fun_t: func->print(out);break;
        	               case Type::array: a->print(out);break;
        	               case Type::htable: t->print(out);break;
        	               case Type::number: out<<f; break;
        	               case Type::boolean: out<<(i?"true":"false"); break;
        	               case Type::null: out<<"null"; break;
                               case Type::userdata: out<< "\"Userdata\""; break;
                        }
                }

                Value():type(Type::null),i(0){}
                Value(uint64_t i,Type::types type):type(type),i(i){}
                Value(bool _i):type(Type::boolean),i(static_cast<uint64_t>(_i)){}
                Value(long double f):type(Type::number),f(f){}
                Value(std::string) = delete;
                Value(double _f):type(Type::number),f(static_cast<long double>(_f)){}
                Value(GC *gc,String * s):type(Type::str),s(s){
                        gc->push((GCObject*)s);
                }
                Value(GC *gc,AbstractUserdata* u):type(Type::userdata),u(u){
                        gc->push((GCObject*)u);
                }

                Value Clone(GC*gc){
                        if(type==Type::str){
        	               return Value(gc,s->Clone());
                        }
                        if(type==Type::htable)
        	               return Value(gc,t->Clone(gc));
                        if(type==Type::array)
        	               return Value(gc,a->Clone(gc));
                        if(type==Type::userdata)
                                return Value(gc,u->Clone(gc));
                        return Value(*this);
                }

                void markAll(GC*gc,VirtualMachine*vm){
                    	if(type==Type::str){
                      		gc->mark(s);
                    	}else if(type==Type::htable){
                    		if(gc->marked(t)==false){
                			gc->mark(t);
                			t->markAll(gc,vm);
                	        }
                    	}else if(type==Type::array){
                        	if(gc->marked(t)==false){
                			gc->mark(a);
                			a->markAll(gc,vm);
                	        }
                       }else if(type==Type::fun_t){
                    	   gc->mark(func);
                       }else if(type==Type::userdata){
                                u->MarkAll(gc,vm);
                       }
                }

                Value(GC *gc,Table<Value> * t):type(Type::htable),t(t){
                        gc->push((GCObject*)t);
                }
                Value(GC *gc,Array<Value> * a):type(Type::array),a(a){
                        gc->push((GCObject*)a);
                }
                Value(GC *gc,Function * func):type(Type::fun_t),func(func){
                        gc->push((GCObject*)func);
                }

                Value operator -()const{
                        if(this->type!=Type::number)
        	               return Value();
                        return Value(-f);
                }
                operator bool ()const{
                        return this->type>Type::boolean ||
                                 (type == Type::boolean && i != 0);
                }
        };

        inline const Value operator!(const Value & val){
                return Value(!bool(val),Type::boolean);
        }
        inline const Value operator+(const Value&l,const Value&r){
                if(l.type>Type::number || r.type>Type::number||!l.type || !r.type)
                        return Value(0,Type::null);
                if(l.type==Type::boolean && r.type==Type::boolean)
                        return Value(!!(l.i+r.i),Type::boolean);
                if(l.type==Type::boolean)
                        return Value(l.i+r.f);
                if(r.type==Type::boolean)
                        return Value(l.f+r.i);
                return Value(l.f+r.f);
        }
        inline const Value operator-(const Value &l,const Value &r){
                return l+(-r);
        }
        inline const Value operator*(const Value&l,const Value&r){
                if(l.type>Type::number || r.type>Type::number||!l.type || !r.type)
                        return Value(0,Type::null);
                if(l.type==Type::boolean && r.type==Type::boolean)
                        return Value((l.i*r.i),Type::boolean);
                if(l.type==Type::boolean)
                        return Value(l.i*r.f);
                if(r.type==Type::boolean)
                        return Value(l.f*r.i);
                return Value(l.f*r.f);
        }
        inline const Value operator/(const Value&l,const Value&r){
                if(l.type>Type::number || r.type>Type::number||!l.type || !r.type)
                        return Value(0,Type::null);
                if(l.type==Type::boolean && r.type==Type::boolean)
                        return Value(l.i/((long double)r.i));
                if(l.type==Type::boolean)
                        return Value(l.i/r.f);
                if(r.type==Type::boolean)
                        return Value(l.f/((long double)r.i));
                return Value(l.f/r.f);
        }
        inline const Value operator%(const Value&l,const Value&r){
                if(l.type>Type::number || r.type>Type::number||!l.type || !r.type)
                        return Value(0,Type::null);
                if(r.type==Type::boolean){
                        if(r.i == 0)
                                return Value(0.0l/0.0l);
                        return Value(0,Type::boolean);
                }
                if(l.type==Type::boolean)
                        return Value(r.f!=0?
                                ((long double)(l.i%((int64_t) r.f))):(0.0/0.0));
                return Value(r.f!=0?((long double)(((int64_t)l.f)%((int64_t)r.f)))
                        :(0.0/0.0));
        }

        inline const Value operator&&(const Value&l,const Value&r){
                if(!bool(l))
                        return l;
                return r;
        }


        inline const Value operator||(const Value&l,const Value&r){
                if(bool(l))
                        return l;
                return r;
        }

        inline const Value operator==(Value&l,Value&r){
                if(l.type==0 && r.type==0)
                        return Value(1,Type::boolean);
                if(l.type == 0 || r.type == 0)
                        return Value(0,Type::boolean);
                if(l.type>Type::str && l.type>Type::str)
                        return Value(l.t==r.t,Type::boolean);
                if(l.type>Type::str || l.type>Type::str)
                        return Value(0,Type::boolean);
                if(l.type==Type::str && r.type==Type::str)
                        return Value(!strcmp(l.s->str,r.s->str),Type::boolean);
                if(l.type == Type::number && r.type==Type::number)
                        return Value(l.f==r.f,Type::boolean);
                if(l.type==Type::boolean && r.type==Type::boolean)
                        return Value(l.i==r.i,Type::boolean);
                if(l.type==Type::userdata && !(l.type-r.type))
                        return Value(l.u==r.u);
                return Value(0,Type::boolean);
        }
        inline const Value operator!=(Value &l,Value &r){
                return !(r==l);
        }

        inline const Value operator<(Value&l,Value&r){
                if(l.type==Type::null && r.type==Type::null)
                        return Value(0,Type::boolean);
                if(l.type== Type::null || r.type == Type::null)
                        return Value(0,Type::boolean);
                if(l.type == r.type){
                        if(l.type==Type::str){
                                return Value(strcmp(l.s->str,r.s->str)<0,Type::boolean);
                        }
                        if(l.type==Type::number){
                                return Value(l.f<r.f,Type::boolean);
                        }
                        if(l.type==Type::boolean){
                                return Value(l.i<r.i,Type::boolean);
                        }
                        return Value(0,Type::boolean);
                }
                return Value(0,Type::boolean);
        }
        inline const Value operator>(Value &l,Value &r){
                return r<l;
        }
        inline const Value operator<=(Value &l,Value &r){
                return r>l || r==l;
        }

        inline const Value operator>=(Value &l,Value &r){
                return r<=l;
        }
        inline const Value operator++(Value&l){
                return Value(1.0l)+l;
        }
        inline const Value operator--(Value&l){
                return Value(-1.0l)+l;
        }
}