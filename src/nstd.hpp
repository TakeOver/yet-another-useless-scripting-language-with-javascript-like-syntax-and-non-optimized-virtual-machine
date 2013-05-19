#include <cmath>
#include "nvm.hpp"
#include "nparser.hpp"
#pragma once
namespace nls{

#define BINDALL(api) \
        do{ \
                api->bindFunction("__native__unsafe__evaluate",__eval,false);\
                api->bindFunction("__native__safe__json__parse",__json_parse,false);\
                api->bindFunction("__native__typeof",__native__typeof,false);\
                api->bindFunction("__init__safe__istream",__init_safe_file_istream,false);\
                api->bindFunction("__native__reflection_heap_size",reflection_heap_size,false);\
                api->bindFunction("__native__reflection_gc_collect",__reflection_gc_collect,false);\
                srand(time(nullptr));\
        }while(0)

        void reflection_heap_size(VirtualMachine*vm,Value*self){
              vm->Push(Value(static_cast<long double>(vm->getGC()->PtrCount())));
        }
        void __eval(VirtualMachine *vm,Value*){
           auto self = vm->GetArg();
            if(self.type !=Type::str){
                  vm->Push(Value());
                  return;
            }
            auto _what = self.s->str;
            Parser *par = nullptr;
            BasicBlock* bb = nullptr;
            VirtualMachine *_v = nullptr;
            RefHolder * rh = nullptr;
            try{
                    par = new Parser(new Lexer(std::string(_what)));
                    par->Parse();
                    rh = new RefHolder();
                    bb = new BasicBlock(rh);
                    par->getRoot()->emit(bb);
                    bb->ReplaceLabels();
                    bb->emit(IL::hlt);
                    auto reg = par->getRoot()->Registry;
                    _v = new VirtualMachine();
                    _v->keepSilent();
                    _v->SetBasicBlock(bb);
                    _v->run();
                    auto tmp = _v->getReg(reg);
                    vm->Push(tmp.Clone(vm->getGC()));
            }catch(...){
                    vm->Push(Value());
            }
            delete _v;
            delete bb;
            delete par;
            delete rh;
        }
        void __json_parse(VirtualMachine *vm,Value*){
            auto self = vm->GetArg();
            if(self.type !=Type::str){
                  vm->Push(Value());
                  return;
            }
            auto _what = self.s->str;
            Parser *par = nullptr;
            BasicBlock* bb = nullptr;
            VirtualMachine *_v = nullptr;
            RefHolder * rh = nullptr;
            try{
                    par = new Parser(new Lexer(std::string(_what)));
                    auto obj = par->ParseObj();
                    rh = new RefHolder();
                    bb = new BasicBlock(rh);
                    obj->emit(bb);
                    bb->ReplaceLabels();
                    bb->emit(IL::hlt);
                    auto reg = obj->Registry;
                    _v = new VirtualMachine();
                    _v->keepSilent();
                    _v->SetBasicBlock(bb);
                    _v->run();
                    auto tmp = _v->getReg(reg);
                    vm->Push(tmp.Clone(vm->getGC()));
            }catch(...){
                    vm->Push(Value());
            }
            delete _v;
            delete bb;
            delete rh;
            delete par;
        }
        void __native__typeof(VirtualMachine *vm,Value*){
              auto val = vm->GetArg();
              const char* ptr;
                  #define k case Type
              switch(val.type){
                    k::null: ptr =(const char*) "null"; break;
                    k::boolean: ptr = (const char*)"boolean"; break;
                    k::number: ptr = (const char*)"number"; break;
                    k::str: ptr =(const char*) "string"; break;
                    k::fun_t: ptr =(const char*) "function"; break;
                    k::array: ptr = (const char*)"array"; break;
                    k::htable: ptr = (const char*)"object"; break;
                    k::userdata: ptr = (const char*)"userdata"; break;
                    default: ptr = "Oops!";
            }
                  #undef k
            vm->Push(Value(vm->getGC(),new String(ptr)));
        }
        void __init_safe_file_istream(VirtualMachine *vm,Value* self){
            if(!self->type || self->type!=Type::htable){
                  vm->Push(Value());
                  return;
            }
            auto path = self->t->get("__path");
            if(path.type!=Type::str){
                    vm->Push(Value());
                    return;
            }
            auto arr = Value(vm->getGC(),new Array<Value>());
            self->t->set("__buf",arr);
            std::ifstream in (path.s->str);
            if(!in){
                    vm->Push(Value());
                    return;
            }
            uint i = 0;
            std::string s;
            while(!in.eof()){
                  std::getline(in,s);
                  if(s.length()<=0)
                        continue;
                  arr.a->set(i++,Value(vm->getGC(),new String((const char*)s.c_str())));
          }
          in.close();
          vm->Push(Value());
        }
        void __reflection_gc_collect(VirtualMachine *vm,Value*){
              vm->CallGCCollection();
              vm->Push(Value());
        }
}