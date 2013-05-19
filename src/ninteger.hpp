#pragma once
#include "stdint.h"
#include "string"
namespace nls{
        class Integer{
                int64_t val;
        public:
                Integer(){
                        val = 0;
                }
                Integer(int64_t val):val(val){}
                ~Integer(){}
                Integer(Integer&in){
                        this->val = in.val;
                }
                Integer& add(Integer&rhs){
                        this->val+=rhs.val;
                        return *this;
                }
                Integer& mul(Integer&rhs){
                        this->val*=rhs.val;
                        return *this;
                }

                Integer& div(Integer&rhs){
                        this->val/=rhs.val;
                        return *this;
                }

                Integer& mod(Integer&rhs){
                        this->val%=rhs.val;
                        return *this;
                }

                Integer& neg(){
                        this->val=-this->val;
                        return *this;
                }
                bool less(Integer&rhs){
                        return this->val < rhs.val;
                }
                bool great(Integer&rhs){
                        return this->val > rhs.val;
                }
                bool less_eq(Integer&rhs){
                        return this->val <= rhs.val;
                }
                bool great_eq(Integer&rhs){
                        return this->val >= rhs.val;
                }
                bool equal(Integer&rhs){
                        return this->val == rhs.val;
                }
                bool notequal(Integer&rhs){
                        return this->val != rhs.val;
                }
                std::string tostr(){
                        return std::to_string(this->val);
                }
                static Integer* create(int64_t val){
                        return new Integer(val);
                }

        };
}