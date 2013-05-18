#include "../nls.hpp"
#include "../nregex.hpp"
using namespace nls;
std::string test(long double val){
        return std::to_string(val);
}
uint64_t test2(uint32_t i,bool b,const char* c, Value obj){
        std::cout<<i<<'\n'
        <<(b?"true":"false")<<'\n'
        <<c<<'\n';
        obj.print(std::cout);
        std::cout<<'\n';
        return 42;
}
void testvoid(void){
        std::cout<<"Ok!\n";
}
class CLASS{
        long double _a;
public:
        int b;
        CLASS(long double a){
                _a=a;
        }
        CLASS(){_a=0;}
        long double getA(){
                std::cerr<<"CLASS::getA\n";
                return _a;
        }
        void setA(long double val){
                std::cerr<<"CLASS::setA\n";
                _a = val;
        }
        void print(std::ostream & out){
                out<<"CLASS{a="<<_a<<"}";
        }
        std::string toString(){
                std::cerr<<"CLASS::toString\n";
                std::stringstream ss;
                print(ss);
                return ss.str();
        }
        CLASS& operator+=(CLASS &cl){
                this->_a+=cl._a;
                this->b+=cl.b;
                return *this;
        }
        bool operator!(){
                return !this->_a;
        }
        CLASS& operator++(){
                this->_a++;
                return *this;
        }
};
int main(int argc, char const *argv[])
{
        NlsApi* api = new NlsApi();
        api->Require("testf.nls");
        api->SetMainArgs(argc, argv);
        api->NativeBind("native", test);
        api->NativeBind("test2", test2);
        api->NativeBind("voidf", testvoid);
        api->bindClass<CLASS>(
                "TestClass",{
                        immutableFieldStrict(CLASS,b),
                        {"__set:a",def(&CLASS::setA)},
                        {"__print",def(&CLASS::print)},
                        {"__get:a",def(&CLASS::getA)},
                        {"__tostr",def(&CLASS::toString)},
                        {"__add",def(&CLASS::operator+=)},
                        {"__not",def(&CLASS::operator!)},
                        {"__inc",def(&CLASS::operator++)},
                        {"construct",def((void(*)(CLASS*, long double))
                                [](CLASS*cl,long double v){
                                        cl->setA(v);
                                })}});
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