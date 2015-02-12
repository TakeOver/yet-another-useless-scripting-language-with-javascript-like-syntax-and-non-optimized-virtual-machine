#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

#include "ncodegen.hpp"
#include "nexc.hpp"
#include "nlexer.hpp"
#include "nast.hpp"
#include "ntypes.hpp"

namespace nls{
    
    class Parser{
        
    private:
        std::unordered_map<std::string, unsigned int> OpPrecedence;
        
        void init(){
            OpPrecedence["+"]=120;
            OpPrecedence["new"]=150;//160
            OpPrecedence["-"]=120;
            OpPrecedence["++"]=150;
            OpPrecedence["--"]=150;
            OpPrecedence["u-"]=150; //unary// no plus, just ignore it
            OpPrecedence["*"]=130;
            OpPrecedence["/"]=130;
            OpPrecedence["%"]=130;
            OpPrecedence["||"]=40;
            OpPrecedence["&&"]=50;
            OpPrecedence["*="]=20;
            OpPrecedence["+="]=20;
            OpPrecedence["-="]=20;
            OpPrecedence["/="]=20;
            OpPrecedence["%="]=20;
            OpPrecedence["||="]=20;
            OpPrecedence["&&="]=20;
            OpPrecedence["or"]=40;
            OpPrecedence["and"]=50;
            OpPrecedence["!"]=150;
            OpPrecedence["not"]=150;
            OpPrecedence["=="]=90;
            OpPrecedence["!="]=90;
            OpPrecedence["@"]=150;
            OpPrecedence[">"]=100;
            OpPrecedence["<"]=100;
            OpPrecedence[">="]=100;
            OpPrecedence["<="]=100;
            OpPrecedence["="]=20;
            OpPrecedence[".."]=140;
            OpPrecedence["#"]=150;
            OpPrecedence["."]=150;//160
            OpPrecedence["("]=150;//160
            OpPrecedence["?"]=30;
            OpPrecedence["["]=150;//160
        }
        
        Lexer * lex;
        ast_t * root;
        
        inline bool is_right_assoc(std::string &op){
            return op == "=" || is_op_and_assign(op);
        }
        std::unordered_map<std::string, bool> user_unary_oper;
        bool is_unary(std::string& s){
            static std::unordered_map<std::string,bool> _mt = {
                {"++",1},{"--",1},{"not",1},{"!",1},{"new",1},{"#",1},{"@",1}
            };
            return _mt[s] || user_unary_oper[s];
        }
        std::unordered_map<std::string, bool> is_user_operator;
        uint8_t match_oper(std::string &op){
            if(failed)
                return 0;
#define OPER(x) (IL::Math::x)
            static std::unordered_map<std::string, uint16_t> _match = {
                {"=",IL::mov},{"+",OPER(add)},{"-",OPER(sub)},{"*",OPER(mul)},{"/",OPER(div)},{"%",OPER(mod)},
                {"u-",OPER(neg)},{"!",OPER(log_not)},{"not",OPER(log_not)},{"&&=",OPER(log_and)},{"||=",OPER(log_or)},
                {"==",OPER(log_eq)},{"!=",OPER(log_ne)},{"<",OPER(log_lt)},
                {"<=",OPER(log_le)},{">",OPER(log_gt)},{"+=",OPER(add)},{"-=",OPER(sub)},{"*=",OPER(mul)},{"/=",OPER(div)},{"%=",OPER(mod)},
                {">=",OPER(log_ge)},{"&&",OPER(log_and)},{"and",OPER(log_and)},{"||",OPER(log_or)},
                {"or",OPER(log_or)},{"++", OPER(inc)},{"--",OPER(dec)},{"..", (IL::concat)}, {".",IL::getProperty},{"[",IL::getProperty},
                {"(",IL::call},{"new",IL::pushNewObj},{"#",IL::length},{"@",IL::getProperty}
            };
#undef OPER
            return _match[op];
        }
        std::unordered_map<std::string, std::string> oper_func;
        std::unordered_map<std::string, bool> include_guard;
        std::string __error_msg;
        std::string __path;
        bool failed = false;
        ast_t* Error(std::string __1,std::string __2){
            std::stringstream ss;
            ss<<"Syntax error on line "<< lex->getLine()<<"\n"<<__1<<__2<<"\n in file:"<<__path;
            __error_msg = ss.str();
            failed = true;
            lex->setError();
            return nullptr;
        }
        
        ast_t* ParseNumber(){
            if(failed)
                return nullptr;
            auto _ = new ConstantExpression(lex->getLastStr(),
                                            static_cast<Type::types>(lex->getLastTokV() - bool_tok+ Type::boolean));
            lex->getNextTok();
            return _;
        }
        ast_t*ParseDelExpr(){
            lex->getNextTok();
            auto wat = ParseExpression(true);
            if(!wat)
                return nullptr;
            
            if(dynamic_cast<Variable*>(wat))
                return new Expression(
                                      new BinaryExpression(IL::mov,wat,
                                                           new ConstantExpression("null",Type::null)));
            
            auto prop = dynamic_cast<PropertyExpression*>(wat);
            if(!prop){
                delete wat;
                return Error("PropertyExpression|Variable exptected, found:",lex->getLastStr());
            }
            return new DelExpression(prop);
        }
        ast_t* ParseIdentifer(){
            std::string id = lex->getLastStr();
            lex->getNextTok();
            ast_t* var =  new Variable(id);
            return var;
        }
        
