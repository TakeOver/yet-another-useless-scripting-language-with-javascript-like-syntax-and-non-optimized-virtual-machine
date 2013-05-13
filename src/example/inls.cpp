#include "../nls.hpp"
int main(int argc,char**argv){
  auto api = new nls::NlsApi();
  api->CompileFile("std.nls");
  std::string src;
  std::getline(std::cin,src);
  api->CompileText(src);
  api->PreprocessBitCode();
  api->InitVM();
  api->Execute();
  api->Release();
}
