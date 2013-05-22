#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include <map>
#include <functional>

#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nexc.hpp"
#include "ngc.hpp"
#include "nvalue.hpp"
#include "ncodegen.hpp"
#include "nlogger.hpp"

#ifndef GC_CYCLO
#define GC_CYCLO 1000
#endif

namespace nls{

  class VirtualMachine{
  public:
        typedef std::function<void(VirtualMachine*,Value*)> native_func_t;
  private:
#define RD R[bc[pc].dest]
#define RS1 R[bc[pc].src1]
#define RS2 R[bc[pc].src2]
        Value * R = nullptr; //! @p registers
        uint registers;
        std::vector<Value> S; //! @p Stack-for-spilling-and-func-args
        std::vector<uint> Args; //! @p Stack-for-number-of-args
        std::vector<uint> Addr; //! @p Stack-for-ret-addres
        std::vector<uint> Unpacked;
        pcode* bc = nullptr;
        uint32_t pc = 0;
        std::vector<uint> tryAddr;
        uint exitcode = 0;
        bool silent = false;
        uint64_t bcsize;
        std::unordered_map<std::string, uint16_t> meta_variables_info;

#ifdef DEBUG_INFO
        std::vector<std::string> __funcs;
#endif

#ifdef EXPERIMENTAL_GC_ROOT
        std::vector<uint16_t> gcroots;
        uint16_t rootsCount;
#endif

        std::map<uint8_t,Table<Value>*> basic_prototypes;

        GC* gc = nullptr;

        inline Table<Value>* _getPrototypeOf(uint8_t _t){
                return basic_prototypes[_t];
        }

    inline void tostr(Value&val, std::stringstream& ss){
        if(val.type == Type::htable){
	       auto _f = val.t->get("__tostr");
	       if(_f.type == Type::fun_t){
                        call(_f.func, val).print(ss);
                        return;
	        }
        }else if(val.type==Type::userdata){
                auto _fc = val.u->get("__tostr",this);
                if(_fc.type==Type::fun_t){
                        call(_fc.func,val).print(ss);
                        return;
                }
        }
        val.print(ss);
    }

    inline void createArgsArray(){
        auto count = Args.back();
        auto reg = bc[pc].dest;
        auto off = S.size()-1;
        R[reg]= Value(gc,new Array<Value>());
        for (int i = 0; i < count; ++i){
                R[reg].a->set(i, S[off-i]);
        }

    }

    inline void packArg(){
        Array<Value>* arr = new Array<Value>();
        uint idx = 0;
        while(Args.back()--){
                arr->set(idx++,S.back());
                S.pop_back();
        }
        RD = Value(gc,arr);
    }

    inline void unpackArg(){
        if(RD.type != Type::array){
                S.push_back(RD);
                Unpacked.push_back(1);
                return;
        }
        auto idx = RD.a->arr.size();
        while(idx--){
                S.push_back(RD.a->get(idx));
        }
        Unpacked.push_back(RD.a->arr.size());
    }

    inline void delProp(){
        if(RD.type!=Type::htable && RD.type!=Type::array && RD.type!=Type::userdata)
	       return;
        std::stringstream s;
        if(RD.type==Type::userdata){
                tostr(RS1,s);
                RD.u->del(s.str(), this);
                return;
        }
        if(RD.type==Type::htable){
	       tostr(RS1,s);

                auto __del = RD.t->get("__del:"+s.str());

                if(__del.type==Type::fun_t){
                        call(__del.func,RD);
                        return;
                }

                __del = RD.t->get("__del");

                if(__del.type==Type::fun_t){
                        call(__del.func,RD,{Value(gc,new String(s.str()))});
                        return;
                }

        	auto iter = RD.t->table.find(s.str());
        	if(iter == RD.t->table.end())
        	       return;

        	RD.t->table.erase(iter);

        }else{
        	if(RS1.type!=Type::number){
        	        tostr(RS1,s);
                	auto iter = _getPrototypeOf(Type::array)->table.find(s.str());
                	if(iter == _getPrototypeOf(Type::array)->table.end())
        	                return;
        	        _getPrototypeOf(Type::array)->table.erase(iter);
        	}else{
        	        auto iter = RD.a->arr.find((uint64_t)RS1.f);
        	        if(RD.a->arr.end()==iter)
        	               return;
        	        RD.a->arr.erase(iter);
        	}
        }

    }

