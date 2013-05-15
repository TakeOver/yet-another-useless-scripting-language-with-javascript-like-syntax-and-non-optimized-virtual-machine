#include <cmath>
#pragma once
#define BINDALL \
do{ api->bindFunction("__native__substr",str_substr);\
api->bindFunction("__native__unsafe__evaluate",__eval);\
api->bindFunction("__native__safe__json__parse",__json_parse);\
api->bindFunction("__native__next",__iterator_next);\
api->bindFunction("__native__typeof",__native__typeof);\
api->bindFunction("__init__safe__istream",__init_safe_file_istream);\
api->bindFunction("__native__write__to__file",__write__to__file);\
api->bindFunction("__native__write__stdout",__writeline);\
api->bindFunction("__native__reflection_heap_size",reflection_heap_size);\
api->bindFunction("__native__reflection_gc_collect",__reflection_gc_collect);\
srand(time(nullptr));\
api->bindFunction("__native__read__stdin",__readline);} while(0)

void reflection_heap_size(nls::VirtualMachine*vm,nls::Value*self){
  vm->Push(nls::Value(static_cast<long double>(vm->getGC()->PtrCount())));
}
void str_substr(nls::VirtualMachine*vm,nls::Value* self){
  if(self->type!=nls::Type::str){
    vm->Push(nls::Value());
    return;
  }
  auto &str = *self;
  if(str.type != nls::Type::str){
    vm->Push(nls::Value());
    return;
  }
  auto from = vm->GetArg();
  if(from.type!=nls::Type::number){
    vm->Push(nls::Value());
    return;
  }
  auto size = vm->GetArg();
  if(size.type!=nls::Type::number){
    vm->Push(nls::Value());
    return;
  }
  if(from.f>=str.s->len){
    vm->Push(nls::Value());
    return;
  }
  if(size.f ==0){
    vm->Push(nls::Value(vm->getGC(),new nls::String("\0")));
    return;
  }
  if(size.f<0){
    vm->Push(nls::Value());
    return;
  }
  if(size.f+from.f>str.s->len){
    vm->Push(nls::Value());
    return;
  }
  char * _str = new char[((uint)size.f)+1];
  strncpy(_str,str.s->str+static_cast<uint>(from.f),(uint)size.f);
  _str[(uint)size.f]='\0';
  vm->Push(nls::Value(vm->getGC(),new nls::String(_str)));
}
void __eval(nls::VirtualMachine *vm,nls::Value*){
  auto self = vm->GetArg();
  if(self.type !=nls::Type::str){
    vm->Push(nls::Value());
    return;
  }
  auto _what = self.s->str;
  nls::Parser *par = nullptr;
  nls::BasicBlock* bb = nullptr;
  nls::VirtualMachine *_v = nullptr;
  nls::RefHolder * rh = nullptr;
  try{
    par = new nls::Parser(new nls::Lexer(std::string(_what)));
    par->Parse();
    rh = new nls::RefHolder();
    bb = new nls::BasicBlock(rh);
    par->getRoot()->emit(bb);
    bb->ReplaceLabels();
    bb->emit(nls::IL::hlt);
    auto reg = par->getRoot()->Registry;
    _v = new nls::VirtualMachine();
    _v->keepSilent();
    _v->SetBasicBlock(bb);
    _v->run();
    auto tmp = _v->getReg(reg);
    vm->Push(tmp.Clone(vm->getGC()));
  }catch(...){
    vm->Push(nls::Value());
  }
  delete _v;
  delete bb;
  delete par;
  delete rh;
}
void __json_parse(nls::VirtualMachine *vm,nls::Value*){
  auto self = vm->GetArg();
  if(self.type !=nls::Type::str){
    vm->Push(nls::Value());
    return;
  }
  auto _what = self.s->str;
  nls::Parser *par = nullptr;
  nls::BasicBlock* bb = nullptr;
  nls::VirtualMachine *_v = nullptr;
  nls::RefHolder * rh = nullptr;
  try{
    par = new nls::Parser(new nls::Lexer(std::string(_what)));
    auto obj = par->ParseObj();
    rh = new nls::RefHolder();
    bb = new nls::BasicBlock(rh);
    obj->emit(bb);
    bb->ReplaceLabels();
    bb->emit(nls::IL::hlt);
    auto reg = obj->Registry;
    _v = new nls::VirtualMachine();
    _v->keepSilent();
    _v->SetBasicBlock(bb);
    _v->run();
    auto tmp = _v->getReg(reg);
    vm->Push(tmp.Clone(vm->getGC()));
  }catch(...){
    vm->Push(nls::Value());
  }
  delete _v;
  delete bb;
  delete rh;
  delete par;
}
void __iterator_next(nls::VirtualMachine *vm,nls::Value*){
  auto obj = vm->GetArg();
  auto pos = vm->GetArg();
  if(obj.type!=nls::Type::htable){
    vm->Push(nls::Value());
    return;
  }
  if(pos.type!=nls::Type::number){
    vm->Push(nls::Value());
    return;
  }
  if(pos.f>=obj.t->table.size()){
    vm->Push(nls::Value());
    return;
  }
  auto _it = obj.t->table.begin();
  for(uint i=0;i<(uint)pos.f;++i) ++_it;
  auto res = nls::Value(vm->getGC(),new nls::String((const char*)_it->first.c_str()));
  vm->Push(res);
}

void __native__typeof(nls::VirtualMachine *vm,nls::Value*){
  auto val = vm->GetArg();
  const char* ptr;
  #define k case nls::Type
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
  vm->Push(nls::Value(vm->getGC(),new nls::String(ptr)));
}
void __init_safe_file_istream(nls::VirtualMachine *vm,nls::Value* self){
  using namespace nls;
  if(!self->type || self->type!=Type::htable){
    vm->Push(nls::Value());
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
void __write__to__file(nls::VirtualMachine *vm,nls::Value*val){
  using namespace nls;
  auto self = vm->GetArg();
  if(self.type!=Type::str){
    vm->Push(Value());
    return;
  }
  std::ofstream out (self.s->str);
  if(!out){
    vm->Push(Value());
    return;
  }
  std::stringstream wat;
  auto _Wat = vm->GetArg();
  _Wat.print(wat);
  out<<wat.str();
  out.close();
  vm->Push(Value(1,Type::boolean));
}
void __readline(nls::VirtualMachine *vm,nls::Value*){
  std::string s;
  getline(std::cin,s);
  vm->Push(nls::Value(vm->getGC(),new nls::String((const char*)s.c_str())));
}
void __writeline(nls::VirtualMachine *vm,nls::Value*){
  auto wat = vm->GetArg();
  wat.print(std::cout);
  vm->Push(nls::Value());
}

void __reflection_gc_collect(nls::VirtualMachine *vm,nls::Value*){
  vm->CallGCCollection();
  vm->Push(nls::Value());
}