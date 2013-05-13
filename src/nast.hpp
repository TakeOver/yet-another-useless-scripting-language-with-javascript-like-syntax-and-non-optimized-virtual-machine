#pragma once

#include <cstdint>
#include <cassert>

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <typeinfo>

#include "ntypes.hpp"
#include "nexc.hpp"
#include "ncodegen.hpp"

namespace nls{
  template<typename T> void merge(T &one, T &two){
    one.insert(one.begin(), two.begin(), two.end());
  }
  class ast_t{
  public:
    uint16_t Registry;
    virtual ~ast_t(){}
    ast_t(){}
    bool is_const = false;
    virtual void emit(BasicBlock*bb) = 0;
  };

  class BlockExpression: public ast_t{
    std::vector<ast_t*> block;
    bool global = false;
  public:
    BlockExpression(bool global = false):global(global){}
    BlockExpression(ast_t* _node, bool global = false): global(global){
      auto _b = dynamic_cast<BlockExpression*>(_node);
      if(_b!= nullptr){
	merge(block,_b->block);
	_b->block.clear();
	delete _node;
      }else{
	block.push_back(_node);
      }
    }
    BlockExpression(ast_t* _node1,ast_t* _node2, bool global = false):global(global){
      auto _b = dynamic_cast<BlockExpression*>(_node1);
      if(_b!= nullptr){
	merge(block,_b->block);
	_b->block.clear();
	delete _node1;
      }else{
	block.push_back(_node1);
      }
      block.push_back(_node2);
    }
    ~BlockExpression(){
      for(auto&x:block)
	delete x;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock* nbb;
      if(global)
	nbb = bb;
      else nbb = new BasicBlock(bb);
      for(auto&x:block){
	x->emit(nbb);
	Registry = x->Registry;
      }
      if(!global){
	nbb->MergeToGlobal();
	delete nbb;
      }
    }
  };

  class ExternExpression: public ast_t{
    std::string name;
  public:
    ExternExpression(std::string name):name(name){}
    ~ExternExpression(){}
    virtual  void emit(BasicBlock*bb){
      if(!bb->exist_var(name)){
      	Registry = bb->newVarReg(name);
      }else
      	Registry = bb->getVar(name);
    }
  };
  class PackedArg: public ast_t{
        ast_t* val;
  public:
    PackedArg(ast_t* val):val(val){}
    ~PackedArg(){
        delete val;
    }
    virtual  void emit(BasicBlock*bb){
        val->emit(bb);
        bb->emit(IL::unpackArg,val->Registry);
    }
  };
  class Variable: public ast_t{
    std::string name;
  public:
    std::string retName() const{
      return name;
    }
    Variable(std::string name):name(name){}
    ~Variable(){}
    virtual  void emit(BasicBlock*bb){
      if(!bb->exist_var(name)){
	throw eval_error("Variable undefined: "+name);
      }
      Registry = bb->getVar(name);
    }
  };
  bool is_HTE(ast_t*);
  bool is_NCE(ast_t*);
  bool is_FCE(ast_t*);
  class Expression: public ast_t{
    ast_t* val;
  public:
    Expression(ast_t* val):val(val){}
    ~Expression(){
      delete val;
    }
    virtual  void emit(BasicBlock*bb){
      val->emit(bb);
      Registry = val->Registry;
    }
    ast_t* getVal(){return val;}
    bool is_HTable(){return is_HTE(val);}
    bool is_NewClass(){return is_NCE(val);}
    bool is_FCall(){return is_FCE(val);}
  };

  class PropertyExpression: public ast_t{
  public:
    ast_t* obj;
    ast_t* ind;
    bool setter = false;
    bool _doNotOpt = false;
    PropertyExpression(ast_t* obj,ast_t* ind, bool setter):obj(obj),ind(ind),setter(setter){}
    ~PropertyExpression(){
      delete obj;
      delete ind;
    }
    void doNotOptimize(){
        _doNotOpt = true;
    }
    void emit(BasicBlock*bb){
      obj->emit(bb);
      ind->emit(bb);
      if(!_doNotOpt && dynamic_cast<PropertyExpression*>(obj))
	Registry=obj->Registry;
      else
	Registry = bb->newReg();
      bb->emit(IL::getProperty,Registry,obj->Registry,ind->Registry);
    }
  };