        std::vector<ast_t*> ParseArgs(bool hanlde_packed = false){
            if(failed)
                return std::vector<ast_t*>();
            std::vector<ast_t*> args;
            while(true){
                if(lex->getLastTokV() == close_br){
                    return args;
                }
                if(lex->getLastTokV()==pack_tok && hanlde_packed){
                    lex->getNextTokV();
                    auto val = ParseExpression();
                    if(!val){
                        for(auto&x:args)
                            delete x;
                        failed = true;
                        return std::vector<ast_t*>();
                    }
                    args.push_back(new PackedArg(val));
                }else{
                    ast_t* tmp;
                    tmp = ParseExpression();
                    if(!tmp){
                        for(auto&x:args)
                            delete x;
                        return std::vector<ast_t*>();
                    }
                    args.push_back(tmp);
                }
                if(lex->getLastTokV() == comma)
                    lex->getNextTok();
            }
        }
        ast_t* ParseArray(){
            lex->getNextTok();
            std::vector<ast_t*> res;
            while(true){
                if(lex->getLastTokV()==close_sq)
                    break;
                if(lex->getLastTokV() == comma){
                    res.push_back(new ConstantExpression("null",Type::null));
                    lex->getNextTok();
                    continue;
                }
                auto _tmp = ParseExpression();
                if(!_tmp){
                    for(auto&x:res)
                        delete x;
                    return nullptr;
                }
                res.push_back(_tmp);
                if(lex->getLastTokV() == close_sq)
                    break;
                if(lex->getLastTokV()==comma)
                    lex->getNextTok();
                else{
                    for(auto&x:res)
                        delete x;
                    return Error("',' Expected in initial. list of array, but found:",lex->getLastStr());
                }
            }
            lex->getNextTok();
            return new ArrayExpression(res);
        }
        ast_t* ParseClass(){
            if(lex->getNextTokV()!=ident_tok)
                return Error("Idntifer expected in class definition, found:",lex->getLastStr());
            std::string _cl_name = lex->getLastStr();
            BlockExpression* _class = new BlockExpression();
            if(lex->getNextTokV()==extends_tok){
                auto _what = lex->getNextTok();
                if(_what.tok!=ident_tok){
                    delete _class;
                    return Error("Identifer expected in inherit, found:",lex->getLastStr());
                }
                _class = new BlockExpression(_class,
                                             new BinaryExpression(IL::mov,
                                                                  new PropertyExpression(
                                                                                         new Variable("this"),
                                                                                         new ConstantExpression("prototype", Type::str), true),
                                                                  new NewClassExpr(
                                                                                   new FuncCallExpression(
                                                                                                          new Variable(_what.str), std::vector<ast_t*>()))));
                _class = new BlockExpression(_class,
                                             new BinaryExpression(IL::mov,
                                                                  new PropertyExpression(
                                                                                         new Variable("this"),
                                                                                         new ConstantExpression("__proto__", Type::str),false),
                                                                  new Variable(_what.str)));
                _class = new BlockExpression(_class,
                                             new BinaryExpression(IL::mov,
                                                                  new PropertyExpression(
                                                                                         new Variable("this"),
                                                                                         new ConstantExpression("super", Type::str), true),
                                                                  new PropertyExpression(
                                                                                         new PropertyExpression(
                                                                                                                new Variable("this"),
                                                                                                                new ConstantExpression("prototype", Type::str), false),
                                                                                         new ConstantExpression("construct", Type::str), false)));
                lex->getNextTokV();
            }else{
                _class = new BlockExpression(_class,
                                             new BinaryExpression(IL::mov,
                                                                  new PropertyExpression(
                                                                                         new Variable("this"),
                                                                                         new ConstantExpression("prototype", Type::str), true),
                                                                  new NewClassExpr(
                                                                                   new FuncCallExpression(
                                                                                                          new Variable("Object"), std::vector<ast_t*>()))));
                _class = new BlockExpression(_class,
                                             new BinaryExpression(IL::mov,
                                                                  new PropertyExpression(
                                                                                         new Variable("this"),
                                                                                         new ConstantExpression("__proto__", Type::str),false),
                                                                  new Variable("Object")));
                
            }
            _class = new BlockExpression(_class,
                                         new BinaryExpression(IL::mov,
                                                              new PropertyExpression(
                                                                                     new Variable("this"),
                                                                                     new ConstantExpression("_F", Type::str),false),
                                                              new Variable("_F")));
            if(lex->getLastTokV()!=open_bl){
                delete _class;
                return Error("{ expected in class definition, found:",lex->getLastStr());
            }
            lex->getNextTokV();
            std::vector<std::string> args;
            bool found_constr = false;
            bool packed_constr = false;
            ast_t* constr = nullptr;
            while(lex->getLastTokV()!=close_bl){
                if(lex->getLastStr()=="construct"){
                    if(found_constr){
                        delete _class;
                        return Error("Multiply constructors are not possible,found:",lex->getLastStr());
                    }
                    found_constr=true;
                    if(lex->getNextTokV()!=open_br){
                        delete _class;
                        return Error("( expected as constructor function in class def.,found:",lex->getLastStr());
                    }
                    lex->getNextTokV();
                    while(lex->getLastTokV()!=close_br){
                        if(lex->getLastTokV()!=ident_tok){
                            delete _class;
                            return Error("Identifer expected in constructor declaration,found:",lex->getLastStr());
                        }
                        args.push_back(lex->getLastStr());
                        if(lex->getNextTokV() == pack_tok){
                            packed_constr = true;
                            if(lex->getNextTokV()!=close_br){
                                delete _class;
                                return Error(") expected after packed arg declaration,found:",lex->getLastStr());
                            }
                        }
                        if(lex->getLastTokV()==comma)
                            lex->getNextTokV();
                    }
                    lex->getNextTokV();
                    constr = ParseBlock();
                    if(!constr){
                        delete _class;
                        return nullptr;
                    }
                }else if(lex->getLastTokV()== fun_tok){
                    auto _func = ParseFuncDef(false,true);
                    if(!_func){
                        delete _class;
                        return nullptr;
                    }
                    FuncDefinition* func = dynamic_cast<FuncDefinition*>(_func);
                    if(!func){
                        delete _class;
                        delete _func;
                        return Error("WTF&!","function not function!");
                    }
                    auto newfd = new FuncDefinition("", func->args, func->body, true);
                    func->body=nullptr;
                    _class = new BlockExpression(_class,
                                                 new BinaryExpression(IL::mov,
                                                                      new PropertyExpression(
                                                                                             new Variable("this"),
                                                                                             new ConstantExpression(func->name, Type::str), true), newfd));
                    delete _func;
                }else if(lex->getLastTokV() == const_tok ||lex->getLastTokV()==var_tok){
                    auto id = lex->getNextTok();
                    if(id.tok!=ident_tok){
                        delete _class;
                        return Error("Identifer expected in class definition,found:",id.str);
                    }
                    ast_t* val = nullptr;
                    if(lex->getNextStr()=="="){
                        lex->getNextTokV();
                        val = ParseExpression();
                        if(!val){
                            delete _class;
                            return nullptr;
                        }
                    }
                    if(!val)
                        val = new ConstantExpression("null", Type::null);
                    _class = new BlockExpression(_class,
                                                 new BinaryExpression(IL::mov,
                                                                      new PropertyExpression(
                                                                                             new Variable("this"),
                                                                                             new ConstantExpression(id.str, Type::str), true), val));
                }else{
                    delete _class;
                    return Error("Unexpected token in class def.:",lex->getLastStr());
                }
                while(lex->getLastTokV()==splitter)
                    lex->getNextTokV();
            }
            lex->getNextTokV();
            if(!constr)
                constr = new BlockExpression();
            std::vector<ast_t*> __args;
            for(auto&x:args)
                __args.push_back(new Variable(x));
            auto confunc = new FuncDefinition("", args, constr, true,false,packed_constr);
            _class = new BlockExpression(_class,
                                         new BinaryExpression(IL::mov,
                                                              new PropertyExpression(
                                                                                     new Variable("this"),
                                                                                     new ConstantExpression("construct", Type::str), true), confunc));
            _class  = new BlockExpression(_class,
                                          new FuncCallExpression(
                                                                 new PropertyExpression(
                                                                                        new Variable("this"),
                                                                                        new ConstantExpression("construct", Type::str), false), __args,true));
            return new FuncDefinition(_cl_name, args, _class, false);
            
        }
        
