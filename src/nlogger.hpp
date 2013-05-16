#pragma once
#include <string>
#include <iostream>
#include <fstream>
namespace nls{
        class NLogger{
                protected:
                        NLogger(){
                        }
                        ~NLogger(){
                        }
                public:
                        NLogger& operator = (NLogger & rhs) = delete;
                        static void log (std::string msg, uint warn = 1){
                                static std::ofstream out ("nls.log");
                                out << "NLogger::["<<warn<<"]:\t"<<msg<<std::endl;
                        }
        };
}