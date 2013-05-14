#include "../nls.hpp"
using namespace nls;
std::string test(long double val){
        return std::to_string(val);
}
uint64_t test2(uint32_t i,bool b,const char* c){
        std::cout<<i<<'\n'
        <<(b?"true":"false")<<'\n'
        <<c<<'\n';
        return 42;
}
int main(int argc, char const *argv[])
{
        NlsApi* api = new NlsApi();
        api->Require("testf.nls");
        api->NativeBind("native", test);
        api->NativeBind("test2", test2);
        api->NativeBind("experimental_cos", cosl);
        api->NativeBind("experimental_sin", sinl);
        api->NativeBind("experimental_tg", tanl);
        api->NativeBind("experimental_ctg",(long double (*)(long double))
                [](long double f)->long double{
                        return 1.0/tanl(f);
                }
        );
        api->NativeBind("experimental_log", logl);
        api->Execute();
        ScriptFunction<std::string> func = api->getFunction<std::string>("hello");
        std::cout<<func()<<'\n';
        auto next = api->getFunction<long double>("obj", "method");
        std::cout<< next("johnOne")<<'\n';
        auto var = api->getFunction<void>("variadic");
        var(1,"str1",123,-0.113456,"str2",true,false,std::string("str3"),123,-32,nullptr);
        delete api;
        return 0;
}