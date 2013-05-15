#pragma once
#include "nvm.hpp"
#include "memory"
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
        inline void MarshalVariadic(GC*,std::vector<Value>&){}
        template<typename T,typename ...Args> inline void MarshalVariadic(GC*gc,std::vector<Value>& tmp,T t, Args ... arg){
                tmp.push_back(MarshalType(gc,t));
                MarshalVariadic(gc,tmp, arg...);
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
                        std::vector<Value> args;
                        MarshalVariadic(vm->getGC(),args, vargs...);
                        Value res;
                        try{
                                res = vm->MakeCall(func, self,args);
                        }catch(...){}
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
        template<typename T1,typename ...T2> NativeFunction<T1, T2...>* defun(T1(*ptr)(T2...)){
                return new NativeFunction<T1, T2...>(ptr);
        }
        template<class C> class Userdata: public AbstractUserdata{
                C* clazz;
                std::unordered_map<std::string,Value> methods;
        public:
                virtual ~Userdata<C>(){
                                delete clazz;
                }
                virtual void SetMethods(std::unordered_map<std::string, Value> v){
                        methods = v;
                }
                Userdata<C>(){
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
                virtual void del(std::string what){
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
        template<class C,typename T,typename ...T1>struct NativeMethod: public AbstractNativeFunction{
                T (*ptr)(C*,T1...);
                virtual ~NativeMethod<C,T, T1...>(){}
                NativeMethod<C,T,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        vm->Push(MarshalType(vm->getGC(),ptr((ThisCast<C>(*self)), UserType<T1> (vm->GetArg()) ...)));
                }
        };
        template<class C,typename ...T1>  struct NativeMethod<C,void,T1...>: public AbstractNativeFunction{
                void (*ptr)(C*,T1...);
                virtual ~NativeMethod<C,void, T1...>(){}
                NativeMethod<C,void,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        ptr((ThisCast<C>(*self)), UserType<T1> (vm->GetArg()) ...);
                        vm->Push(Value());
                }
        };
        template<class C, typename T1,typename ...T2> NativeMethod<C,T1, T2...>* defmem(T1(*ptr)(C*,T2...)){
                return new NativeMethod<C,T1, T2...>((ptr));
        }
        //This two macros generate getter and setter for _PUBLIC_ONLY members of class using lambda
        #define defvarget(classname,varname) {"__get:"#varname,defmem((decltype(classname::varname)(*)(classname*))\
        [](classname*ptr)->decltype(classname::varname){\
                return ptr->varname;\
        })}

        #define defvarset(classname,varname) {"__set:"#varname,defmem((void(*)(classname*,decltype(classname::varname)))\
        [](classname*ptr, decltype(classname::varname) val){\
                ptr->varname = val;\
        })}
}