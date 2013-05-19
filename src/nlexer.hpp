#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <unordered_map>
#include <sstream>

#include "ntokens.hpp"
#include "nexc.hpp"

namespace nls{
  class Lexer{
  private:
        std::string __path;
    inline void SyntaxError(std::string s){
      std::stringstream ss;
      ss<<"Syntax error at line:"<<lineno<<"\nmsg:\n"<<s <<"\n in file:"<<__path;
      throw eval_error(ss.str());
    }
    std::unordered_map<std::string, token> keywords;
    inline void define_keyword(std::string x,token y){
      keywords[x]=y;
    }
    std::unordered_map<std::string, std::string> aliases, definitions;
    inline void init_kwds(){
        aliases["__FILE__"]="\""  + this->__path;
	define_keyword("return", return_tok);
        define_keyword("alias", alias_tok);
        define_keyword("define", define_tok);
        define_keyword("operator", operator_tok);
        define_keyword("undef", undef_tok);
        define_keyword("defined", defined_tok);
        define_keyword("macro", macro_tok);
	define_keyword("continue", continue_tok);
        define_keyword("class", class_tok);
        define_keyword("extends", extends_tok);
	define_keyword("break", break_tok);
	define_keyword("extern",extern_tok);
	define_keyword("function", fun_tok);
	define_keyword("raise", raise_tok);
	define_keyword("try", try_tok);
	define_keyword("catch", catch_tok);
	define_keyword("finally", finally_tok);
	define_keyword("throw", throw_tok);
	define_keyword("const", const_tok);
	define_keyword("print", print_tok);
	define_keyword("require", require_tok);
	define_keyword("var", var_tok);
	define_keyword("else",else_tok);
	define_keyword("delete",del_tok);
	define_keyword("while",while_tok);
	define_keyword("for",for_tok);
	define_keyword("true",bool_tok);
	define_keyword("false",bool_tok);
	define_keyword("in",in_tok);
	define_keyword("new",oper_tok);
	define_keyword("null",null_tok);
	define_keyword("if",if_tok);
	define_keyword("mod",oper_tok);
	define_keyword("@",oper_tok);
	define_keyword(":", doubledot);
	define_keyword(";", splitter);
	define_keyword(",", comma);
	define_keyword("?", qu_mark);
	define_keyword("->", arrow);
	define_keyword("+",oper_tok);
	define_keyword("++",oper_tok);
	define_keyword("--",oper_tok);
	define_keyword("-",oper_tok);
	define_keyword("*",oper_tok);
	define_keyword("/",oper_tok);
	define_keyword("=",oper_tok);
	define_keyword("/",oper_tok);
	define_keyword("||",oper_tok);
	define_keyword("&&",oper_tok);
	define_keyword("!",oper_tok);
	define_keyword("%",oper_tok);
	define_keyword("++",oper_tok);
	define_keyword("--",oper_tok);
	define_keyword("+=",oper_tok);
	define_keyword("-=",oper_tok);
        define_keyword("||=",oper_tok);
        define_keyword("&&=",oper_tok);
	define_keyword("|",oper_tok);
	define_keyword("%=",oper_tok);
	define_keyword("==",oper_tok);
	define_keyword("!=",oper_tok);
	define_keyword("<>",oper_tok);
	define_keyword(">",oper_tok);
	define_keyword("#",oper_tok);
	define_keyword("<",oper_tok);
	define_keyword(">=",oper_tok);
	define_keyword("<=",oper_tok);
	define_keyword("(",open_br);
	define_keyword(")",close_br);
	define_keyword("[",open_sq);
	define_keyword("]",close_sq);
	define_keyword("{",open_bl);
	define_keyword("}",close_bl);
        define_keyword("do", do_tok);
	define_keyword("..",cnct_tok);
        define_keyword("...",pack_tok);
	define_keyword(".",dot);
      }
      std::string src;
      bool failed = false;
      uint64_t lineno = 1;
      uint64_t pos = 0;
      inline void trimSrc(){
	while(pos<src.length() && std::isspace(src[pos])){
	  lineno+= (src[pos]=='\n');
	  pos++;
	}
	if(pos+1<src.length()&&(src[pos] == '/' && src[pos+1]=='/')){
	  while(src[pos]!='\n' && src[pos]!='\0')
	    pos++;
	  lineno+= (src[pos]=='\n');
	  pos++;
	  trimSrc(); //! \a sic!
	}else if(pos+1<src.length() && (src[pos]=='/' && src[pos+1]=='*')){
	  bool reached = false;
	  pos+=2;
	  while(!reached && pos+1<src.length()){
	    reached = src[pos]=='*' && src[pos+1]=='/';
	    ++pos;
	  }
	  ++pos;
	  if(reached)
	    trimSrc();
	}
      }

