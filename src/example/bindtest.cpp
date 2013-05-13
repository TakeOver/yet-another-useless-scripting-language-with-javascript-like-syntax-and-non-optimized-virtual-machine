#include "../nls.hpp"
using namespace nls;
int main(int argc, char const *argv[])
{
        NlsApi* api = new NlsApi();
        api->Require("testf.nls");
        api->Execute();
        ScriptFunction<std::string> func = api->getFunction<std::string>("hello");
        std::cout<<func()<<'\n';
        auto next = api->getFunction<long double>("obj", "method");
        std::cout<< next("johnOne")<<'\n';
        auto var = api->getFunction<void>("variadic");
        var("str1",123,-0.113456,"str2",true,false,std::string("str3"),123,-32,nullptr);
        delete api;
        return 0;
}