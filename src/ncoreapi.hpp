#pragma once

#include "nparser.hpp"
#include "nvm.hpp"
#include "nstd.hpp"
#include "nbind.hpp"
#include "nregex.hpp"
#include "nlogger.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "niterator.hpp"
#include <complex>
namespace nls{

        std::string readLine(){
                std::string line;
                std::getline(std::cin,line);
                return line;
        }

        void writeLine(Value val){
                val.print(std::cout);
        }

        std::string substr(std::string str, int64_t start,int64_t end){
                return str.substr(start,end);
        }

        class NlsApi{
        private:
                BasicBlock *bb;
                VirtualMachine * vm;
                RefHolder * rh;
                uint16_t rootreg = 0;
                std::vector<std::pair<std::string,std::string> > definitions;
                std::vector<std::pair<std::string,std::string> >aliases;
                std::vector<AbstractNativeFunction*> native_binds;
                std::vector<const char*> _argv;
                void __init__(){
                        rh = new RefHolder();
                        bb = new BasicBlock(rh);
                        vm = new VirtualMachine();
                }
        public:

                NlsApi(){
                        __init__();
                }
                ~NlsApi(){
                        delete bb;
                        delete vm;
                        delete rh;
                        for(auto&x:native_binds)
                                delete x;
                }

                template <typename C> void BindVariable(std::string name,C& var, bool constant = false,bool to_ud = true){
                        bindFunction( "__get:" + name,
                                [ &var ](VirtualMachine*vm,Value*){
                                        vm->Push(MarshalType(vm->getGC(), var));
                                },to_ud);
                        if( ! constant ){
                                bindFunction( "__set:" + name,
                                        [ &var ] ( VirtualMachine* vm, Value* ){
                                                var = UserType<C>(vm->GetArg());
                                        }, to_ud);
                        }
                }

                template <typename C> void BindSysVariable(std::string name, C& var, bool constant = false){
                        BindVariable(name, var,constant,false);
                }

                template <typename T1,typename ...T2>void NativeBind(std::string name,T1(*ptr)(T2...), bool to_ud = true){
                        NativeFunction<T1, T2...> *_F = def(ptr);
                        native_binds.push_back(_F); //locking ptr;
                        if(to_ud){
                                setToUD(name, Value(vm->getGC(),new Function(_F)));
                        }else{
                                vm->setSysFunction(name, _F);
                        }
                }

                template <typename T1,typename ...T2>void BindToSystem(std::string name,T1(*ptr)(T2...)){
                        NativeBind(name, ptr,false);
                }

                template <class C,typename... CArgs> void bindSysClass(std::string clazz,C*(* allocator)(CArgs...),std::unordered_map<std::string,AbstractNativeFunction*> _mem){
                        bindClass<C>(clazz,allocator, _mem,false);
                }

                template <class C,typename ...CArgs> void bindClass(std::string clazz,C*(* allocator)(CArgs...), std::unordered_map<std::string,AbstractNativeFunction*> _mem, bool to_ud = true){
                        std::unordered_map<std::string, Value> mem;
                        for(auto&x:_mem){
                                mem[x.first] = Value(vm->getGC(),new Function(x.second));
                                native_binds.push_back(x.second);
                        }
                        auto constr = [clazz,mem,this, allocator](VirtualMachine*vm,Value*){
                                Value self;
                                try{
                                        self.type = Type::userdata;
                                        self.u = new Userdata<C>(allocator(UserType<CArgs> (vm->GetArg())...),mem);
                                }catch(std::string msg){
                                        this->RaiseException(msg);
                                }
                                vm->Push(self);
                        };
                        bindFunction(clazz,constr,to_ud);
                }

