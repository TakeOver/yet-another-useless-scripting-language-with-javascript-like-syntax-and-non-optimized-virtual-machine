#include "../nconfig.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include "../ndisasm.hpp"

int main(int argc,char**argv){
  if(argc<2){
    std::cerr<<"Too few files\n";
    return 0;
  }
  if(strlen(argv[1])<5){
    std::cerr<<"File format not recognized\n";
    return 0;
  }
  if(strcmp(argv[1]+strlen(argv[1])-4,".nlc")!=0){
    std::cerr<<"Wrong file format\n";
    return 0;
  }
  nls::DisAssembly * dis = new nls::DisAssembly(argv[1]);
  std::stringstream ss;
  dis->dump(ss);
  delete dis;
  if(argc>2){
    std::ofstream out(argv[2]);
    if(!out){
      std::cout<<ss.str()<<std::endl;
    }else out<<ss.str()<<std::endl;
  }else
    std::cout<<ss.str()<<std::endl;
}