    inline void StopTheWorld(){

#ifdef EXPERIMENTAL_GC_ROOT

        for (int i = 0; i < rootsCount; ++i){
                R[gcroots[i]].markAll(gc,this);
        }

#else

        for (int i = 0; i <= registers; ++i){
                R[i].markAll(gc,this);
        }

#endif

        for(auto&x:S)
	       x.markAll(gc,this);
	for(auto&x:basic_prototypes){
	        if(gc->marked(x.second))
	               continue;
	        gc->mark(x.second);
	        x.second->markAll(gc,this);
	}
	gc->Collect();
    }

    inline void mov(){
        if(RD.type==Type::htable){
                auto prop = RD.t->get("__assign");
                if(prop.type==Type::fun_t){
                        call(prop.func, RD,{RS1});
                        return;
                }
        }else if(RD.type==Type::userdata){
                auto prop = RD.u->get("__assign",this);
                if(prop.type==Type::fun_t){
                        call(prop.func, RD,{RS1});
                        return;
                }
        }
        RD=RS1;
        if(RS1.type==Type::str)
	       RD.s=RD.s->Clone();
    }

    inline void objAlloc(){
        assert(bc[pc].dest<=registers);
        RD=Value(gc,new Table<Value>());
        RD.t->set("prototype",Value(gc,_getPrototypeOf(Type::htable)));
    }

    inline void arrAlloc(){
        RD=Value(gc,new Array<Value>());
    }

    inline void hlt(){
        exitcode = 1; //
    }

    inline void getArg(){
        if(Args.empty() || Args.back()==0){
	       RD=Value(0,Type::null);
        } else {
	       pop();
	       Args.back()-=1;
        }
    }

    inline void length(){
        if(RS1.type== Type::htable){
                auto _fc = RS1.t->get("__len");
                if(_fc.type==Type::fun_t){
                        RD = call(_fc.func,RS1);
                        return;
                }
	        RD=Value(RS1.t->table.size()+.0);
        }else if (RS1.type==Type::str){
	        RD=Value(RS1.s->len+.0);
        }else if(RS1.type==Type::array){
	        if(RS1.a->arr.size()==0){
	               RD=Value(0.0);
                }else{
	               RD=Value(RS1.a->arr.rbegin()->first+1.0);
               }
        }else if(RS1.type==Type::userdata){
                auto _fc = RS1.u->get("__len",this);
                if(_fc.type==Type::fun_t){
                        RD = call(_fc.func,RS1);
                        return;
                }
        }else{
                RD=Value(1.0);
        }
    }

    inline void clearArgs(){
        if(Args.empty())
                return;
        if(S.size()!=0){
                for(uint i=0;i<Args.back() && S.size();++i){
	               S.pop_back();
                }
        }
        Args.pop_back();
    }

    inline void dup(){
        S.push_back(S.back());
    }

    inline void pushNewObj(){
        S.push_back(Value(gc,new Table<Value>()));
    }

    inline void push(){
        assert(bc[pc].dest<=registers);
        S.push_back(RD);
    }

    inline void pop(){
        assert(bc[pc].dest<=registers);
        assert(!S.empty());
        RD=S.back();
        S.pop_back();
    }

    inline void top(){
        assert(bc[pc].dest<=registers);
        assert(!S.empty());
        RD=S.back();
    }


    inline void print(){
        std::stringstream ss;
        tostr(RD,ss);
        std::cout<<ss.str()<<std::endl;
    }

