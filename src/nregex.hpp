#pragma once
#include <regex>
#include "nvalue.hpp"
#include "nbind.hpp"
namespace nls{
        class Regex{
                std::regex reg;
                std::cmatch cm;
        public:
                ~Regex(){}
                Regex(){}
                Regex(std::string regex){
                        try{
                                reg = std::regex(regex);
                        }catch(...){
                                throw std::string("Invalid regex");
                        }
                }
                std::vector<std::string> Apply(std::string what){
                        try{
                                std::regex_match(what.c_str(), cm,reg);
                        }catch(...){
                                return std::vector<std::string>();
                        }
                        return std::vector<std::string> (cm.begin(),cm.end());
                }
                int32_t IndexOf(std::string what){
                        try{
                                std::regex_match(what.c_str(), cm,reg);
                        }catch(...){
                                return -1;
                        }
                        if(cm.size()==0)
                                return -1;
                        return cm.position();
                }
                bool Exist(std::string what){
                        return IndexOf(what)!=-1;
                }
                static Regex* Create(std::string templ){
                        return new Regex(templ);
                }
        };
}