  class ConstantExpression: public ast_t{
    std::string value;
    Type::types type;
  public:
    ConstantExpression(std::string value, Type::types type): value(value),type(type){ is_const = true;}
    ~ConstantExpression(){}
    virtual  void emit(BasicBlock*bb){
      Registry = bb->ConstReg(value,type);
    }
    std::string  getValue()const{
        return value;
    }
  };


  class BinaryExpression: public ast_t{
    uint8_t op;
    ast_t  *lhs,
	   *rhs;
    bool delrhs,
         dellhs;
  public:
    BinaryExpression(uint8_t op, ast_t* lhs, ast_t* rhs, bool dntdellhs=false,bool dntdelrhs=false):
    op(op),lhs(lhs),rhs(rhs),delrhs(!dntdelrhs),dellhs(!dntdellhs){
      assert(lhs != nullptr);
      assert(rhs != nullptr);
    }
    ~BinaryExpression(){
        if(delrhs)
                delete rhs;
        if(dellhs)
                delete lhs;
    }
    virtual  void emit(BasicBlock*bb){
      if(op == IL::mov){
	if(dynamic_cast<ConstantExpression*>(lhs) != nullptr){
	  throw eval_error("Cannot assign to lvalue");
	}
	PropertyExpression*prop;
	if((prop=dynamic_cast<PropertyExpression*>(lhs))!=nullptr){
	  prop->obj->emit(bb);
	  prop->ind->emit(bb);
	  rhs->emit(bb);
	  bb->emit(IL::setProperty,prop->obj->Registry,prop->ind->Registry,(Registry=rhs->Registry));
	  return;
	}
	lhs->emit(bb);
	rhs->emit(bb);
	if(!bb->is_const_reg(lhs->Registry)){
          	  bb->emit(IL::mov,(Registry = lhs->Registry),rhs->Registry);
        }else std::cerr<<"Warning: Trying assign lvalue to constant\n";

	return;
      }
      lhs->emit(bb);
      rhs->emit(bb);
      if(dynamic_cast<BinaryExpression*>(lhs))
	Registry=lhs->Registry;
      else if(dynamic_cast<BinaryExpression*>(rhs))
	Registry=rhs->Registry;
      else
	Registry = bb->newReg();
      bb->emit((op==IL::concat)?IL::concat:IL::math_op,
	       (IL::op_t)(op),
	       Registry,
	       lhs->Registry,
	       rhs->Registry);
    }
  };

  class UnaryExpression: public ast_t{
    uint8_t op;
    ast_t* val;
    bool pre = false;
  public:
    UnaryExpression(uint8_t op,ast_t* val, bool pre):op(op),val(val),pre(pre){}
    ~UnaryExpression(){
      delete val;
    }
    virtual  void emit(BasicBlock*bb){
      val->emit(bb);
      if(pre == false){
        auto obj = dynamic_cast<PropertyExpression*>(val);
        auto var = dynamic_cast<Variable*>(val);
        if(!obj && !var)
                throw eval_error("Post int/dec can be applied only on rvalue");
	Registry = bb->newReg();
	bb->emit(IL::mov,Registry,val->Registry);
	bb->emit(IL::math_op,op,val->Registry,val->Registry);
        if(obj)
                bb->emit(IL::setProperty,obj->obj->Registry,obj->ind->Registry,val->Registry);
	return;
      }
      if((op == IL::Math::inc || op == IL::Math::dec) || dynamic_cast<UnaryExpression*>(val)||
	dynamic_cast<BinaryExpression*>(val))
	Registry = val->Registry;
      else
	Registry = bb->newReg();
      if(op == IL::length)
	bb->emit((IL::op_t)op,Registry,val->Registry);
      else
	bb->emit(IL::math_op,op,Registry,val->Registry);
      PropertyExpression* prop;
      if((prop=dynamic_cast<PropertyExpression*>(val))!=nullptr
	&& pre && (op==IL::Math::inc ||op==IL::Math::dec)){
	auto obj = prop->obj;
	auto ind = prop->ind;
	bb->emit(IL::setProperty,obj->Registry,ind->Registry,Registry);
      }
    }
  };

