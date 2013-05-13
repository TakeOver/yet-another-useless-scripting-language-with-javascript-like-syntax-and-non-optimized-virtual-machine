#pragma once
#include <exception>
#include <string>
namespace nls{
  class eval_error: public std::exception{
  std::string _what;
  public:
    eval_error(std::string _):_what(_){}
    virtual const char* what() const noexcept{
      return _what.c_str();
    }
  };
  class vm_error: public std::exception{
    std::string _w;
  public:
    vm_error(decltype(_w)_w):_w(_w){}
    const char*what()const noexcept{
      return _w.c_str();
    }
  };

  class ApiError:public std::exception{
    std::string msg;
  public:
    ApiError(std::string msg):msg(msg){}
    virtual const char* what() const noexcept{
      return msg.c_str();
    }
  };
}
