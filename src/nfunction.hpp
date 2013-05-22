#pragma once
#include "ngcobject.hpp"
#include <ostream>
#include <functional>
namespace nls{
        class VirtualMachine;
        struct Value;
        class Function;

        void __VMForceCall(VirtualMachine*,Function*,VirtualMachine*,Value*);
        void __VMCall(Function*,VirtualMachine*,Value*);

        struct AbstractNativeFunction{
                virtual ~AbstractNativeFunction(){}
                virtual void call(VirtualMachine*,Value*)=0;
        };

        class Function:public GCObject{
        public:
                bool is_native, is_abstract;
                VirtualMachine * owner;

                union{
                        AbstractNativeFunction* abstract;

                        std::function<void(VirtualMachine*,Value*)> * stlfunc;

                        uint32_t offset;
                };
                Function(VirtualMachine*vm, std::function<void(VirtualMachine*,Value*)> stlfunc):is_native(true),is_abstract(false){
                        this->stlfunc = new std::function<void(VirtualMachine*,Value*)> (stlfunc);
                        owner = vm;
                }

                Function(VirtualMachine*vm,uint32_t offset):is_native(false),is_abstract(false),offset(offset){
                        owner = vm;
                }

                Function(VirtualMachine*vm,AbstractNativeFunction* abstract):is_native(true),is_abstract(true), abstract(abstract){
                        owner = vm;
                }

                ~Function(){}

                void print(std::ostream &out){
                        out<<"\"function\"";
                }

                uint32_t getCall(){return offset;}

                std::function<void(VirtualMachine*,Value*)> getNativeCall(){return *stlfunc;}

                void createCall(VirtualMachine*v,Value*s){
                        if(is_abstract){
                                abstract->call(v, s);
                        }else if(is_native){
                                (*stlfunc)(v,s);
                        }else{
                                if(owner!=v){
                                        __VMForceCall(owner,this,v,s);
                                }else{
                                        __VMCall(this,v,s);
                                }
                        }
                }
                void createCall(Value*s){
                        if(is_abstract){
                                abstract->call(owner, s);
                        }else if(is_native){
                                (*stlfunc)(owner,s);
                        }else{
                                        __VMCall(this,owner,s);
                        }
                }
        };
}