    inline void ret(){
        S.push_back(RD);
        if(Addr.size()){
                pc = Addr.back();
                Addr.pop_back();
        }else{
                NLogger::log("VirtualMachine::ret():\t Addr.size()==0");
                pc = bcsize-2;
        }
        RS2.type = Type::null;

#ifdef DEBUG_INFO

        while(!__funcs.empty()&&__funcs.size()>=Addr.size())
        __funcs.pop_back();

#endif

    }

    inline void nativeCall(Function*func){
        auto self = S.back();
        S.pop_back();
        --Args.back();
        if(func->is_abstract){
                func->createCall(this, &self);
                return;
        }
        native_func_t callee = (native_func_t)func->getNativeCall();
        callee(this,&self);
    }

    inline void call(){

#ifdef DEBUG_INFO
        if(RS2.type==Type::str)
                __funcs.push_back(RS2.s->str);
        else
                __funcs.push_back("???");
#endif

        Args.push_back(bc[pc].src1);
        while(!Unpacked.empty()){
                if(!Unpacked.back()){
                        --Args.back();
                }else{
                        Args.back()+=(Unpacked.back()-1);
                }
                Unpacked.pop_back();
        }
        if(RD.type == Type::fun_t){
               Value self = S.back();
               S.pop_back();
               Args.back()--;
               RD.func->createCall(this, &self);
#ifdef DEBUG_INFO
               if(RD.type ==Type::fun_t && RD.func->is_native && __funcs.size()){
                        __funcs.pop_back();
               }
#endif
               return;
        }
        if(RD.type == Type::htable){
	        auto prop = RD.t->get("__call");
	        if(prop.type!=Type::fun_t){
	               clearArgs();
	               S.push_back(Value());
#ifdef DEBUG_INFO
                       __funcs.pop_back();
#endif
	               return;
	        }
	       S.pop_back();
               Args.back()--;
               prop.func->createCall(this, &RD);
#ifdef DEBUG_INFO
               if(prop.type ==Type::fun_t && prop.func->is_native && __funcs.size()){
                        __funcs.pop_back();
               }
#endif
               return;
        }else if(RD.type==Type::userdata){
                auto __call = RD.u->get("__call",this);
                if(__call.type==Type::fun_t){
                        S.pop_back();
                        --Args.back();
                        __call.func->createCall(this, & RD);
#ifdef DEBUG_INFO
                        if(__call.type== Type::fun_t && __call.func->is_native && __funcs.size())
                                __funcs.pop_back();
#endif
                        return;
                }
        }
        clearArgs();
        S.push_back(Value());
#ifdef DEBUG_INFO
        __funcs.pop_back();
#endif
    }

    inline void createFunc(){
        RD=Value(gc,new Function(this,bc[pc].src1));
    }

    std::map<uint8_t, std::string> overload_operators = {
        {IL::Math::add,"__add"},
        {IL::Math::mul,"__mul"},
        {IL::Math::sub,"__sub"},
        {IL::Math::div,"__div"},
        {IL::Math::mod,"__mod"},
        {IL::Math::log_or,"__or"},
        {IL::Math::log_and,"__and"},
        {IL::Math::log_not,"__not"},
        {IL::Math::neg,"__neg"},
        {IL::Math::inc,"__inc"},
        {IL::Math::dec,"__dec"},
        {IL::Math::log_le,"__less_eq"},
        {IL::Math::log_eq,"__equal"},
        {IL::Math::log_ne,"__nonequal"},
        {IL::Math::log_ge,"__great_eq"},
        {IL::Math::log_gt,"__great"},
        {IL::Math::log_lt,"__less"}
    };
    constexpr inline static bool is_unary_op(uint8_t q ){
        using namespace IL::Math;
        return q==inc || q==dec || q==neg || q==log_not;
    }
    inline void math_op(){
        using namespace IL::Math;
        if(RS1.type==Type::htable){
                auto op  = overload_operators[bc[pc].subop];
                auto object_operator = RS1.t->get(op);
                if(object_operator.type==Type::fun_t){
                        if(!is_unary_op(bc[pc].subop))
                                RD = call(object_operator.func, RS1, {RS1,RS2});
                        else
                                RD = call(object_operator.func,RS1,{RS1});
                        return;
                }
        }else if(RS1.type==Type::userdata){
                auto op  = overload_operators[bc[pc].subop];
                        auto object_operator = RS1.u->get(op,this);
                if(object_operator.type==Type::fun_t){
                        if(!is_unary_op(bc[pc].subop))
                                RD = call(object_operator.func, RS1, {RS2});
                        else
                                RD = call(object_operator.func,RS1);
                        return;
                }
        }
        switch (bc[pc].subop){
        	case add: RD = RS1+RS2; break;
        	case sub: RD = RS1-(RS2); break;
        	case mul: RD = RS1*RS2; break;
        	case div: RD = RS1/RS2; break;
        	case mod: RD = RS1%RS2; break;
        	case log_and: RD = RS1&&RS2; break;
        	case log_or: RD = RS1||RS2; break;
        	case log_eq: RD = RS1==RS2; break;
        	case log_ne: RD = RS1!=RS2; break;
        	case log_lt: RD = RS1<RS2; break;
        	case log_le: RD = RS1<=RS2; break;
        	case log_gt: RD = RS1>RS2; break;
        	case log_ge: RD = RS1>=RS2; break;
        	case log_not: RD = !(RS1); break;
        	case neg: RD = -(RS1); break;
        	case inc: RD = Value(1.0)+RD; break;
        	case dec: RD = Value(-1.0)+RD; break;
        }
    }