        ast_t* ParseUnary(){
            if(failed)
                return nullptr;
            auto _str = lex->getLastStr(); //check for -+
            if(_str == "+"){
                lex->getNextTok(); //eat +
                auto left = ParsePrimary();
                if(!left){
                    return Error("Primary excepted in unary expression,found:",lex->getLastStr());
                }
                auto oper = match_oper(lex->getLastStr());
                if(!oper)
                    return left;
                auto rest = ParseRest(OpPrecedence["u-"],left);
                return rest;
            }
            if(_str == "-")
                _str = "u-";
            int op = match_oper(_str);
            
            if(op == IL::pushNewObj){
                lex->getNextTok();
                auto val = ParseIdentifer();
                while(lex->getLastTokV()==dot || lex->getLastTokV()==open_sq){
                    if(lex->getLastTokV()==dot){
                        auto id  = lex->getNextTok();
                        if(id.tok!=ident_tok){
                            delete val;
                            return Error("Identifer expected in class-call expr,found:",id.str);
                        }
                        val = new PropertyExpression(val,
                                                     new ConstantExpression(id.str, Type::str), false);
                    }else{
                        lex->getNextTokV();
                        auto inx = ParseExpression();
                        if(!inx){
                            delete val;
                            return nullptr;
                        }
                        if(lex->getLastTokV()!=close_sq){
                            delete val;
                            delete inx;
                            return Error("] Expected in object factory call,found:",lex->getLastStr());
                        }
                        val = new PropertyExpression(val, inx, false);
                    }
                    lex->getNextTokV();
                }
                if(lex->getLastTokV()!=open_br){
                    return new NewClassExpr(
                                            new FuncCallExpression(val, std::vector<ast_t *> ()));
                }
                lex->getNextTok();
                auto args = ParseArgs();
                if(failed){
                    delete val;
                    return nullptr;
                }
                if(lex->getLastTokV()!=close_br){
                    delete val;
                    for(auto&x:args)
                        delete x;
                    return Error("')' exptected,but found:",lex->getLastStr());
                }
                lex->getNextTok();
                return new NewClassExpr(new FuncCallExpression(val,args));
            }else if( op == IL::getProperty){
                auto id =lex->getNextTok();
                if(id.tok!=ident_tok){
                    return new Variable("this");
                }
                lex->getNextTok();
                return ParseRest(OpPrecedence[_str],
                                 new PropertyExpression(
                                                        new Variable("this"),
                                                        new ConstantExpression(id.str,Type::str),false));
            }else if(is_user_op(_str)){
                lex->getNextTokV();
                auto what = ParsePrimary();
                if(!what)
                    return nullptr;
                if(lex->getLastTokV()==oper_tok){
                    what = ParseRest(OpPrecedence[_str],what);
                    if(!what)
                        return nullptr;
                }
                return new FuncCallExpression(new Variable(_str+"$operator"), {what});
            }
            lex->getNextTok();
            auto left = ParsePrimary();
            if(!left)
                return nullptr;
            auto oper = match_oper(lex->getLastStr());
            if(!oper)
                return new UnaryExpression(op,left,true);
            left = ParseRest(OpPrecedence[_str],left);
            return new UnaryExpression(op,left, /*! @p pre_inc|dec:*/true);
        }
        ast_t * ParseParen(){
            if(failed)
                return nullptr;
            lex->getNextTok();
            auto _expr = ParseExpression();
            if(!_expr)
                return nullptr;
            lex->getNextTok();
            return _expr;
        }
        inline bool is_op_and_assign(std::string&s){
            return (s=="+=" || s=="-=" || s=="%=" || s=="*=" || s=="/=" || s=="||=" || s=="&&=");
        }
        ast_t *ParseExpression(bool return_as_is = false){
            if(failed)
                return nullptr;
            auto left = ParsePrimary();
            if(!left)
                return nullptr;
            if(!is_user_op(lex->getLastStr()) && !match_oper(lex->getLastStr()))
                return left;
            auto res = ParseRest(0,left);
            if(!res){
                return nullptr;
            }
            
            if(return_as_is && (dynamic_cast<PropertyExpression*>(res) || dynamic_cast<Variable*>(res)))
                return res;
            return new Expression(res);
        }
        