  class HTableExpression: public ast_t{
    std::unordered_map<std::string, ast_t*> ht;
  public:
    HTableExpression(std::unordered_map<std::string,ast_t*> ht):ht(ht){}
    ~HTableExpression(){
      for(auto&x:ht)
	delete x.second;
    }
    virtual  void emit(BasicBlock*bb){
      Registry = bb->newReg();
      bb->emit(IL::objAlloc,Registry);
      for(auto&x:ht){
	x.second->emit(bb);
	auto tmp = new ConstantExpression(x.first,Type::str);
	tmp->emit(bb);
	bb->emit(IL::setProperty,Registry,tmp->Registry,x.second->Registry);
	delete tmp;
      }
    }
  };
  class ArrayExpression: public ast_t{
    std::vector<ast_t*> ht;
  public:
    ArrayExpression(std::vector<ast_t*> ht):ht(ht){}
    ~ArrayExpression(){
      for(auto&x:ht)
	delete x;
    }
    virtual  void emit(BasicBlock*bb){
      Registry = bb->newReg();
      bb->emit(IL::arrAlloc,Registry);
      uint i = 0;
      for(auto&x:ht){
	std::stringstream ss;
	x->emit(bb);
	ss<<i++;
	auto tmpRegistry = bb->ConstReg(ss.str(),Type::number);
	bb->emit(IL::setProperty,Registry,tmpRegistry,x->Registry);
      }
    }
  };
      #ifdef DEBUG_INFO
  std::string resolveName(ast_t*);
  #endif
  class FuncCallExpression: public ast_t{
    ast_t * func;
    std::vector<ast_t*> args;
    bool withNew = false;
    uint16_t newreg;
    bool notdel;
  public:
    FuncCallExpression(ast_t* func, std::vector<ast_t*>args, bool notdelf = false):func(func),args(args),notdel(notdelf){}
    ~FuncCallExpression(){
      for(auto&x:args)
	delete x;
      if(!notdel)
        delete func;
    }
    void setUsingNew(uint16_t r){
      newreg =r;
      withNew = true;
    }
    virtual  void emit(BasicBlock*bb){
      auto self = dynamic_cast<PropertyExpression*>(func);
      if(self && !withNew)
        self->doNotOptimize();
      func->emit(bb);
      for(auto i = args.rbegin(), e = args.rend(); i!=e; ++i){
	(*i)->emit(bb);
        if(!dynamic_cast<PackedArg*>(*i))
	       bb->emit(IL::pushArg,(*i)->Registry);
      }
      if(self != nullptr && !withNew){
	bb->emit(IL::pushArg,self->obj->Registry); //! @a this|self
      }else{
        if(!withNew){
	       bb->emit(IL::pushArg, bb->ConstReg("null",Type::null));
        }else
	       bb->emit(IL::pushArg,newreg);
      }
      bb->emit(IL::call,func->Registry,static_cast<uint16_t>(args.size()+1)
      #ifdef DEBUG_INFO
      ,bb->ConstReg(resolveName(func),Type::str)
      #endif
        );

      bb->emit(IL::pop,Registry=bb->newReg());
    }
  };

  class NewClassExpr: public ast_t{
    ast_t* callee;
  public:
    NewClassExpr(ast_t* callee):callee(callee){Registry=0;}
    ~NewClassExpr(){
      delete callee;
    }
    virtual  void emit(BasicBlock*bb){
      auto obj = dynamic_cast<FuncCallExpression*>(callee);
      bb->emit(IL::objAlloc,(Registry=bb->newReg()));
      if(!obj){
        Variable* var;
        if((var = dynamic_cast<Variable*>(obj))!=nullptr){
                FuncCallExpression * fc = new FuncCallExpression(var, std::vector<ast_t *>());
                fc->setUsingNew(Registry);
                fc->emit(bb);
                delete fc;
                return;
        }
        if(dynamic_cast<ConstantExpression*>(obj))
	       throw eval_error("Wrong using of 'new' operator!");
        FuncCallExpression* fc = new FuncCallExpression(obj, std::vector<ast_t *> ());
        fc->setUsingNew(Registry);
        fc->emit(bb);
        delete fc;
        return;
      }
      obj->setUsingNew(Registry);
      obj->emit(bb);
    }
  };

