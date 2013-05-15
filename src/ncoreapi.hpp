#pragma once

#include "nparser.hpp"
#include "nvm.hpp"
#include "nstd.hpp"
#include "nbind.hpp"
#include "nregex.hpp"
#include <string>
#include <vector>
#include <iostream>

namespace nls{
  class NlsApi{
  private:
    BasicBlock *bb;
    VirtualMachine * vm;
    RefHolder * rh;
    uint16_t rootreg = 0;
    std::vector<std::pair<std::string,std::string> > definitions;
    std::vector<std::pair<std::string,std::string> >aliases;
    std::vector<AbstractNativeFunction*> native_binds;
  public:

    NlsApi(){
      rh = new RefHolder();
      bb = new BasicBlock(rh);
      vm = new VirtualMachine();
    }
    ~NlsApi(){
      delete bb;
      delete vm;
      delete rh;
      for(auto&x:native_binds)
        delete x;
    }
    template <typename T1,typename ...T2>void NativeBind(std::string name,T1(*ptr)(T2...)){
        NativeFunction<T1, T2...> *_F = defun(ptr);
        native_binds.push_back(_F); //locking ptr;
        vm->setSysFunction(name, _F);
    }
    template <class C> void bindClass(std::string clazz,std::unordered_map<std::string,AbstractNativeFunction*> _mem){
        std::unordered_map<std::string, Value> mem;
        for(auto&x:_mem){
                mem[x.first] = Value(vm->getGC(),new Function(x.second));
                native_binds.push_back(x.second);
        }
        auto constr = [clazz,mem,this](VirtualMachine*vm,Value*self){
                try{
                        self->type = Type::userdata;
                        self->u = new Userdata<C>();
                        self->u->SetMethods(mem);
                        auto init = self->u->get("construct",vm);
                        if(init.type==Type::fun_t){
                                vm->callWithReplaceArgs(init.func,*self);
                        }
                }catch(std::string msg){
                        this->RaiseException(msg);
                }
                vm->Push(*self);
        };
        bindFunction(clazz,constr);
    }
    void RaiseException(std::string msg){
        try{
                getFunction<void>("RaiseException")(msg);
        }catch(...){}
    }
    Value createString(const char*str){
        return Value(getGC(),new String(str));
    }
    Value createString(std::string str){
        return createString(str.c_str());
    }
    template<typename T> ScriptFunction<T> getFunction(std::string obj,std::string method){
        Value val = getUserData()->get(obj);
        if(val.type!=Type::htable)
                throw nls::ApiError("Object not found");
        Value func = val.t->get(method);
        if(func.type!=Type::fun_t)
                throw nls::ApiError("Method undefined");
        return ScriptFunction<T>(vm,val,func.func);
    }
    template<typename T> ScriptFunction<T> getFunction(std::string method){
        Value func = getUserData()->get(method);
        if(func.type!=Type::fun_t)
                throw nls::ApiError("Method undefined");
        return ScriptFunction<T>(vm,Value(),func.func);
    }
    void setToUD(std::string name,Value _what){
        getUserData()->set(name,_what);
    }
    Value call(Value func,Value self,std::vector<Value>args = std::vector<Value>())const{
        if(func.type!=Type::fun_t && func.type!=Type::htable)
                throw nls::ApiError("Function undefined");
        if(func.type==Type::htable){
                return call(func.t->get("__call"),func,args);
        }
        return vm->MakeCall(func.func, self, args);
    }
    inline GC*getGC()const{
        return vm->getGC();
    }
    inline void define(std::string x,std::string y){
        definitions.push_back(std::make_pair(x, y));
    }
    inline void alias(std::string x,std::string y){
        aliases.push_back(std::make_pair(x, y));
    }
    inline void cleardefs(){
        definitions.clear();
    }
    inline void clearaliases(){
        aliases.clear();
    }
    inline void cleanall(){
        this->cleardefs();
        this->clearaliases();
    }
    inline static Value Eval(std::string s, GC*gc){
        NlsApi* api = new NlsApi();
        api->CompileText(s,"NlsApi::Eval",/*save root reg*/true);
        api->PreprocessBitCode();
        api->InitVM();
        api->Execute();
        auto reg = api->rootreg;
        Value res;
        if(!reg)
                res= Value();
        else{
                res = api->vm->getReg(reg).Clone(gc);
                if(res.type==Type::fun_t)
                        res= Value();
        }
        delete api;
        return res;
    }
    Value call(std::string _class,std::string _method,std::vector<Value>args = std::vector<Value>()){
        Table<Value>* ud = getUserData();
        Value _uclass = ud->get(_class);
        if(_uclass.type!=Type::htable)
                return Value();
        Value _umethod = _uclass.t->get(_method);
        if(_umethod.type!=Type::fun_t)
                return Value();
        return call(_umethod,_uclass,args);
    }
    inline Table<Value>*getUserData(){
        if(vm->isInitialized()==false)
                throw nls::ApiError("VM is uninitiallized");
        return vm->getUserData();
    }
    inline void CompileText(std::string & code,std::string path="",bool saverootreg = false){
        Parser * par = new Parser(new Lexer(code,path),path);
        for(auto&x:definitions)
                par->define(x.first, x.second);
        for(auto&x:aliases)
                par->alias(x.first, x.second);
        par->Parse();
        par->getRoot()->emit(bb);
        if(saverootreg)
                rootreg =  par->getRoot()->Registry;
        delete par;
    }

