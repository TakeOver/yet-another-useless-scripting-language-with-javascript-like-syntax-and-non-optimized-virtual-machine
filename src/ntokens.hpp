#pragma once

#include <unordered_map>
#include <string>

namespace nls{
    enum token {
      undefined=0, eof_tok,ident_tok, splitter,error_tok,var_tok,null_tok,bool_tok,num_tok,str_tok,
      const_tok,try_tok,catch_tok,finally_tok,raise_tok,throw_tok,doubledot,arrow,oper_tok,in_tok,
      require_tok,open_br,close_br,open_bl,close_bl,open_sq,close_sq,at_tok,extern_tok,class_tok,
      fun_tok,comma, return_tok,dot,del_tok,continue_tok,break_tok,do_tok,extends_tok,
      alias_tok,define_tok,undef_tok,macro_tok,expr_tok,defined_tok,operator_tok,pack_tok,
      if_tok,else_tok,while_tok,for_tok,void_tok, qu_mark, print_tok, cnct_tok
    };
    typedef struct token_value{
      token tok;
      std::string str;
      token_value(token tok,std::string str): tok(tok),str(str){}
    } token_value;
}