    inline void concat(){
        std::stringstream ss;
        tostr(RS1,ss);
        tostr(RS2,ss);
        RD=Value(gc,new String(ss.str().c_str()));
    }

    inline void setProperty(){
        if(RD.type == Type::array && RS1.type==Type::number){
	       RD.a->set(((uint64_t)RS1.f),RS2);
	       return;
        }
        std::stringstream ss;
        tostr(RS1,ss);
        if(RD.type!=Type::htable){
                if(RD.type==Type::userdata){
                        RD.u->set(ss.str(),RS2,this);
                        return;
                }
	        auto prot = _getPrototypeOf(RD.type);
	        if(!prot)
	                return;
	       prot->set(ss.str(),RS2);
        }else{
                if(RD.t->exist(ss.str())==false){
                        auto setter = RD.t->get("__set:"+ss.str());
                        if(setter.type==Type::fun_t){
                                call(setter.func, RD,{RS2});
                                return;
                        }
                        setter = RD.t->get("__set");
                        if(setter.type==Type::fun_t){
                                call(setter.func,RD,{Value(gc,new String(ss.str())),RS2});
                                return;
                        }
                }
	        RD.t->set(ss.str(),RS2);
        }
    }

    inline void getProperty(){
        if(RS1.type == Type::array && RS2.type==Type::number){
	        RD= RS1.a->get((uint64_t)RS2.f);
	        return;
        }
        std::stringstream ss;
        tostr(RS2,ss);
        if(RS1.type!=Type::htable){
                if(RS1.type==Type::userdata){
                        RD = RS1.u->get(ss.str(),this);
                        return;
                }
	        auto prot = _getPrototypeOf(RS1.type);
	        if(!prot)
	                RD.type=Type::null;
	        RD=prot->get(ss.str());
        }else{
                if(RS1.t->exist(ss.str())==false){
                        auto getter = RS1.t->get("__get:"+ss.str());
                        if(getter.type!=Type::fun_t){
                                getter = RS1.t->get("__get");
                                if(getter.type!=Type::fun_t){
                                        RD = Value();
                                        return;
                                }
                                RD = call(getter.func, RS1,{Value(gc,new String(ss.str()))});
                                return;
                        }
                        RD = call(getter.func, RS1);
                        return;
                }
	       RD=RS1.t->get(ss.str());
        }
    }

    inline void jcc(){
        if(bc[pc].subop==IL::Jump::jmp){
	        pc = bc[pc].offset;
	        return;
        }
        bool res = RD.type==Type::null
                        ||(RD.type==Type::boolean && RD.i==0)
                        || (RD.type==Type::number && RD.f==0.0);
        if(bc[pc].subop!=IL::Jump::jz)
                res=!res;
        pc=res?bc[pc].offset:pc;
    }

