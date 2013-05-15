#pragma once
#include <regex>
#include "nvalue.hpp"
#include "nbind.hpp"
namespace nls{
        class Regex{
                std::regex reg;
                std::cmatch cm;
        public:
                Regex(){}
                ~Regex(){}
                Regex(std::string regex){
                        try{
                                reg = std::regex(regex);
                        }catch(...){
                                throw std::string("Invalid regex");
                        }
                }
                static std::vector<std::string> Apply(Regex* regex,std::string what){
                        auto reg = regex->reg;
                        auto cm = regex->cm;
                        try{
                                std::regex_match(what.c_str(), cm,reg);
                        }catch(...){
                                return std::vector<std::string>();
                        }
                        return std::vector<std::string> (cm.begin(),cm.end());
                }
                static int32_t IndexOf(Regex* regex,std::string what){
                        auto reg = regex->reg;
                        auto cm = regex->cm;
                        try{
                                std::regex_match(what.c_str(), cm,reg);
                        }catch(...){
                                return -1;
                        }
                        if(cm.size()==0)
                                return -1;
                        return cm.position();
                }
                static bool Exist(Regex* regex,std::string what){
                        return Regex::IndexOf(regex, what)!=-1;
                }
                static void Create(Regex* regex,std::string templ){
                        *regex = Regex(templ);
                }
        };
}