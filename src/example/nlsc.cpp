#include "../nls.hpp"
#include <vector>
using std::vector;
int main(int argc, char** argv){
  vector<char*> files;
  char* ext = (char*)".nls";
  for(int i=1;i<argc;++i){
    uint len;
    if((len=strlen(argv[i]))>5){
      if(strcmp(argv[i]+len-4,ext)==0)
	files.push_back(argv[i]);
    }
  }
  if(!files.size()){
        std::cerr<<"No files\n";
        return 0;
  }
  auto nls = new nls::NlsApi();
  for(uint i=0;i<files.size()-1;++i)
    nls->CompileFile(files[i]);
  nls->Require(files.back());
  files.back()[strlen(files.back())-1]='c';
  nls->SaveAssembly(files.back());
  nls->Release();
  std::cout<<"Successful!\n Produced: "<<files.back()<<std::endl;
}
