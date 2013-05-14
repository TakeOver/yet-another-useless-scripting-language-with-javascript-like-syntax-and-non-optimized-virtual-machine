#pragma once
#include "nvm.hpp"
namespace nls{
        template<typename T> T UserType(Value res){
                throw "Not Implemented";
        }
        template<> void UserType<void>( Value  val){}
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
        template<typename T,typename ...T1>
        struct NativeFunction: public AbstractNativeFunction{
                T (*ptr)(T1...);
                ~NativeFunction<T, T1...>(){}
                NativeFunction<T,T1...>(decltype(ptr) ptr):ptr(ptr){}
                virtual void call(VirtualMachine*vm,Value * self){
                        vm->Push(MarshalType(vm->getGC(),(*ptr)(UserType<T1> (vm->GetArg()) ...)));
                }
        };
        template<typename T1,typename ...T2> NativeFunction<T1, T2...>* defun(T1(*ptr)(T2...)){
                return new NativeFunction<T1, T2...>(ptr);
        }
}