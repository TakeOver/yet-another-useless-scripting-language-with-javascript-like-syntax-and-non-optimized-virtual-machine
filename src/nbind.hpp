#pragma once
#include "nvm.hpp"
#include "memory"
#include "nlogger.hpp"
namespace nls{
        template<typename T> T UserType(Value res){
                throw "Not Implemented";
        }
        template<> void UserType<void>( Value  val){}
        template<> Value UserType<Value>(Value val){
                return val;
        }
        template<> std::string UserType<std::string>( Value  val){
                if(val.type!=Type::str)
                        return std::string();
                return std::string(val.s->str);
        }
        template<> long double UserType<long double>(Value val){
                if(val.type!=Type::number)
                        return 0;
                return val.f;
        }
        template<> double UserType<double>(Value val){
                if(val.type!=Type::number)
                        return 0;
                return val.f;
        }
        template<> float UserType<float>(Value val){
                if(val.type!=Type::number)
                        return 0;
                return val.f;
        }
        template<> bool UserType<bool>(Value val){
                return bool(val);
        }
        template<> uint64_t UserType<uint64_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<uint64_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> uint32_t UserType<uint32_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<uint32_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> uint16_t UserType<uint16_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<uint16_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> uint8_t UserType<uint8_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<uint8_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> int64_t UserType<int64_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<int64_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> int32_t UserType<int32_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<int32_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> int16_t UserType<int16_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<int16_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> int8_t UserType<int8_t>(Value val){
                if(val.type==Type::number)
                        return static_cast<int8_t>(val.f);
                if(val.type==Type::boolean)
                        return val.i;
                return 0;
        }
        template<> const char* UserType<const char*>(Value  val){
                if(val.type==Type::str)
                        return val.s->str;
                return nullptr;
        }
        template<> Array<Value> * UserType< Array<Value>* >(Value  val){
                if(val.type==Type::array)
                        return val.a;
                return nullptr;
        }
        template<> Table<Value> * UserType< Table<Value>* >(Value  val){
                if(val.type==Type::htable)
                        return val.t;
                return nullptr;
        }
        template<> std::map<uint64_t, Value> UserType< std::map<uint64_t, Value> >(Value  val){
                if(val.type==Type::array)
                        return val.a->arr;
                return std::map<uint64_t, Value>();
        }
        template<> std::unordered_map<std::string, Value> UserType< std::unordered_map<std::string, Value> >(Value  val){
                if(val.type==Type::htable)
                        return val.t->table;
                return std::unordered_map<std::string, Value>();
        }
        template<> Function* UserType<Function*>(Value  val){
                if(val.type==Type::fun_t)
                        return val.func;
                return nullptr;
        }
        inline Value MarshalType(GC*gc,Value val){
                return val;
        }
        inline Value MarshalType(GC*gc,std::string val){
                return Value(gc,new String(val));
        }
        inline Value MarshalType(GC*gc,const char* val){
                return Value(gc,new String(val));
        }
        inline Value MarshalType(GC*gc,long double val){
                return Value(val);
        }
        inline Value MarshalType(GC*gc,double val){
                return Value(val);
        }
        inline Value MarshalType(GC*gc,float val){
                return Value(val);
        }
        inline Value MarshalType(GC*gc,bool val){
                return Value(val);
        }
        inline Value MarshalType(GC*gc,std::nullptr_t val){
                return Value();
        }
        inline Value MarshalType(GC*gc,uint64_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,uint32_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,uint16_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,uint8_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,int64_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,int32_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,int16_t val){
                return Value(static_cast<long double>(val));
        }
        inline Value MarshalType(GC*gc,int8_t val){
                return Value(static_cast<long double>(val));
        }
        template <typename T> inline Value MarshalType(GC*gc,std::vector<T> arr){
                auto array = new Array<Value>();
                uint64_t idx = 0;
                for(auto&x:arr)
                        array->set(idx,MarshalType(gc, x));
                return Value(gc,array);
        }
        template<typename RETVAL> struct ScriptFunction{
        private:
                VirtualMachine *vm;
                Value self;
                Function * func;
        public:
                ScriptFunction<RETVAL> (VirtualMachine*vm,Value self,Function*func):vm(vm),self(self){
                        this->func= new Function(*func);
                }
                ~ScriptFunction<RETVAL>(){
                        delete func;
                }
                RETVAL operator () (std::vector<Value> args = std::vector<Value>()){
                        Value res;
                        try{
                                res = vm->MakeCall(func, self,args);
                        }catch(...){}
                        return UserType<RETVAL>(res);
                }
                template<typename ...Args>
                RETVAL operator () (Args ... vargs){
                        Value res;
                        try{
                                res = vm->MakeCall(func, self,{MarshalType(vm->getGC(), vargs)...});
                        }catch(...){
                                NLogger::log("Failed to vm->call in ScriptFunction");
                        }
                        return UserType<RETVAL>(res);
                }
        };
        template<typename T,typename ...T1>struct NativeFunction: public AbstractNativeFunction{
                T (*ptr)(T1...);
                virtual ~NativeFunction<T, T1...>(){}
                NativeFunction<T,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        //fucking magic of c++11 variadic templates! god bless clang!
                        vm->Push(MarshalType(vm->getGC(),(*ptr)(UserType<T1> (vm->GetArg()) ...)));
                }
        };
        template<typename ...T1>  struct NativeFunction<void,T1...>: public AbstractNativeFunction{
                void (*ptr)(T1...);
                virtual ~NativeFunction<void, T1...>(){}
                NativeFunction<void,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        (*ptr)(UserType<T1> (vm->GetArg()) ...);
                        vm->Push(Value());
                }
        };
        template<typename T1,typename ...T2> NativeFunction<T1, T2...>* def(T1(*ptr)(T2...)){
                return new NativeFunction<T1, T2...>(ptr);
        }
        template<class C> class Userdata: public AbstractUserdata{
                C* clazz;
                std::unordered_map<std::string,Value> methods;
        public:
                virtual ~Userdata<C>(){
                                delete clazz;
                }
                Userdata<C>(decltype(methods) mem):methods(mem){
                        clazz = new C();
                }
                Userdata<C>(C*clazz, decltype(methods) &methods):clazz(clazz), methods(methods){}
                virtual void MarkAll(GC*gc){
                        for(auto&x:methods)
                                x.second.markAll (gc);
                }
                virtual Value get(std::string what, VirtualMachine*vm){
                        auto iter = methods.find(what);
                        if(iter==methods.end()){
                                iter = methods.find("__get:"+what);
                                if(iter==methods.end() || iter->second.type!=Type::fun_t){
                                        return Value();
                                }
                                Value self = Value(vm->getGC(),this);
                                return vm->call(iter->second.func,self);
                        }
                        return iter->second;
                }
                virtual void set(std::string what, Value whatval,VirtualMachine*vm){
                        auto iter = methods.find(what);
                        if(iter==methods.end()){
                                iter = methods.find("__set:"+what);
                                if(iter==methods.end() || iter->second.type!=Type::fun_t){
                                        return;
                                }
                                Value self = Value(vm->getGC(),this);
                                vm->call(iter->second.func,self,{ whatval});
                                return;
                        }
                        iter->second = whatval;
                }
                virtual void del(std::string what, VirtualMachine*vm){
                        auto __del = methods.find("__del:"+what);
                        if(__del->second.type==Type::fun_t){
                                vm->call(__del->second.func,Value(vm->getGC(),this));
                                return;
                        }
                        __del = methods.find("__del");
                        if(__del->second.type==Type::fun_t){
                                vm->call(__del->second.func,Value(vm->getGC(),this),{Value(vm->getGC(),new String(what))});
                                return;
                        }
                        auto iter = methods.find(what);
                        if(iter==methods.end()){
                                return;
                        }
                        methods.erase(iter);
                }
                virtual AbstractUserdata* Clone(GC*gc){
                        auto res = new Userdata<C>(new C(*clazz), methods);
                        gc->push(res);
                        return res;
                }
                C * getData(){
                        return clazz;
                }
        };
        template<class C> C* ThisCast(Value val){
                if(val.type!=Type::userdata)
                        throw nls::ApiError("Cannot cast Value to this. val.type!=Type::userdata");
                auto self = dynamic_cast<Userdata<C>*>(val.u);
                if(!self)
                        throw nls::ApiError("Types mismatch. Failed to cast val.u to Userdata<C>*");
                return self->getData();
        }
        template<class C,typename T,typename ...T1>struct NativeMemFunction: public AbstractNativeFunction{
                T (*ptr)(C*,T1...);
                virtual ~NativeMemFunction<C,T, T1...>(){}
                NativeMemFunction<C,T,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        vm->Push(MarshalType(vm->getGC(),ptr((ThisCast<C>(*self)), UserType<T1> (vm->GetArg()) ...)));
                }
        };
        template<class C,typename ...T1>  struct NativeMemFunction<C,void,T1...>: public AbstractNativeFunction{
                void (*ptr)(C*,T1...);
                virtual ~NativeMemFunction<C,void, T1...>(){}
                NativeMemFunction<C,void,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        ptr((ThisCast<C>(*self)), UserType<T1> (vm->GetArg()) ...);
                        vm->Push(Value());
                }
        };
        template<class C,typename T,typename ...T1>struct NativeMethod: public AbstractNativeFunction{
        typedef T (C::*method)(T1...);
                method ptr;
                virtual ~NativeMethod<C,T, T1...>(){}
                NativeMethod<C,T,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        vm->Push(MarshalType(vm->getGC(),((*ThisCast<C>(*self)).*ptr)(UserType<T1> (vm->GetArg()) ...)));
                }
        };
        template<class C,typename ...T1>  struct NativeMethod<C,void,T1...>: public AbstractNativeFunction{
        typedef void (C::*method)(T1...);
                method ptr;
                virtual ~NativeMethod<C,void, T1...>(){}
                NativeMethod<C,void,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        ((*ThisCast<C>(*self)).*ptr)(UserType<T1> (vm->GetArg()) ...);
                        vm->Push(Value());
                }
        };
        template<class C>struct NativeBinary: public AbstractNativeFunction{
        typedef C& (C::*method)(C&);
                method ptr;
                virtual ~NativeBinary<C>(){}
                NativeBinary<C>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        if(self->type!=Type::userdata){
                                vm->Push(Value());
                                return;
                        }
                        Value res;
                        ((*ThisCast<C>(res=Value(vm->getGC(),self->u->Clone(vm->getGC())))).*ptr)(*ThisCast<C> (vm->GetArg()));
                        vm->Push(res);
                }
        };
        template<class C>struct NativeUnary: public AbstractNativeFunction{
        typedef C& (C::*method)();
                method ptr;
                virtual ~NativeUnary<C>(){}
                NativeUnary<C>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        if(self->type!=Type::userdata){
                                vm->Push(Value());
                                return;
                        }
                        (*ThisCast<C>(*self).*ptr)();
                        vm->Push(*self);
                }
        };
        template<class C> NativeBinary<C>* def(C&(C::*ptr)(C&)){
                return new NativeBinary<C>((ptr));
        }
        template<class C> NativeUnary<C>* def(C&(C::*ptr)()){
                return new NativeUnary<C>((ptr));
        }
        template<class C, typename T1,typename ...T2> NativeMethod<C,T1, T2...>* def(T1(C::*ptr)(T2...)){
                return new NativeMethod<C,T1, T2...>((ptr));
        }
        template<class C, typename T1,typename ...T2> NativeMemFunction<C,T1, T2...>* def(T1(*ptr)(C*,T2...)){
                return new NativeMemFunction<C,T1, T2...>((ptr));
        }
        //This macroses generate getter and setter for _PUBLIC_ONLY members of class using lambda
        #define bindfieldget(classname,varname) {"__get:"#varname,def((decltype(classname::varname)(*)(classname*))\
                [](classname*ptr)->decltype(classname::varname){\
                        return ptr->varname;\
                })}

        #define bindfieldset(classname,varname) {"__set:"#varname,def((void(*)(classname*,decltype(classname::varname)))\
                [](classname*ptr, decltype(classname::varname) val){\
                        ptr->varname = val;\
                })}
        #define field(classname,varname) bindfieldset(classname,varname),bindfieldget(classname,varname)
        #define constfield(classname,varname) bindfieldget(classname,varname)

        #define bindfielddelexception(classname,varname){\
                "__del:"#varname, def((void(*)(classname*))\
                                [](classname*p){\
                                       throw ApiError("Trying to delete immutable field "#classname"::"#varname);\
                                }\
                        )\
        }
        #define bindfieldsetexception(classname,varname){\
                "__set:"#varname, def((void(*)(classname*,decltype(classname::varname)))\
                                [](classname*p,decltype(classname::varname)){\
                                        throw ApiError("Trying to assign to immutable field "#classname"::"#varname);\
                                }\
                        )\
        }
        #define bindfielddelnonstrict(classname,varname)\
                {"__del:"#varname, def((void(*)(classname*)) [](classname*p){})}
        #define immutableFieldStrict(classname,varname) bindfieldget(classname,varname), bindfielddelexception(classname,varname),\
                bindfieldsetexception(classname,varname)
        #define immutableField(classname,varname) bindfielddelnonstrict(classname,varname),bindfieldget(classname,varname)
}