      inline bool is_digit(char c, bool begin){
	return (c>='0' && c<='9')
		|| (!begin
		    && ((c=='e'|| c== 'E')|| (c=='.')));
      }
      inline std::string getNumber(){
	std::string res;
	bool find_dot = false;
	while(pos<src.length()&& (is_digit(src[pos],false)&&(src[pos]!='.'||!find_dot))){
		if(src[pos]=='.')
			find_dot=true;
	  if(src[pos]=='e' || src[pos]=='E'){
	    res+=src[pos++];
	    if(src[pos]!='-' && src[pos]!='+'){
	      continue;
	    }
	  }
	  res+=src[pos++];
	}
	while(res.back()=='.'){
	  --pos;
	  res.pop_back();
	}
	return res;
      }

      inline bool is_ident(char c){
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c=='_') || (is_digit(c,false)
	&& c!='-'&& c!='.' && c!='+');
      }

      inline std::string getIdentifer(){
	std::string res;
	while(pos<src.length() && is_ident(src[pos])){
	  res+=src[pos++];
	}
	return res;
      }
      inline std::string getOper(){
	std::string res; res = src[pos++];
	unsigned int delta = 0;
	while(delta + pos < src.length() && keywords[res+src[pos+delta]]!=undefined){
	  res+= src[pos+ delta++];
	}
	pos+=delta;
	return res;
      }
      inline void escape_str(std::string & str){
	std::string res;
	for(unsigned int i = 0,_len=str.length();i<_len;){
	  if(str[i] == '\\'){
	    ++i;
	    if(i<_len && str[i] == 'x'){
	      uint32_t code = 0;
	      ++i;
	      if(i>=_len || !is_digit(str[i],true)){
		res+="\\x";
		continue;
	      }
	      uint _nums = 4u - str[i]>'2';
	      while(i<_len && is_digit(str[i],true) && --_nums){
		code=code*10u + str[i++];
		if(code>255u){
		  code/=10u;
		  --i;
		  break;
		}
	      }
	      res+= (char) code;
	      continue;
	    }
	    switch(str[i]){
	      case '\"':
		res+='\"';
		break;
	      case '\'':
		res+='\'';
		break;
	      case 'n':
		res+='\n';
		break;
	      case 't':
		res+='\t';
		break;
	      case 'r':
		res+='\r';
		break;
	      case 'a':
		res+='\a';
		break;
	      case 'f':
		res+='\f';
		break;
	      case 'b':
		res+='\b';
		break;
	      case '\\':
		res+='\\';
		break;
	      case '0':
		res+='\0';
		break;
	      default:
		res+="\\";
		res+=str[i];
		break;
	    }
	    ++i;
	    continue;
	  }
	  res+=str[i++];

	}
	str= res;
      }

      inline std::string getStringConst(){
	std::string res;
	auto start_sym = src[pos++];
	while(pos<src.length() && src[pos]!=start_sym){
	  if(src[pos] == '\\' && pos+1< src.length() && src[pos+1] == start_sym){
	    res+=src[++pos];
	    ++pos;
	  }else{
	    res+=src[pos++];
	  }
	}
	if(src[pos]!=start_sym){
	  SyntaxError("Wrong string constant. String constant not closed.");
	}
	++pos;
	escape_str(res);
	return "\""+res;
      }
      inline bool is_alias(std::string&x){
        return aliases.find(x)!=aliases.end();
      }
      inline std::string _getTok(){
	char c = src[pos];
	{
	  if(is_digit(c,true))
	    return getNumber();
	  if(is_ident(c))
	    return getIdentifer();
	  if(c == '\'' || c == '\"')
	    return getStringConst();
	  return getOper();
	}
      }