    inline void ResetBitCode(){
      delete bb;
      delete rh;
      rh = new RefHolder();
      bb = new BasicBlock(rh);
    }

    inline void PreprocessBitCode(){
      bb->ReplaceLabels();
      bb->emit(IL::hlt);
    }

    inline void bindFunction(std::string name, VirtualMachine::native_func_t _f){
      vm->setSysFunction(name,_f);
    }
    inline void ResetVM(){
      delete vm;
      vm = new VirtualMachine();
    }

    inline void CompileFile(std::string  path){
      std::ifstream in (path);
      std::string res( (  std::istreambuf_iterator<char>( in ) ),
			  std::istreambuf_iterator<char>());
      in.close();
      CompileText(res,path);
    }
    inline void saveCode(int filedesc){
      if (write(filedesc,&(bb->getCode()[0]),bb->getCode().size()*sizeof(pcode))
	!=static_cast<int>(bb->getCode().size()*sizeof(pcode))) {
	throw ApiError("Cannot save binary: SaveAssembly");
	}
    }
    inline void saveCPool(int filedesc){
      for(auto&x:bb->getCPool()){
	write(filedesc,&x,sizeof(constpcode));
	if(x.subop==IL::ConstTypes::string){
	  write(filedesc,x.c,sizeof(char)*strlen(x.c)+sizeof(char));
	}
      }
    }
    inline void SaveAssembly(std::string out){
      int filedesc = open(out.c_str(),O_WRONLY | O_CREAT | O_TRUNC,
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

      if (filedesc < 0) {
	throw ApiError("Cannot open file: SaveAssembly");
      }
      uint16_t _tmp;
      uint64_t size = bb->getCode().size();
      write(filedesc,&(_tmp=bb->getRegCount()),sizeof(uint16_t));
      write(filedesc,&(size),sizeof(uint64_t));
      std::vector<uint16_t> roots (bb->makeRootArray());
      write(filedesc,&(_tmp=roots.size()),sizeof(uint16_t));
      write(filedesc,roots.data(),roots.size()*sizeof(uint16_t));
      saveCPool(filedesc);
      saveCode(filedesc);
      close(filedesc);
    }
    inline void LoadLibMath(){
        NativeBind("rand", std::rand);
        NativeBind("acos", acosl);
        NativeBind("__native__cos", cosl);
        NativeBind("__native__sin", sinl);
        NativeBind("__native__tg", tanl);
        NativeBind("__native__ctg",(long double (*)(long double))
                [](long double f)->long double{
                        return 1.0/tanl(f);
                }
        );
        NativeBind("__native__log", logl);
        NativeBind("__native__pow", powl);
        NativeBind("__native__sqrt",sqrtl);
        NativeBind("__native__lg",log10);
        NativeBind("__native__lb",log2l);
        NativeBind("__native__lgamma",lgammal);
        NativeBind("__native__round", roundl);
        NativeBind("__native__ceil",ceill);
        NativeBind("__native__atan", atanl);
        NativeBind("__native__asin", asinl);
        LoadLibComplex();
    }
    inline void LoadLibRegex(){
        bindClass< Regex >("Regex", {
                {"apply",defmem(Regex::Apply)},
                {"indexOf",defmem(Regex::IndexOf)},
                {"exists",defmem(Regex::Exist)},
                {"construct",defmem(Regex::Create)}});
    }
    inline void LoadLibComplex(){
        //!TODO
    }
    inline void LoadAllLibs(){
        LoadLibRegex();
        LoadLibMath();
    }
    inline void InitVM(){
      assert(vm!=nullptr);
      assert(bb!=nullptr);
      vm->SetBasicBlock(bb);
      #define api this
      BINDALL;
      #undef api
      LoadAllLibs();
    }

    inline void InitVM(const char*path){
      assert(vm!=nullptr);
      vm->LoadAssembly(path);
      #define api this
      BINDALL;
      #undef api
      LoadAllLibs();
    }

    void Require(const char*path){
       if(strlen(path)<5)
               throw ApiError("File name non valid. *.nlc - binary file | *nls - sourse file!");
       if(strcmp(path+strlen(path)-4,".nlc")==0){
               InitVM(path);
       }else if(strcmp(path+strlen(path)-4,".nls")==0){
               CompileFile(std::string(path));
               PreprocessBitCode();
               InitVM();
       }else
               throw ApiError("File name don't recognized. *.nlc - binary file | *nls - sourse file! ");
    }

    void Execute(){
      assert(vm!=nullptr);
      if(vm->isInitialized() == false)
	throw ApiError("Cannot execute non-init. VirtualMachine");
      vm->run();
    }

    inline void Release(){
      delete this;
    }

  };
}