    inline void set_try(){
        tryAddr.push_back(bc[pc].offset);
    }

    Value StackTrace(){

#ifdef DEBUG_INFO

        Array<Value>* st = new Array<Value>();
        for(uint i=0;i<__funcs.size();++i){
                st->set(i,Value(gc,new String(__funcs[i])));
        }
        return Value(gc,st);

#else

        return Value();

#endif

    }

    inline void throw_ex(){
        auto &nexception=RD;
        if(tryAddr.empty()){
                if(silent){
                        exitcode = 2;
                        return;
                }
                auto handle =getUserData()->get("OnException");
                if(handle.type==Type::fun_t && nexception.type==Type::htable && bool(nexception.t->get("isUserdata"))){
                        auto ud = Value(gc,getUserData());
                        if(call(handle.func,ud,{nexception,StackTrace()})){
                                return;
                        }
                        exitcode = 2;
                        return;
                }else if((handle=R[1].t->get("OnException")).type==Type::fun_t){
                        if(call(handle.func,R[1],{nexception, StackTrace()})){
                                return;
                        }
                        exitcode = 2;
                        return;
                }else{
	                std::cerr<<"Unhandled exception:\n";
                        std::stringstream ss;
                        tostr(nexception,ss);
                        std::cerr<<ss.str();
                        std::cerr<<"\n";
                }
                std::stringstream ss;
                ss<<("Unhandled exception\n");
                tostr(nexception,ss);
                exitcode = 2;
	        if(Addr.empty()==false)
	               ss<<"\nInstruction address stack trace:\n";
        	for(auto x = Addr.rbegin(),e=Addr.rend();x!=e;++x){
	               ss<<"callee:"<<*x;
#ifdef DEBUG_INFO
                        if(__funcs.empty()){
#endif
                                ss<<"\tin\t[???]\n";
#ifdef DEBUG_INFO
                        }else{
                                ss<<"\tin\t["<<__funcs.back()<<"]\n";
	                        __funcs.pop_back();
                        }
#endif
                }
	       ss<<"Instruction:\n[pc:"<<pc<<"]\t[op:"
        	<<bc[pc].op<<"]\t[subop:"<<(uint)bc[pc].subop<<"]\tr"<<bc[pc].dest
	       <<"\tr"<<bc[pc].src1<<"\tr"<<bc[pc].src2<<'\n';
                ss<<"Value StackTrace:\n";
                for(int i = S.size()-1;i>=0;--i){
                        ss<<"["<<i<<"]=";
                        S[i].print(ss);
                        ss<<'\n';
                }
                ss<<"[end of value stack trace]\n";
        	ss<<"Excecution aborted\n";
                NLogger::log(ss.str());
                Addr.clear();
                Addr.push_back(bcsize-2);
                S.clear();
	       return;
        }
        pc = tryAddr.back();
        tryAddr.pop_back();
        S.push_back(nexception);
    }

        inline void pop_try(){
                assert(tryAddr.empty()==false);
                tryAddr.pop_back();
        }

        bool using_mapping = false;
        const char* fdata;
        struct stat finfo;
        int fd;

