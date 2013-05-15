#include "../nls.hpp"
int main(int argc,char**argv){
  if(argc<2){
    std::cerr<<"Too few files\n";
    return 0;
  }
  if(argc>2){
    std::cerr<<"Too many files\n";
    return 0;
  }
  if(strlen(argv[1])<5){
    std::cerr<<"File format not recognized\n";
    return 0;
  }
  if(strcmp(argv[1]+strlen(argv[1])-4,".nls")!=0){
    std::cerr<<"Wrong file format\n";
    return 0;
  }

  auto api = new nls::NlsApi();

  api->Require((argv[1]));

  BINDALL;
  api->Execute();
  api->Release();
}