        void export_defs(Parser* par){
            for(auto&x:par->OpPrecedence)
                this->OpPrecedence[x.first]=x.second;
            for(auto&x:par->is_user_operator)
                this->is_user_operator[x.first]=x.second;
            for(auto&x:par->oper_func)
                this->oper_func[x.first]=x.second;
            for(auto&x:par->user_unary_oper)
                this->user_unary_oper.insert(x);
            this->lex->export_defs(par->lex);
        }
        //! \brief This function would create new lexer and parser and evaluate file; no double include guard
        //! FIXME \p DOUBLE-INCLUDE-GUARD
        ast_t*ParseRequire(){
            auto tok = lex->getNextTok(false);
            std::string wat;
            if(tok.tok == str_tok){
                wat = tok.str;
            }else if(tok.tok == ident_tok)
                wat = tok.str +".nls";
            else
                return Error("identifer of string-path expected in require expression,found:",tok.str);
            if(include_guard[wat]){
                lex->getNextTokV();
                return new BlockExpression();
            }else{
                include_guard[wat]=true;
            }
            std::ifstream in (wat);
            if(!in)
                return Error("cannon load module in require, found:",tok.str);
            
            std::string res( (  std::istreambuf_iterator<char>( in ) ),
                            std::istreambuf_iterator<char>());
            in.close();
            
            Parser* par = new Parser(new Lexer(res,wat),wat,true);
            par->export_defs(this);
            par->include_guard.insert(include_guard.begin(),include_guard.end());
            par->Parse();
            auto root =  par->getRoot();
            this->export_defs(par);
            include_guard.insert(par->include_guard.begin(), par->include_guard.end());
            delete par;
            lex->getNextTok();
            return root;
        }
        