  class VarDeclareExpression: public ast_t{
  public:
    std::string name;
    ast_t* val;
    bool is_const_decl;
    VarDeclareExpression(std::string name,ast_t*val,bool is_const_decl = false):name(name),val(val),
    is_const_decl(is_const_decl){}
    ~VarDeclareExpression(){
      if(val)
	delete val;
    }
    virtual  void emit(BasicBlock*bb){
      if(val != nullptr){
	val->emit(bb);
	auto expr = dynamic_cast<Expression*>(val);
	if(!expr ||( !expr->is_FCall() && !expr->is_HTable() && !expr->is_NewClass())){
	  Registry = bb->newVarReg(name);
	  bb->emit(IL::mov,Registry,val->Registry);
	}else
	if(expr->is_HTable())
	  Registry = dynamic_cast<HTableExpression*>(expr->getVal())->Registry;
	else if(expr->is_NewClass())
	  Registry = dynamic_cast<NewClassExpr*>(expr->getVal())->Registry;
	else if(expr->is_FCall())
	  Registry = dynamic_cast<FuncCallExpression*>(expr->getVal())->Registry;
	bb->setRegVar(Registry,name);
        if(is_const_decl)
	       bb->makeConstant(Registry);
      }else{
	Registry = bb->newVarReg(name);
      }
    }
  };