      token_value last_token = token_value(undefined,""),
		  next_token = last_token;

      inline void _getnxtk(bool preprocess = true){
	trimSrc();
        aliases["__LINE__"] = std::to_string(lineno);
	if(src.length() <= pos){
	  last_token = token_value(eof_tok,"$EOF$");
          return;
        }
	std::string tok_str = _getTok();
        if(preprocess && is_def(tok_str)){
                src.insert(pos,definitions[tok_str]);
                _getnxtk(preprocess);
                return;
        }
        while(preprocess && is_alias(tok_str)){
                std::string & y = aliases[tok_str];
                if(y==tok_str){
                        throw nls::eval_error("Recursive alias:"+tok_str);
                }
                tok_str=y;
        }
	last_token = token_value(keywords[tok_str],tok_str);
	if(last_token.tok == undefined) {
	  if(is_digit(last_token.str.front(),true)){
	    last_token.tok = num_tok;
	  }else if(last_token.str.front() == '\"' || last_token.str.front() == '\''){
	    last_token.tok = str_tok; last_token.str.erase(last_token.str.begin());
	  } else if (is_ident(last_token.str.front())){
	    last_token.tok = ident_tok;
	  }else{
	//    SyntaxError(std::string("Unknown character ")+last_token.str);
                last_token = {undefined,tok_str};
	  }
	}
      }
    public:

      void export_defs(Lexer*lex){
        for(auto&x:lex->keywords)
                this->keywords[x.first]=x.second;
        for(auto&x:lex->aliases)
                this->aliases[x.first]=x.second;
        for(auto&x:lex->definitions)
                this->definitions[x.first]=x.second;
        aliases["__FILE__"]="\""  + this->__path;
      }
      Lexer(){
	  init_kwds();
      }

      Lexer(std::string src,std::string path="undefined"):__path(path),src(src){
	  init_kwds();
      }
      void dumpAliasesAndDefs(){
        std::cerr<<"aliases:\n";
        for(auto&x:aliases){
                std::cerr<<"\t"<<x.first<<":"<<x.second<<'\n';
        }
        std::cerr<<"definitions:\n";
        for(auto&x:definitions){
                std::cerr<<"\t"<<x.first<<":"<<x.second<<'\n';
        }
      }
      inline std::string&getLastStr(){
        return last_token.str;
      }
      inline token& getLastTokV(){
        return last_token.tok;
      }
      inline std::string&getNextStr(bool preprocess = true){
        auto & tk = getNextTok(preprocess);
        return tk.str;
      }
      inline token& getNextTokV(){
        auto &tk = getNextTok();
        return tk.tok;
      }
      inline std::string scanToEOL(){
        std::stringstream ss;
        while(pos<src.length() && src[pos]!='\n'){
                ss<<src[pos];
                ++pos;
        }
        return ss.str();
      }
   inline bool is_def(std::string &x){
        return definitions.find(x)!=definitions.end();
    }
    inline void define(std::string &x,std::string y){
        definitions[x]=y;
    }
    inline bool undef(std::string &x){
        auto iter = definitions.find(x);
        if(iter==definitions.end())
                return false;
        definitions.erase(iter);
        return true;
    }
    inline void alias(std::string &x,std::string &y){
        aliases[x]=y;
    }

      inline bool isSuccess(){
	return !failed;
      }
      inline auto getLine()-> decltype(lineno){
	return lineno;
      }
      inline void setError(){
      	pos = src.length();
      	last_token = {error_tok,"ERROR"};
      }
      inline token_value& getNextTok(bool preprocess = true){
	try{
	  _getnxtk(preprocess);
	}catch(nls::eval_error &e){
	  return last_token={error_tok,e.what()};
	}
	return last_token;
      }

      inline token_value& getLastTok(){
	return last_token;
      }
      inline void setOperator(std::string&kwd){
        define_keyword(kwd, oper_tok);
      }
  };

}
