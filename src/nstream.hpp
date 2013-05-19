#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include "nvalue.hpp"
#include <vector>
namespace nls{
        class StringBuilder{
                private:
                        std::stringstream ss;
                public:
                        StringBuilder(){}
                        StringBuilder(StringBuilder &copy){
                                ss.clear();
                                ss<< copy.ss.str();
                        }
                        ~StringBuilder(){}
                        static StringBuilder* create(){
                                return new StringBuilder();
                        }
                        std::string toString(){
                                return ss.str();
                        }
                        void clear(){
                                ss.clear();
                        }
                        void Append(Value val){
                                val.print(ss);
                        }
                        char __get(uint64_t idx){
                                return ss.str()[idx];
                        }
                        StringBuilder& operator=(StringBuilder&) = delete;
        };
        class FileStreamWriter{
                private:
                        std::stringstream ss;
                        bool closed = false;
                public:
                        std::string path;
                        FileStreamWriter(std::string path):path(path){}

                        ~FileStreamWriter(){
                                Close();
                        }
                        FileStreamWriter(FileStreamWriter& fst){
                                Close();
                                this->path = fst.path;
                        }
                        void Close(){
                                if(closed)
                                        return;
                                std::fstream fs (path);
                                if(!fs)
                                        return;
                                fs << ss.str();
                                fs.close();

                        }
                        static FileStreamWriter* create(std::string path){
                                return new FileStreamWriter(path);
                        }
                        void Write(std::string s){
                                ss << s;
                        }
                        void Clear(){
                                ss.clear();
                        }
        };
        class FileStreamReader{
                private:
                        std::vector<std::string> buf;
                        uint64_t idx = 0;
                public:
                        const std::string path;
                        FileStreamReader(std::string path):path(path){
                                std::ifstream in(path);
                                if(!in)
                                        return;
                                std::string str;
                                while(in>>str)
                                        buf.push_back(str);
                        }
                        ~FileStreamReader(){}
                        FileStreamReader(FileStreamReader& fst){
                                this->buf = fst.buf;
                                this->idx = fst.idx;
                        }
                        void Close(){
                                buf.clear();
                                idx = 0;
                        }
                        static FileStreamWriter* create(std::string path){
                                return new FileStreamWriter(path);
                        }
                        std::string Read(){
                                if(!Eof())
                                        return buf[idx++];
                                return std::string();
                        }
                        bool Eof(){
                                return idx>=buf.size();
                        }
                        void Clear(){
                                buf.clear();
                                idx = 0;
                        }
        };
};