  class IfExpression: public ast_t{
    ast_t *cond,
	  *_then,
	  *_else;
    bool ternar;
  public:
    IfExpression(ast_t* cond, ast_t* _then, ast_t* _else): cond(cond), _then(_then),_else(_else){
		   Registry=0;
		   ternar=false;
    }
    IfExpression(ast_t* cond,     ast_t* _then,ast_t* _else,bool):cond(cond),_then(_then),_else(_else){
		  ternar = true;
    }
    ~IfExpression(){
      delete cond;
      delete _then;
      if(_else)
	delete _else;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock *nbb = new BasicBlock(bb);
      if(ternar)
	Registry = nbb->newReg();
      cond->emit(nbb);
      nbb->emit(IL::nop);
      auto _top = nbb->getTop();
      _then->emit(nbb);
      if(ternar)
	nbb->emit(IL::mov,Registry,_then->Registry);
      nbb->emit(IL::nop);
      auto after_top = nbb->getTop();
      nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jz,cond->Registry,nbb->createLabel()),_top);// else label
      if(_else != nullptr){
	_else->emit(nbb);
	if(ternar)
	  nbb->emit(IL::mov,Registry,_else->Registry);
	nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,
			 nbb->createLabel()),after_top); //afterif label
      }
      nbb->MergeToGlobal();
      delete nbb;
    }
  };

  class ForExpression: public ast_t{
    ast_t *decl,
	  *cond,
	  *step,
	  *body;
  public:
    ForExpression(ast_t*decl,ast_t*cond,ast_t*step,ast_t*body):decl(decl),cond(cond),step(step),
    body(body){Registry=0;}
    ~ForExpression(){
      delete decl;
      delete cond;
      delete step;
      delete body;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock *nbb = new BasicBlock(bb);
      decl->emit(nbb);
      auto cond_label = nbb->createLabel();
      cond->emit(nbb);
      nbb->emit(IL::nop);
      auto afterFor = nbb->getTop();
      body->emit(nbb);
      auto step_label = nbb->createLabel();
      step->emit(nbb);
      nbb->emit(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,cond_label);
      uint16_t last_label = nbb->createLabel();
      nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jz,cond->Registry,last_label),afterFor);
      nbb->evalLoopControls(step_label,last_label);
      nbb->MergeToGlobal();
      delete nbb;
    }
  };

  class ForInExpression: public ast_t{
    ast_t *decl,
	  *obj,
	  *body;
  public:
    ForInExpression(ast_t*decl,ast_t*obj,ast_t*body):decl(decl),obj(obj),body(body){Registry=0;}
    ~ForInExpression(){
      delete decl;
      delete obj;
      delete body;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock *nbb = new BasicBlock(bb);
      decl->emit(nbb);
      obj->emit(nbb);

      auto _iter_reg = nbb->getVar("__iter");

      nbb->emit(IL::pushArg,obj->Registry);
      nbb->emit(IL::pushArg,nbb->ConstReg("null",0));
      nbb->emit(IL::call,_iter_reg,2);
      nbb->emit(IL::pop,(_iter_reg=decl->Registry));
      auto cond_label = nbb->createLabel();
      nbb->emit(IL::nop);
      auto afterFor = nbb->getTop();
      body->emit(nbb);
      auto step_label = nbb->createLabel();
      nbb->emit(IL::pushArg,_iter_reg);
      nbb->emit(IL::call,_iter_reg,1);
      nbb->emit(IL::pop,_iter_reg);
      nbb->emit(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,cond_label);
      auto last_label = nbb->createLabel();
      nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jz,_iter_reg,last_label),afterFor);
      nbb->evalLoopControls(step_label,last_label);
      nbb->MergeToGlobal();
      delete nbb;
    }
  };

  class WhileExpression: public ast_t{
    ast_t *cond,
	  *body;
  public:
    WhileExpression(ast_t* cond, ast_t*body):cond(cond),body(body){Registry=0;}
    ~WhileExpression(){
      delete cond;
      delete body;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock *nbb = new BasicBlock(bb);
      auto cond_label = nbb->createLabel();
      cond->emit(nbb);
      nbb->emit(IL::nop);
      auto afterWhile = nbb->getTop();
      body->emit(nbb);
      nbb->emit(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,cond_label);
      auto last_label = nbb->createLabel();
      nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jz,cond->Registry,last_label),afterWhile);
      nbb->evalLoopControls(cond_label,last_label);
      nbb->MergeToGlobal();
      delete nbb;
    }
  };
  class PostWhileExpression: public ast_t{
    ast_t *cond,
          *body;
  public:
    PostWhileExpression(ast_t* cond, ast_t*body):cond(cond),body(body){Registry=0;}
    ~PostWhileExpression(){
      delete cond;
      delete body;
    }
    virtual  void emit(BasicBlock*bb){
      BasicBlock *nbb = new BasicBlock(bb);
      auto begin = nbb->createLabel();
      body->emit(nbb);
      auto cond_label = nbb->createLabel();
      cond->emit(nbb);
      nbb->emit(IL::jcc,(uint8_t)IL::Jump::jnz,cond->Registry,(uint16_t)begin);
      auto last_label = nbb->createLabel();
      nbb->evalLoopControls(cond_label,last_label);
      nbb->MergeToGlobal();
      delete nbb;
    }
  };
  class ReturnExpression: public ast_t{
    ast_t* val;
  public:
    ReturnExpression(ast_t*val):val(val){}
    ~ReturnExpression(){
      if(val)
	delete val;
    }
    virtual  void emit(BasicBlock*bb){
      if(val== nullptr)
	bb->emit(IL::ret,bb->ConstReg("null",Type::null),bb->getVar("this"));
      else{
	BasicBlock *nbb = new BasicBlock(bb);
	val->emit(nbb);
	nbb->emit(IL::ret,val->Registry,bb->getVar("this"));
	nbb->MergeToGlobal();
	delete nbb;
      }
    }
  };

  class FuncDefinition: public ast_t{
  public:
    std::string name;
    std::vector< std::string > args;
    ast_t *body;
    bool anon,lambda,packed;
    FuncDefinition(std::string name, std::vector< std::string > args,ast_t*body, bool anon, bool lambda = false,bool packed = false)
    :name(name),args(args),body(body),anon(anon),lambda(lambda),packed(packed){}
    ~FuncDefinition(){
      delete body;
    }
    virtual  void emit(BasicBlock*bb){

      bb->emit(IL::nop);
      auto create = bb->getTop();
      bb->emit(IL::nop);
      auto start = bb->getTop();
      auto funcOffs = bb->createLabel();

      pcode _funcdecl (IL::nop);
      if(anon){
	Registry = bb->newReg();
	_funcdecl = pcode(IL::createFunc,Registry,funcOffs);
      }else{
	auto _var = new VarDeclareExpression(name,nullptr);
	_var->emit(bb);
	_funcdecl = pcode(IL::createFunc,(Registry=_var->Registry),funcOffs);
	delete _var;
      }
      bb->SetAt(_funcdecl,create);
      auto newbb = new BasicBlock(bb);

      newbb->setRegVar(Registry,"_F");
      newbb->emit(IL::getArg,newbb->newVarReg("this"));

      newbb->emit(IL::createArgsArray,newbb->newVarReg("arguments"));

      for(auto&x:args){
	newbb->emit(IL::getArg,newbb->newVarReg(x));
      }
      if(packed){
        uint idx = newbb->getTop();
        newbb->SetAt({IL::packArg,newbb->getVar(args.back())},idx);
      }

      newbb->emit(IL::clearArgs);

      body->emit(newbb);

      if(dynamic_cast<Expression*>(body)!=nullptr || dynamic_cast<NewClassExpr*>(body) || lambda){
	newbb->emit(IL::ret,body->Registry,newbb->getVar("this"));
      }

      if(newbb->getTopPtr()->op!=IL::ret)
	newbb->emit(IL::ret, newbb->ConstReg("null",Type::null),newbb->getVar("this"));
      newbb->MergeToGlobal();
      delete newbb;
      bb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,bb->createLabel()),start);
    }
  };

  class PrintExpression: public ast_t{
    ast_t* arg;
  public:
    PrintExpression(ast_t* arg):arg(arg){}
    ~PrintExpression(){
      delete arg;
    }
    virtual  void emit(BasicBlock*bb){
      arg->emit(bb);
      bb->emit(IL::print,arg->Registry);
    }
  };

  class DelExpression: public ast_t{
    PropertyExpression* arg;
  public:
    DelExpression(PropertyExpression* arg):arg(arg){}
    ~DelExpression(){
      delete arg;
    }
    virtual  void emit(BasicBlock*bb){
      arg->obj->emit(bb);
      arg->ind->emit(bb);
      bb->emit(IL::del,arg->obj->Registry,arg->ind->Registry);
    }
  };

  class LoopControls: public ast_t{
    bool is_break;
  public:
    LoopControls(bool is_break = true):is_break(is_break){}
    ~LoopControls(){}
    virtual  void emit(BasicBlock*bb){
      bb->emit(is_break?IL::brk:IL::cnt);
    }
  };

  class RaiseExpression: public ast_t{
    ast_t* value, *cond;
  public:
    RaiseExpression(ast_t*value,ast_t* cond):value(value),cond(cond){}
    ~RaiseExpression(){
      delete value;
      delete cond;
    }
    virtual  void emit(BasicBlock * bb){
      cond->emit(bb);
      bb->emit(IL::nop);
      auto afterraise = bb->getTop();
      value->emit(bb);
      bb->emit(IL::throw_ex,value->Registry);
      bb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jz,cond->Registry,bb->createLabel()),afterraise);
    }
  };

  class TryExpression: public ast_t{
  ast_t* try_bb,*catch_bb,*finally_bb;
  std::string catch_arg;
  public:
    TryExpression(ast_t*try_bb,
		  ast_t*catch_bb,
		  std::string catch_arg,
		  ast_t*finally_bb):try_bb(try_bb),catch_bb(catch_bb),finally_bb(finally_bb),
		  catch_arg(catch_arg){}
    ~TryExpression(){
      delete try_bb;
      delete catch_bb;
      if(finally_bb)
	delete finally_bb;
    }
    virtual  void emit(BasicBlock * bb){
      BasicBlock* nbb = new BasicBlock(bb);
      nbb->emit(IL::nop);
      auto jump = nbb->getTop();
      try_bb->emit(nbb);
      nbb->emit(IL::pop_try);
      nbb->emit(IL::nop);
      auto aftercatch = nbb->getTop();
      nbb->SetAt(pcode(IL::set_try,(uint16_t)0,nbb->createLabel()),jump);
      nbb->emit(IL::pop,nbb->newVarReg(catch_arg));
      catch_bb->emit(nbb);
      nbb->SetAt(pcode(IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,nbb->createLabel()),aftercatch);
      if(finally_bb)
	finally_bb->emit(nbb);
      nbb->MergeToGlobal();
      delete nbb;
    }
  };

  class ThrowExpression: public ast_t{
    ast_t* val;
  public:
    ThrowExpression(ast_t* val):val(val){}
    ~ThrowExpression(){
      delete val;
    }
    virtual  void emit(BasicBlock * bb){
      val->emit(bb);
      bb->emit(IL::throw_ex,val->Registry);
    }
  };

      #ifdef DEBUG_INFO
  inline std::string resolveStr(ast_t*t){
        if(dynamic_cast<ConstantExpression*>(t)){
                return "."+static_cast<ConstantExpression*>(t)->getValue();
        }
        return "[expr]";
  }
  inline std::string resolveName(ast_t*t){
        FuncDefinition * fd;
        Variable * v;
        PropertyExpression * pe;
        std::string res = "[expr]";
        if((fd=dynamic_cast<FuncDefinition*>(t))){
                res = fd->name;
        }else if((v=dynamic_cast<Variable*>(t))){
                res = v->retName();
        }else if((pe = dynamic_cast<PropertyExpression*>(t))){
                res = resolveName(pe->obj) + resolveStr(pe->ind);
        }
        return res;
  }
  #endif
  inline bool is_NCE(ast_t* ast){return dynamic_cast<NewClassExpr*>(ast)!=nullptr;}
  inline bool is_HTE(ast_t* ast){return dynamic_cast<HTableExpression*>(ast)!=nullptr;}
  inline bool is_FCE(ast_t* ast){return dynamic_cast<FuncCallExpression*>(ast)!=nullptr;}
}