        void VirtualDealloc(){
                munmap((void*)fdata, finfo.st_size);
                close(fd);
        }
        void VirtualAlloc(const char *path){
                int fd = open(path, O_RDONLY);
                if (fd<0) {
	               throw vm_error("Cannot open assembly.\n");
                }
                fstat(fd, &finfo);
                fdata = (const char*)mmap(0, finfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (!fdata) {
	               throw vm_error("Cannot load assembly.\n");
                }
                initBitCode(fdata);
                using_mapping = true;
        }

        void initBitCode(const char* fdata){
                char* _pt = (char*)fdata;
                registers = *(uint16_t*)_pt;
                _pt+=sizeof(uint16_t);
                bcsize = *(uint64_t*)_pt;
                _pt+=sizeof(uint64_t);
                uint rootssize =*(uint16_t*)_pt;
#ifdef EXPERIMENTAL_GC_ROOT
                rootsCount = rootssize;
#endif
                _pt+=sizeof(uint16_t);
#ifdef EXPERIMENTAL_GC_ROOT
                for (int i = 0; i < rootsCount; ++i)
                {
                        gcroots.push_back(*reinterpret_cast<uint16_t*>(_pt));
                        _pt+=sizeof(uint16_t);
                }
#else
                _pt += sizeof(uint16_t)*rootssize;
#endif
                auto metasize = *(uint32_t*)_pt;
                _pt+=sizeof(uint32_t);
                std::vector<uint16_t> meta;
                for(uint i = 0;i<metasize;++i)
                {
                        uint16_t reg = ((uint16_t*)_pt)[i];
                        meta.push_back(reg);
                }
                _pt+=sizeof(uint16_t)*metasize;
                InitRuntime();
                loadAssemblyConst(_pt);
                for(uint i=0;i<(metasize);i+=2)
                {
                        auto reg = meta[i];
                        auto strreg = meta[i+1];
                        if(R[strreg].type!=Type::str)
                        {
                                NLogger::log("Incorrect type of register["+std::to_string(strreg)+"], expected string type");
                                continue;
                        }
                        std::string key (R[strreg].s->str);

                        meta_variables_info[key] = reg;
                }
                pcode * _code = ( pcode * ) _pt;
                bc = _code;
                Addr.push_back(bcsize-2);
        }

        void InitRuntime(){

                _is_inited = true;
                R = new Value[registers+1]();
                Args.push_back(0);
                if(!is_module){
                        gc = new GC();
                }
                R[1]=Value(gc,new Table<Value>());
                basic_prototypes={
        	       {Type::null,new Table<Value>()},
                       {Type::boolean,new Table<Value>()},
                       {Type::str,new Table<Value>()},
        	       {Type::number,new Table<Value>()},
        	       {Type::fun_t,new Table<Value>()},
        	       {Type::array,new Table<Value>()},
        	       {Type::htable,new Table<Value>()}
                };
                for(auto&x:basic_prototypes){
        	       x.second->set("prototype",Value(gc,x.second));
                }
                R[1].t->set("Object",Value(gc,_getPrototypeOf(Type::htable)));
                R[1].t->set("Number",Value(gc,_getPrototypeOf(Type::number)));
                R[1].t->set("String",Value(gc,_getPrototypeOf(Type::str)));
                R[1].t->set("Boolean",Value(gc,_getPrototypeOf(Type::boolean)));
                R[1].t->set("Function",Value(gc,_getPrototypeOf(Type::fun_t)));
                R[1].t->set("Null",Value(gc,_getPrototypeOf(Type::null)));
                R[1].t->set("Array",Value(gc,_getPrototypeOf(Type::array)));
                R[1].t->set("Userdata", Value(gc,new Table<Value>()));
#ifdef DEBUG_INFO
                __funcs.push_back("__start__");
#endif
        }

        bool _is_inited =false;
        bool is_module = false;
        void loadAssemblyConst(char*&ptr){
                constpcode * x;
                while(true){
	               x = (constpcode*)ptr;
	                if(x->op!=IL::loadConstcc)
	                        return;
	                switch(x->subop){
	                       case 3:  {
                                                R[x->dest]=Value(gc,new String((const char*)(ptr+sizeof(constpcode))));
                                                ptr+=strlen(((char*)(x+1)))+1;
                                        }break;

	                       case 0:  R[x->dest]=Value(0,Type::null);   break;
	                       case 1:  R[x->dest]=Value(x->i,Type::boolean);   break;
	                       case 2:  R[x->dest]=Value(x->f); break;
	               }
	               ptr+=sizeof(constpcode);
                }
        }

        void loadConst(std::vector<constpcode>pool){
                if(registers==0 && pool.size()>0)
	               throw vm_error("runtime not initialized, registers == 0");
                for(auto&x:pool){
	               switch(x.subop){
	                       case 3:  R[x.dest]=Value(gc,new String((const char*)x.c));   break;
	                       case 0:  R[x.dest]=Value(0,Type::null);   break;
	                       case 1:  R[x.dest]=Value(x.i,Type::boolean);   break;
	                       case 2:  R[x.dest]=Value(x.f);  break;
	                       default: assert(false);
	               }
                }
        }
        void SetAssembly(std::vector<pcode>code){
                bc = new pcode[code.size()+1]();
                memcpy(bc,&code[0],code.size()*sizeof(code[0]));
                Addr.push_back(bcsize=(code.size()-2));
        }
public:
        VirtualMachine ( GC*gc){
                // creating fork; module;
                this->gc = gc;
                registers = 0;
        }
        inline Value call(Function*func,Value self,std::vector<Value> args = std::vector<Value>()){
                auto __pc = pc;
                auto res = MakeCall(func, self,args);
                if(exitcode==1){
                        exitcode = 0;
                        pc=__pc;
                }
                return res;
        }

        inline Value callWithReplaceArgs(Function*func,Value&self){
                auto __pc = pc;
                Value res;
                do{
                        Args.back()++;
                        S.push_back(self);
                        if(func->is_native){
                                nativeCall(func);
                                auto tmp = S.back();
                                S.pop_back();
                                res = tmp;
                                break;
                        }
                        Addr.push_back(bcsize);
                        pc = func->getCall()+1;
                        run();
                        res = S.back();
                        S.pop_back();
                        Args.push_back(0);
                }while(0);
                if(exitcode==1){
                        exitcode = 0;
                        pc=__pc;
                }
                return res;
        }
        inline GC* getGC(){
                return gc;
        }

        Value getReg(uint reg){
                if(reg>registers)
	               throw vm_error("Register non exist");
                return R[reg];

        }
        inline Value GetArg(){
                if(Args.back()==0)
	               return Value();
                --Args.back();
                auto res = S.back();
                S.pop_back();
                return res;
        }

        inline void CallGCCollection(){StopTheWorld();}

        inline void Push(Value val){
                S.push_back(val);

        }

        inline void setSysFunction(std::string name, native_func_t addr){
                assert(R[1].type == Type::htable);
                R[1].t->set(name,Value(gc,new Function(this,addr)));
        }

        inline void setSysFunction(std::string name,AbstractNativeFunction* addr){
                assert(R[1].type == Type::htable);
                R[1].t->set(name,Value(gc,new Function(this,addr)));
        }
        inline void SetToSystem(std::string name,Value val){
                R[1].t->set(name,val);
        }
        VirtualMachine():registers(0){}

        ~VirtualMachine(){
                if(!_is_inited)
	               return;
                registers=0;
                delete []R;
                gc->Collect();
                if(using_mapping){
                        VirtualDealloc();
                }else{
                        delete []bc;
                }
                if(!is_module){
                        delete gc;
                }
        }
        bool isInitialized(){
                return _is_inited;
        }
        void keepSilent(){
                silent = true;
        }

        void SetBasicBlock(BasicBlock *bb){
                registers = bb->getRegCount()+1;
#ifdef EXPERIMENTAL_GC_ROOT
                rootsCount = bb->getRootsSize();
                gcroots = bb->makeRootArray();
#endif
                bcsize = bb->getCode().size();
                InitRuntime();
                loadConst(bb->getCPool());
                SetAssembly(bb->getCode());
                meta_variables_info.insert(bb->getMetaVarInfo().begin(),bb->getMetaVarInfo().end());
                Addr.push_back(bcsize-2);
        }
        void LoadAssembly(const char*path){
                VirtualAlloc(path);
        }
        void Release(){
	       delete this;
        }

        Value GetVariable(std::string key){
                auto iter = meta_variables_info.find(key);
                if(iter == meta_variables_info.end())
                        return Value();
                return R[iter->second];
        }

        void SetVariable(std::string key,Value val){
                auto iter = meta_variables_info.find(key);
                if(iter == meta_variables_info.end())
                        return;
                R[iter->second] = val;
        }

        Value ForceCall(Function* func, VirtualMachine * owner, Value * self){
                std::vector<Value> args;
                while(owner->Args.back())
                        args.push_back(owner->GetArg());
                return call(func,self,args);
        }

        Table<Value>* getUserData(){
                return R[1].t->get("Userdata").t;
        }
        inline Value MakeCall(Function*func,Value & self,std::vector<Value> args = std::vector<Value>()){
                Args.push_back(1+args.size());

#ifdef DEBUG_INFO
                __funcs.push_back("VirtualMachine::MakeCall");
#endif
                for (std::vector<Value>::reverse_iterator i = args.rbegin(); i != args.rend(); ++i)
                {
                        S.push_back(*i);
                }
                S.push_back(self);
                if(func->is_native){
                        if(func->is_abstract){
                                auto self = S.back();
                                S.pop_back();
                                Args.back()--;
                                func->createCall(this, &self);
                                self = S.back();
                                S.pop_back();
                                return self;
                        }
                        nativeCall(func);
                        auto tmp = S.back();
                        S.pop_back();
                        clearArgs();
#ifdef DEBUG_INFO
                        __funcs.pop_back();
#endif
                        return tmp;
                }
                Addr.push_back(bcsize);
                pc = func->getCall()+1;
                run();
                auto res = S.back();
                S.pop_back();
                return res;
        }
        void run(){
                uint16_t iterations = 0;
                uint16_t nextCollect=GC_CYCLO;
                exitcode= 0;
                while(!exitcode){
        	        iterations=(iterations+1)%nextCollect;
        	        if(!iterations){
        	               StopTheWorld();
        	               ++nextCollect;
        	        }
                	switch(bc[pc].op){
                                case IL::hlt:hlt();break;
                                case IL::nop:break;
                        	case IL::pushNewObj:pushNewObj();break;
                        	case IL::pop:pop();break;
                        	case IL::top:top();break;
                        	case IL::clearArgs:clearArgs();break;
                        	case IL::pushArg:push();break;
                        	case IL::objAlloc:objAlloc();break;
                        	case IL::print:print();break;
                        	case IL::ret:ret();break;
                        	case IL::getArg:getArg();break;
                        	case IL::mov:mov();break;
                        	case IL::call:call();break;
                        	case IL::math_op:math_op();break;
                        	case IL::concat:concat();break;
                        	case IL::setProperty:setProperty();break;
                        	case IL::getProperty:getProperty();break;
                        	case IL::jcc:jcc();break;
                        	case IL::createFunc:createFunc();break;
                        	case IL::loadConstcc:break;
                        	case IL::set_try: set_try();break;
                        	case IL::throw_ex: throw_ex(); break;
                        	case IL::length: length(); break;
                        	case IL::arrAlloc: arrAlloc();break;
                        	case IL::del: delProp(); break;
                                case IL::createArgsArray: createArgsArray();break;
                                case IL::pop_try:pop_try(); break;
                                case IL::unpackArg: unpackArg(); break;
                                case IL::packArg: packArg(); break;
                        	default: {
                        	       std::cerr<<"Incorrect instruction:\n"<<pc<<":\t"<<bc[pc].op<<"\t"<<(uint)bc[pc].subop<<"\t"<<
                        	       bc[pc].dest<<"\t"<<bc[pc].src1<<"\t"<<bc[pc].src2<<'\n';
                        	       throw vm_error("Oooooops!\n");

                        	}
                	}
        	       ++pc;
                }
        }
        void SetCall(Function*func,Value*self){
                S.push_back(*self);
                Args.back()++;
                Addr.push_back(pc);
                pc = func->getCall();
        }
#undef RD
#undef RS1
#undef RS2
  };
        void __VMForceCall(VirtualMachine* funcowner,Function* func,VirtualMachine* owner,Value* self){
                owner->Push(funcowner->ForceCall(func, owner, self));
        }
        void __VMCall(Function* func,VirtualMachine* owner,Value* self){
                owner->SetCall(func,self);
        }

}