                void RaiseException(std::string msg){
                        try{
                                getFunction<void>("RaiseException")(msg);
                        }catch(...){
                                NLogger::log("Failed to get 'RaiseException' in NlsApi::RaiseException with msg:'"+msg+"'");
                        }
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

                template<typename T> void SetValue(std::string name,T what, bool to_ud = true){
                        if(to_ud){
                                setToUD(name, MarshalType(vm->getGC(), what));
                        }else{
                                vm->SetToSystem(name, MarshalType(vm->getGC(), what));
                        }
                }

                template<typename T> void SetSysValue(std::string name,T what){
                        SetValue(name, what,false);
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

                inline void bindFunction(std::string name, VirtualMachine::native_func_t _f, bool to_ud = true){
                        if(to_ud){
                                getUserData()->set(name,Value(vm->getGC(),new Function(_f)));
                        }else{
                                vm->setSysFunction(name,_f);
                        }
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
                        BindToSystem("rand", std::rand);
                        BindToSystem("acos", acosl);
                        BindToSystem("__native__cos", cosl);
                        BindToSystem("__native__sin", sinl);
                        BindToSystem("__native__tg", tanl);
                        BindToSystem("__native__ctg",(long double (*)(long double))
                                [](long double f)->long double{
                                        return 1.0/tanl(f);
                                }
                        );
                        BindToSystem("__native__log", logl);
                        BindToSystem("__native__pow", powl);
                        BindToSystem("__native__sqrt",sqrtl);
                        BindToSystem("__native__lg",log10);
                        BindToSystem("__native__lb",log2l);
                        BindToSystem("__native__lgamma",lgammal);
                        BindToSystem("__native__round", roundl);
                        BindToSystem("__native__ceil",ceill);
                        BindToSystem("__native__atan", atanl);
                        BindToSystem("__native__asin", asinl);
                        LoadLibComplex();
                }

                inline void LoadIOLib(){
                        BindToSystem("__native__write__stdout",writeLine);
                        BindToSystem("__native__read__stdin",readLine);
                        BindToSystem("__native__substr",substr);\
                }

                inline void LoadLibRegex(){
                        bindSysClass< Regex >("Regex",Regex::Create, {
                                {"apply",def(&Regex::Apply)},
                                {"indexOf",def(&Regex::IndexOf)},
                                {"exists",def(&Regex::Exist)}});
                }

                inline void LoadLibComplex(){
                        using Complex = std::complex<long double>;
                        bindSysClass<Complex>("Complex",
                                (Complex*(*)(long double,long double))[](long double real,long double imm)->Complex*{
                                        return new Complex(real,imm);
                                },{{"__get:real",def((long double(*)(Complex*))[](Complex*self)->long double{
                                        return self->real();
                                })},{"__set:real",def((void(*)(Complex*, long double))[](Complex*self, long double val){
                                        return self->real(val);
                                })},{"__get:imag",def((long double(*)(Complex*))[](Complex*self)->long double{
                                        return self->imag();
                                })},{"__set:imag",def((void(*)(Complex*, long double))[](Complex*self, long double val){
                                        return self->imag(val);
                                })},{"__tostr", def((std::string(*)(Complex*))[](Complex * self)->std::string{
                                        return std::string("{\"real\":")+std::to_string(self->real())+ ",\"imag\":"+std::to_string(self->imag())+"}";
                                })}
                                } );
                }

                inline void LoadIterators(){
                        bindSysClass<HTableIterator>("__obj__iter",HTableIterator::create, {
                                {"next",def(&HTableIterator::next)},
                                {"valid",def(&HTableIterator::valid)},
                                {"__inc",def(&HTableIterator::next)}
                        });
                        bindSysClass<ArrayIterator>("__arr__iter",ArrayIterator::create ,{
                                {"next",def(&ArrayIterator::next)},
                                {"valid",def(&ArrayIterator::valid)},
                                {"__inc",def(&ArrayIterator::next)}
                        });
                }

                inline void LoadAllLibs(){
                        LoadLibRegex();
                        LoadLibMath();
                        LoadIterators();
                        LoadIOLib();
                }

                inline void InitVM(){
                        vm->SetBasicBlock(bb);
                        BINDALL(this);
                        LoadAllLibs();
                }

                inline void SetMainArgs(int &argc, char const** &argv){
                        for(uint i=0;i<argc;++i)
                                _argv.push_back(argv[i]);
                        BindSysVariable("argc", argc,true);
                        BindSysVariable("argv", _argv,true);
                }

                inline void InitVM(const char*path){
                        assert(vm!=nullptr);
                        vm->LoadAssembly(path);
                        BINDALL(this);
                        LoadAllLibs();
                }

                void Require(const char*path){
                       if(strlen(path)<5){
                               throw ApiError("File name non valid. *.nlc - binary file | *nls - sourse file!");
                       }
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
                        try{
                                vm->run();
                        }catch(vm_error& err){
                                NLogger::log(std::string("Catched vm_error in NlsApi::Execute, err.what():'")+err.what()+"'");
                        }catch(ApiError& err){
                                NLogger::log(std::string("Catcher ApiError in NlsApi::Execute, err.what():'")+err.what()+"'");
                        }catch(...){
                                NLogger::log("Catched unknown exception in NlsApi::Execute");
                        }
                }

                inline void Release(){
                        delete this;
                }

        };
}
