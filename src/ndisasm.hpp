#pragma once

#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nexc.hpp"
#include "ncodegen.hpp"

#include <sstream>
#include <iostream>

#include <map>

namespace nls{
  using namespace IL;
  using namespace Math;
  using namespace Jump;
  using namespace ConstTypes;
class DisAssembly{
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
  }
  void initBitCode(const char* fdata){
    char* _pt = (char*)fdata;
    registers = *(uint16_t*)_pt;
    _pt+=sizeof(uint16_t);
    size = *(uint64_t*)_pt;
    _pt+=sizeof(uint64_t);
    rootsize = *(uint16_t*)_pt;
    for (int i = 0; i < rootsize; ++i)
    {
            roots.push_back(*(uint16_t*)_pt);
            _pt+=sizeof(uint16_t);
    }
    _pt+=sizeof(uint16_t);

    loadAssemblyConst(_pt);
    pcode* _code = (pcode*) _pt;
    for(uint i=0;i<size;++i)
      code.push_back(*(_code+i));
    code.push_back(pcode(IL::hlt));
  }
  std::vector<constpcode> cpool;
  std::vector<pcode> code;
  uint32_t registers;
  uint64_t size;
  uint16_t rootsize;
  std::vector<uint16_t> roots;
  void loadAssemblyConst(char*&ptr){
    constpcode * x;
    while(true){
      x = (constpcode*)ptr;
      if(x->op!=IL::loadConstcc)
	return;
      switch(x->subop){
	case 3:  cpool.push_back(constpcode(
		  IL::loadConstcc,
		  IL::ConstTypes::string,
		  x->dest,
		  (char*)(ptr+sizeof(constpcode))));
		ptr+=strlen(((char*)(x+1)))+1;
		break;
	case 0:  cpool.push_back(constpcode(IL::loadConstcc,IL::ConstTypes::null,x->dest,0ll));   break;
	case 1:cpool.push_back(constpcode(IL::loadConstcc,IL::ConstTypes::boolean,x->dest,x->i));break;
	case 2:  cpool.push_back(constpcode(IL::loadConstcc,IL::ConstTypes::number,x->dest,x->f));   break;
      }
      ptr+=sizeof(constpcode);
    }
  }
  std::map<int,std::string> disasm = {{hlt,"hlt"},{nop,"nop"}, {IL::dup,"dup"},{pushNewObj,"new"},
  {push,"push"},{pop,"pop"},{top,"top"}, {loadConstcc,"load"},{objAlloc,"objalloc"},
  {arrAlloc,"arralloc"},{ret,"ret"},{mov,"mov"},{call,"call"},  {createFunc,"func"},
  {math_op,"math"},{setProperty,"set"},{getProperty,"get"},{jcc,"jmp"},
  {label,"label"},{getArg,"getarg"}, {pushArg,"pusharg"},{clearArgs,"clear"},{concat,"concat"},
  {IL::print,"print"},{set_try,"set_try"},{pop_try,"unset_try"} ,{throw_ex,"throw"}, {length,"length"},\
  {del,"delete"},{IL::createArgsArray,"argmunets"}};

  std::map<long,std::string> mathasm = {{(int)add,"+"},{(int)mul,"*"},{(int)sub,"-"},
  {(int)Math::div,"/"},{(int)mod,"%"},{(int)log_or,"||"},{(int)log_and,"&&"},{(int)log_not,"!"},{(int)log_gt,">"},{(int)log_ge,">="},{(int)log_lt,"<"},
  {(int)log_le,"<="},{(int)log_eq,"=="},{(int)log_ne,"!="},{(int)neg,"neg"},{(int)jmp,"anyway"},
  {(int)label,"label"},{(int)jnz,"if true"},{(int)jz,"if false"},{(int)inc,"++"},{(int)IL::Math::dec,"++"}};
  decltype(disasm) constasm = {{null,"null"},{boolean,"bool"},{number,"num"},
  {ConstTypes::string,"str"}};
public:
  DisAssembly(const char* path){
    VirtualAlloc(path);
  }
  void dump(std::ostream& out){
    out<<"Registers used:"<<registers<<"\nGCRoots:\n";
    for (int i = 0; i < rootsize; ++i)
    {
        out<< roots[i]<<'\n';
    }
    uint line = 0;
    for(auto&x:cpool){
      out << "-1" <<'\t';
      out<< disasm[(int)x.op];
      out << std::string("\t");
      out << constasm[(int)x.subop];
      out <<std::string("\t") << 'r'<<x.dest << '\t';
      if(x.subop == null)
	out<<std::string("null");
      else if(x.subop== boolean)
	out<<std::string(x.i?"true":"false");
      else if(x.subop == IL::ConstTypes::string)
	out<<std::string(x.c);
      else{
	std::stringstream ss;
	ss<<x.f;
	out<< ss.str();
      }
      out<<std::string("\n");
    }
    out << '\n';
    for(auto&x:code){
      out << line++ <<'\t';
      out<< disasm[x.op]<< '\t';
      if(x.op==set_try){
	out<<"\t\toffset:"<<x.offset<<"\n";
	continue;
      }
      if(x.subop ==0 && x.op!=jcc){ out<<'\t';}
      else out << mathasm[x.subop]<<'\t';
      if(x.op<=clearArgs){}
      else{
	if(x.op == jcc && x.subop == jmp)
	  out<<'\t';
	else
	  out<<'r'<<x.dest<<'\t';
	if(x.op<=label){}
	else {
	  if(x.op == jcc)
	    out<<"offs:"<<x.offset<<'\t';
	  else if(x.op == createFunc)
	    out<<"offs:"<<x.offset<<'\t';
	  else
	    out<<'r'<< x.src1<<'\t';
	  if(x.op<=jcc){}
	  else {
	    out<< 'r'<<x.src2;
	  }
	}
      }
      out<<'\n';
    }
  }
  ~DisAssembly(){
    VirtualDealloc();
  }
};
}
