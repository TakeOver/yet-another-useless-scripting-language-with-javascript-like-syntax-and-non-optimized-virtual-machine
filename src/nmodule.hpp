#pragma once
#include "nvm.hpp"
#include "nparser.hpp"
#include "nbind.hpp"
#include "nstd.hpp"
namespace nls{
        class Module{
                VirtualMachine * vm;
        public:
                Module(GC*gc,std::string path){
                        vm = new VirtualMachine(gc);
                        Parser * par;
                        if(path.length()<3 || std::string(path.end()-4,path.end()) != ".nls"){
                                vm->LoadAssembly(path.c_str());
                                BINDALL(vm,setSysFunction);
                                vm->run();
                        }else{
                                try{
                                        std::ifstream in (path);
                                        std::string res( (  std::istreambuf_iterator<char>( in ) ),
                                                  std::istreambuf_iterator<char>());
                                        in.close();
                                        par = new Parser(new Lexer(res,path),path);
                                        par->Parse();
                                        RefHolder rh;
                                        BasicBlock bb (&rh);
                                        par->getRoot()->emit(&bb);
                                        bb.ReplaceLabels();
                                        bb.emit(IL::hlt);
                                        vm->SetBasicBlock(&bb);
                                        vm->run();
                                }catch(...){
                                        NLogger::log("Cannot load module:"+path);
                                }
                                delete par;
                        }
                }
                ~Module(){
                        delete vm;
                }
                Value __get(std::string what){
                        return vm->GetVariable(what);
                }
                void __set(std::string what,Value val){
                        return vm->SetVariable(what, val);
                }
                static Module* create(GC*gc,std::string path){
                        return new Module(gc,path);
                }
                void Collect(){
                        vm->CallGCCollection();
                }

        };
}