        ast_t* ParsePrint(){
            lex->getNextTok();
            auto body = ParseExpression();
            if(!body)
                return nullptr;
            return new PrintExpression(body);
        }
        ast_t* ParseReturn(){
            if(lex->getNextTokV() == splitter)
                return new ReturnExpression(nullptr);
            auto val = ParseExpression();
            if(!val)
                return nullptr;
            if(lex->getLastTokV() == splitter)
                lex->getNextTok();
            return new ReturnExpression(val);
        }
        ast_t* ParseLambdaFunc(bool params_expected = true){
            lex->getNextTok();
            std::vector<std::string> args;
            while(true && params_expected){
                if(lex->getLastTokV() == ident_tok){
                    args.push_back(lex->getLastStr());
                    if(lex->getNextTokV() == comma)
                        lex->getNextTok();
                    continue;
                }
                if(lex->getLastStr() == "|"){
                    lex->getNextTok();
                    break;
                }
                return Error("'|' or identifer expected in lambda expression,but found:",lex->getLastStr());
            }
            auto expr = ParseExpression();
            if(!expr){
                return nullptr;
            }
            return new FuncDefinition("",args,expr,true,true);
        }
        ast_t* ParsePrimary(){
            if(failed)
                return nullptr;
            auto _tok = lex->getLastTok();
            ast_t* res = nullptr;
            switch (_tok.tok){
                case fun_tok:   res =  ParseFuncDef(/*!@p anon:*/true);break;
                case ident_tok: res =  ParseIdentifer();break;
                case open_bl:   res =  ParseHTable();break;
                case open_br:   res =  ParseParen();break;
                case open_sq:   res =  ParseArray();break;
                case null_tok:
                case num_tok:
                case str_tok:
                case bool_tok:  res =  ParseNumber();break;
                case defined_tok: {
                    if(lex->getNextTokV()!=open_br){
                        return Error("( expected,found:",lex->getLastStr());
                    }
                    if(lex->is_def(lex->getNextStr(false))){
                        res = new ConstantExpression("true", Type::boolean);
                    }else
                        res = new ConstantExpression("false", Type::boolean);
                    if(lex->getNextTokV()!=close_br){
                        delete res;
                        return Error(") expected,found:",lex->getLastStr());
                    }
                    lex->getNextTokV();
                }break;
                case oper_tok:{
                    std::string op = lex->getLastStr();
                    if(op == "|" || op == "||")
                        res =   ParseLambdaFunc(op!="||");
                    else if(op == "-"|| op == "+" || is_unary(op))
                        res =  ParseUnary();
                    else
                        res =  Error("Unary|Primary excepted, but found: ",op);
                }break;
                default:  res =  Error("Primary expected, but found: ",_tok.str);break;
            }
            if(!res){
                lex->setError();
                return nullptr;
            }
            auto &tk = lex->getLastStr();
            if(tk=="++" || tk == "--"){
                res = new UnaryExpression(match_oper(tk), res, false);
                lex->getNextTokV();
            }
            return res;
        }
        ast_t* ParseRaise(){
            lex->getNextTok();
            auto val = ParseExpression();
            if(!val)
                return nullptr;
            if(lex->getLastTokV()!=if_tok){
                delete val;
                return Error("'If' expected in raise expression,found:",lex->getLastStr());
            }
            lex->getNextTok();
            auto cond = ParseExpression();
            if(!cond){
                delete val;
                return nullptr;
            }
            return new RaiseExpression(val,cond);
        }
        ast_t* ParseTry(){
            lex->getNextTok();
            auto tblock = ParseBlock();
            if(!tblock)
                return nullptr;
            if(lex->getLastTokV()!=catch_tok){
                delete tblock;
                return Error("'catch' expected,found:",lex->getLastStr());
            }
            if(lex->getNextTokV()!=open_br){
                delete tblock;
                return Error("'(' expected as catch agrument, found:",lex->getLastStr());
            }
            auto id = lex->getNextTok();
            if(id.tok!=ident_tok){
                delete tblock;
                return Error("Identifer expected as catch argument,found:",lex->getLastStr());
            }
            if(lex->getNextTokV()!=close_br){
                delete tblock;
                return Error("')'Expected as end of catch arg,found:",lex->getLastStr());
            }
            lex->getNextTok();
            auto cblock = ParseBlock();
            if(!cblock){
                delete tblock;
                return nullptr;
            }
            if(lex->getLastTokV()!=finally_tok)
                return new TryExpression(tblock,cblock,id.str,nullptr);
            lex->getNextTok();
            auto fblock = ParseBlock();
            if(!fblock){
                delete tblock;
                delete cblock;
                return nullptr;
                
            }
            return new TryExpression(tblock,cblock,id.str,fblock);
            
        }
        ast_t* ParseThrow(){
            lex->getNextTok();
            auto val = ParseExpression();
            if(!val)
                return nullptr;
            return new ThrowExpression(val);
        }
        bool is_user_op(std::string&k){
            return is_user_operator.find(k)!=is_user_operator.end();
        }
        ast_t *ParseRest(int exprPrecedence, ast_t *left) {
            if(failed){
                delete left;
                return nullptr;
            }
            if(is_right_assoc(lex->getLastStr())){
                auto operstr = lex->getLastStr();
                lex->getNextTok();
                auto right = ParseExpression();
                if(!right){
                    delete left;
                    return nullptr;
                }
                if(is_op_and_assign(operstr)){
                    auto op = match_oper(operstr);
                    return new BinaryExpression(match_oper(operstr="="), left,
                                                new BinaryExpression(op,left, right,true));
                }
                auto oper = match_oper(operstr);
                return  new BinaryExpression(oper,left,right);
            }
            while (true) {
                std::string __oper = lex->getLastStr();
                int tokPrecedence = OpPrecedence[__oper];
                tokPrecedence = (tokPrecedence && !is_unary(__oper))?tokPrecedence:-1;
                if (tokPrecedence < exprPrecedence)
                    return left;
                auto oper = match_oper(__oper);
                if(lex->getLastTokV() == qu_mark){
                    lex->getNextTok();
                    auto _iftrue = ParseExpression();
                    if(!_iftrue){
                        delete left;
                        return nullptr;
                    }
                    if(lex->getLastTokV()!=doubledot){
                        delete _iftrue;
                        delete left;
                        return Error("':' expected in ternar if expression,but found:",lex->getLastStr());
                    }
                    lex->getNextTok();
                    auto _iffalse = ParseExpression();
                    if(!_iffalse){
                        delete _iftrue;
                        delete left;
                        return nullptr;
                    }
                    left = new IfExpression(left,_iftrue,_iffalse,true);
                    continue;
                }
                ast_t *right ;
                if(is_user_op(__oper)){
                    lex->getNextTokV();
                    right = ParsePrimary();
                    if(!right){
                        delete left;
                        return nullptr;
                    }
                    if(is_user_op(lex->getLastStr()) ||  match_oper(lex->getLastStr())){
                        right = ParseRest(OpPrecedence[__oper],right);
                        if(!right)
                            return nullptr;
                    }
                    //! TODO PRECEDENCE
                    left = new FuncCallExpression(
                                                  new Variable(__oper + "$operator"), {left,right});
                    continue;
                }else{
                    if(oper == IL::getProperty){
                        bool is_objlike = lex->getLastTokV() == dot;
                        if(is_objlike){
                            auto ident = lex->getNextTok();
                            if(ident.tok!=ident_tok){
                                delete left;
                                return Error("Identifer expected, but found: ",ident.str);
                            }
                            left = new PropertyExpression(left,
                                                          new ConstantExpression(ident.str,Type::str),false);
                            lex->getNextTok();
                        }else{
                            if(lex->getLastTokV()!=open_sq){
                                delete left;
                                return Error("'[' Expected, but found ",lex->getLastStr());//никогда не будет :)
                            }
                            lex->getNextTok();
                            auto id = ParseExpression();
                            if(!id){
                                delete left;
                                return nullptr;
                            }
                            if(lex->getLastTokV()!=close_sq){
                                delete left;
                                delete id;
                                return Error("']' exptected,but found: ",lex->getLastStr());
                            }
                            lex->getNextTok();
                            left = new PropertyExpression(left,id,false);
                        }
                        auto &tk = lex->getLastStr();
                        if(tk=="++" || tk =="--"){
                            left = new UnaryExpression(match_oper(tk), left, false);
                            lex->getNextTokV();
                        }
                        continue;
                    }else if (oper == IL::call){
                        lex->getNextTok();
                        auto _v = ParseArgs(true);
                        if(failed || lex->getLastTokV()!=close_br){
                            delete left;
                            return Error("Funcall args expected,found:",lex->getLastStr());
                        }
                        lex->getNextTok();
                        left = new FuncCallExpression(left,_v);
                        continue;
                    }else{
                        lex->getNextTok();
                        right = ParsePrimary();
                        if (right == nullptr){
                            delete left;
                            return nullptr;
                        }
                    }
                }
                int nextPrecedence = OpPrecedence[lex->getLastStr()];
                nextPrecedence= (nextPrecedence&& !is_unary(lex->getLastStr()))?nextPrecedence:-1;
                if (tokPrecedence < nextPrecedence) {
                    right = ParseRest(tokPrecedence/*+1*/, right);
                    if (right == nullptr){
                        delete left;
                        return nullptr;
                    }
                }
                left=  new BinaryExpression(oper,left,right);
            }
        }
        ast_t* ParseVarDecl(bool is_const = false){
            lex->getNextTok(); // eat var
            auto root = new BlockExpression(true);
            while(true){
                auto name = lex->getLastTok();
                if(name.tok!=ident_tok){
                    delete root;
                    return Error("Identifer expected in var decl expression, found:",name.str);
                }
                if(lex->getNextStr() == "="){
                    lex->getNextTok();
                    auto expr = ParseExpression();
                    if(!expr){
                        delete root;
                        return nullptr;
                    }
                    root = new BlockExpression(root,
                                               new VarDeclareExpression(name.str,expr,is_const),true);
                }else if(!is_const){
                    root = new BlockExpression(root,
                                               new VarDeclareExpression(name.str,nullptr),true);
                }else{
                    delete root;
                    return Error("Const value expected! found:",lex->getLastStr());
                }
                if(lex->getLastTokV()!=comma)
                    break;
                lex->getNextTok();
            }
            return root;
        }
        ast_t* ParseBlock(){
            lex->getNextTok();
            BlockExpression* res = new BlockExpression();
            while(lex->getLastTokV()!= close_bl){
                auto tmp = ParseUpExpr();
                if(failed || !tmp){
                    delete res;
                    return nullptr;
                }
                res = new BlockExpression(res,tmp);
            }
            if(lex->getLastTokV()!=close_bl){
                return Error("'{' expected, but found:",lex->getLastStr());
            }
            lex->getNextTok();
            return res;
        }
        ast_t* ParseForLoop(){
            if(lex->getNextTokV() != open_br)
                return Error("'(' expected, but found: ",lex->getLastStr());
            lex->getNextTok(); // eat '('
            ast_t *cond,
            *step,
            *vardecl;
            if(lex->getLastTokV() == splitter){
                vardecl = new BlockExpression(); // hack
            }else{
                vardecl = ParseVarDecl();
                if(!vardecl){
                    return nullptr;
                }
                if(lex->getLastTokV() == in_tok){
                    lex->getNextTok();
                    ast_t* obj = ParseExpression();
                    if(!obj){
                        delete vardecl;
                        return nullptr;
                    }
                    if(lex->getLastTokV()!=close_br){
                        delete obj;
                        delete vardecl;
                        return Error("'(' expected in for-in-loop,but found:",lex->getLastStr());
                    }
                    lex->getNextTok();
                    auto body = ParseUpExpr();
                    if(!body){
                        delete vardecl;
                        delete obj;
                        return nullptr;
                    }
                    return new ForInExpression(vardecl,obj,body);
                }
            }
            if(lex->getLastTokV() == splitter)
                lex->getNextTok();
            if(lex->getLastTokV() == splitter){
                cond = new ConstantExpression("true", Type::boolean);
            }else{
                cond = ParseExpression();
                if(!cond){
                    delete vardecl;
                    return nullptr;
                }
            }
            if(lex->getLastTokV() == splitter)
                lex->getNextTok();
            if(lex->getLastTokV() == close_br){
                step = new BlockExpression();
            }else{
                step = ParseExpression();
                if(!step){
                    delete vardecl;
                    delete cond;
                    return nullptr;
                }
            }
            if(lex->getLastTokV()!= close_br){
                delete cond;
                delete step;
                delete vardecl;
                return Error("')'Expected,but found: ",lex->getLastStr());
            }
            lex->getNextTok();
            auto body = ParseUpExpr();
            if(!body){
                delete cond;
                delete step;
                delete vardecl;
                return nullptr;
            }
            return new ForExpression(vardecl,cond,step,body);
        }
        template<typename T> static inline auto __freeTable(T&t)->void{
            for(auto&x:t)
                delete x.second;
        }
        ast_t * ParseHTable(){
            lex->getNextTok();
            std::unordered_map<std::string, ast_t*> ht;
            while(true){
                if(lex->getLastTokV() == close_bl){
                    lex->getNextTok(); //eat }
                    return new HTableExpression(ht);
                }
                auto ind = lex->getLastStr();
                if(lex->getNextTokV() != doubledot && lex->getLastStr()!="="){
                    __freeTable(ht);
                    return Error("':'|'=' Expected, but found:",lex->getLastStr());
                } // eat :
                lex->getNextTok();
                ast_t* _tmp = ParseExpression();
                if(_tmp == nullptr){
                    __freeTable(ht);
                    return nullptr;
                }
                ht[ind]=_tmp;
                if(lex->getLastTokV() == comma || lex->getLastTokV() == splitter)
                    lex->getNextTok();
            }
        }
        ast_t* ParseOperator(){
            auto what = lex->getNextStr();
            auto precedence  = lex->getNextTok();
            if(precedence.tok!=num_tok){
                return Error("Precedence expected,found:",precedence.str);
            }
            auto prec = std::strtoll(precedence.str.c_str(),0,0);
            if(prec<=0 || prec>=255){
                return Error("Too big/zero precedence,found:",precedence.str);
            }
            auto func = ParseFuncDef(true);
            if(!func){
                return nullptr;
            }
            auto argss = dynamic_cast<FuncDefinition*>(func)->args.size();
            if(argss!=2 && argss!=1){
                delete func;
                return Error("1 or 2 args of operator expected,found:","more then 1 or 2");
            }
            OpPrecedence[what]=prec;
            createOperator(what, prec,argss==1);
            return new VarDeclareExpression(oper_func[what], func,true);
        }
        ast_t* ParseFuncDef(bool anon, bool class_method = false){
            std::string name ;
            if(!anon)
                name = lex->getNextStr();
            if(!anon&&lex->getLastTokV() != ident_tok)
                return Error("Func name expected, but found:",lex->getLastStr());
            if(lex->getNextTokV() == doubledot && class_method){
                auto _sub = lex->getNextTok();
                if(_sub.tok!=ident_tok){
                    return Error("Sub identifer expected in special form of class method,found:",_sub.str);
                }
                name = name + ":" + _sub.str;
                lex->getNextTokV();
            }
            if(lex->getLastTokV() != open_br){
                return Error("'(' expected at func definition, but found:",lex->getLastStr());
            }
            lex->getNextTok(); //eat (
            std::vector<std::string> args;
            bool packed = false;
            while(true){
                if(lex->getLastTokV() == close_br)
                    break;
                if(lex->getLastTokV() != ident_tok)
                    return Error("Arg name(identifer) expected, but found:",lex->getLastStr());
                args.push_back(lex->getLastStr());
                if(lex->getNextTokV()==pack_tok){
                    packed = true;
                    if(lex->getNextTokV()!=close_br){
                        return Error(") expected after of packed argument,found:",lex->getLastStr());
                    }
                    break;
                }
                if(lex->getLastTokV() == comma)
                    lex->getNextTok(); //eat
            }
            lex->getNextTok() ; //eat )
            auto body = ParseBlock();
            if(!body)
                return nullptr;
            
            return new FuncDefinition(name,args,body,anon,false,packed);
        }
        ast_t* ParseExtern(){
            lex->getNextTok();
            auto ext = new BlockExpression(true);
            bool have_one = false;
            while(true){
                auto id = lex->getLastTok();
                if(id.tok!=ident_tok){
                    if(have_one)
                        break;
                    delete ext;
                    return Error("Identifers to exted expected,found:",id.str);
                }
                have_one = true;
                ext = new BlockExpression(ext,new ExternExpression(id.str),true);
                if(lex->getNextTokV()!=comma){
                    return ext;
                }
                lex->getNextTok();
            }
            return nullptr;
        }
        ast_t* ParseUpExpr(){
            if(failed)
                return nullptr;
            ast_t* res = nullptr;
            switch (lex->getLastTokV()){
                case const_tok:
                case var_tok: res= ParseVarDecl(lex->getLastTokV()==const_tok); break;
                case print_tok: res= ParsePrint();break;
                case del_tok: res= ParseDelExpr();break;
                case require_tok: res = ParseRequire();break;
                case return_tok:res= ParseReturn(); break;
                case if_tok: res = ParseIfExpr(); break;
                case open_bl: res = ParseBlock(); break;
                case for_tok: res = ParseForLoop(); break;
                case while_tok: res = ParseWhileExpr(); break;
                case fun_tok: res = ParseFuncDef(/*! @p anon:*/false); break;
                case break_tok: { lex->getNextTok(); res = new LoopControls(/*!@p is_break)*/true);}break;
                case continue_tok: { lex->getNextTok(); res = new LoopControls(/*!@p is_break)*/false);}break;
                case throw_tok: res = ParseThrow();break;
                case raise_tok: res = ParseRaise();break;
                case try_tok: res = ParseTry();break;
                case extern_tok: res = ParseExtern(); break;
                case do_tok: res = ParseWhileExpr(true); break;
                case class_tok : res = ParseClass(); break;
                case operator_tok: res = ParseOperator(); break;
                case define_tok: {
                    auto what = lex->getNextStr();
                    if(lex->getNextStr()!="="){
                        return Error("Assign expected,found:",lex->getLastStr());
                    }
                    auto def = lex->scanToEOL();
                    if(def.length()==0)
                        return Error(" Empty definition.found:",def);
                    lex->define(what, def);
                    lex->getNextTokV();
                    res=new BlockExpression();
                }break;
                case undef_tok: {
                    auto what = lex->getNextStr();
                    lex->undef(what);
                    res = new BlockExpression();
                }break;
                case alias_tok: {
                    auto what = lex->getNextStr();
                    if(lex->getNextStr()!="="){
                        return Error("Assign expected,found:",lex->getLastStr());
                    }
                    auto def = lex->getNextStr();
                    lex->alias(what, def);
                    lex->getNextTokV();
                    res=new BlockExpression();
                    
                }break;
                case macro_tok: {
                    throw eval_error("Non imnplmentat yet.");
                }break;
                default: res=  ParseExpression();break;
            }
            if(!res)
                return nullptr;
            while(lex->getLastTokV() == splitter)
                lex->getNextTok();
            return res;
        }
        ast_t* ParseWhileExpr(bool while_type= false){
            lex->getNextTok();
            if(!while_type){
                auto cond = ParseExpression();
                if(!cond)
                    return nullptr;
                auto body = ParseUpExpr();
                if(!body){
                    delete cond;
                    return nullptr;
                }
                return new WhileExpression(cond,body);
            }
            auto block = ParseUpExpr();
            if(!block)
                return nullptr;
            if(lex->getLastTokV()!=while_tok){
                delete block;
                return Error("While expected, found:",lex->getLastStr());
            }
            lex->getNextTokV();
            auto cond = ParseExpression();
            if(!cond){
                delete block;
                return nullptr;
            }
            return new PostWhileExpression(cond, block);
        }
        ast_t* ParseIfExpr(){
            lex->getNextTok(); // eat if
            auto cond = ParseExpression();
            if(!cond)
                return nullptr;
            auto _then = ParseUpExpr();
            if(!_then){
                delete cond;
                return nullptr;
            }
            ast_t* _else = nullptr;
            if(lex->getLastTokV() == else_tok){
                lex->getNextTok();
                _else = ParseUpExpr();
                if(!_else){
                    delete cond;
                    delete _then;
                    return nullptr;
                }
            }
            return new IfExpression(cond,_then,_else);
        }
        bool do_not_del_root;
    public:
        Parser(Lexer *lex,std::string path="undefined",bool do_not_del_root = false):lex(lex),__path(path),do_not_del_root(do_not_del_root)
        {init();root = nullptr;}
        ~Parser(){
            delete lex;
            if(root && !do_not_del_root)
                delete root;
        }
        ast_t* ParseObj(){
            ast_t* res = nullptr;
            switch (lex->getNextTokV()){
                case open_bl:res =  ParseHTable(); break;
                case open_sq:res =  ParseArray(); break;
                default: throw eval_error("Object|array expected\n");
            }
            if(!res)
                throw eval_error(__error_msg);
            return res;
        }
        ast_t *getRoot(){
            return root;
        }
        ast_t* ParseExpr(){
            lex->getNextTok();
            auto expr = ParseExpression();
            if(!expr)
                return nullptr;
            return new BlockExpression(expr,true);
        }
        void define(std::string &x,std::string &y){
            lex->define(x, y);
        }
        void alias(std::string &x,std::string&y){
            lex->alias(x, y);
        }
        bool undef(std::string&x){
            return lex->undef(x);
        }
        bool defined(std::string&x){
            return lex->is_def(x);
        }
        void createOperator(std::string &kwd,uint8_t precedence, bool unary = false){
            OpPrecedence[kwd]=precedence;
            lex->setOperator(kwd);
            oper_func[kwd]= kwd + "$operator";
            is_user_operator[kwd]=true;
            if(unary)
                user_unary_oper[kwd]=true;
        }
        Lexer* getLex(){
            return lex;
        }
        void Parse(){
            root = new BlockExpression(true);
            lex->getNextTok();
            ast_t* expr;
            while(!failed && lex->getLastTokV()!= eof_tok && lex->getLastTokV()!=error_tok){
                expr = ParseUpExpr();
                if(!expr || failed){
                    delete root;
                    assert(failed);
                    lex->dumpAliasesAndDefs();
                    throw eval_error(__error_msg);
                }
                if(lex->getLastTokV() == error_tok){
                    delete root;
                    lex->dumpAliasesAndDefs();
                    throw eval_error(lex->getLastStr());
                }
                root = new BlockExpression(root,expr,true);
            }
        }
    };
}
