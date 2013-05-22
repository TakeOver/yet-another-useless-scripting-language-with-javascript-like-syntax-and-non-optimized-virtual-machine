#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <map>
#include <set>
namespace nls{
  namespace IL{
    enum op_t{
      //!*@param 0*/
      hlt = 0, nop,
      dup,
      pushNewObj, //* uses as ... new ObjName(); this == new object*/
      clearArgs, //! @param  number of getted args.
      //!*@param 1*/
      push, pop, top, //* push: rX | mem */
      createArgsArray,
      packArg,
      unpackArg,
      pushArg,getArg,
      set_try, throw_ex,pop_try,
      label, /*uint32_t label num; */
      //!@param rX:src|dest*/
      loadConstcc,  //* num /bool/str*/
      del,
      objAlloc, //!*@param rX:dest */
      arrAlloc, //!*@param rX:dest */
      print, ///! @param rX:register of any value
      //!*@param 2*/
       length,
      mov,//!*@param x:dest=y:src*/
      tostr, //! @param rX:dest<-rX:src
      call,//! @param rX:result|int:number-of-args
      ret, /*!@param rX:res*/ /* void func ret-s null*/
      createFunc,
      jcc, //!*@param rX:bool|offset*/
      //!*@param 3*/
      math_op, //* add, sub, div,mul,mod,lshift,rshift,,  */
      concat, //! @param rX:dest|rX:src1|rX:src2 @if src1|src2!=str->cast-tostr
      setProperty, //!*@param objDest|str:property|obj:Value*/
      getProperty //!*@param rX:dest|obj:src|str:property*/
      ,brk,cnt //! \brief code gen only!
      //!* @c CFG-only */
    };
    bool breaksCFBlock(enum op_t op){
      return (op == ret || op == jcc || op == hlt);
    }
    namespace Jump{
      enum{
	jmp= 0,jnz,jz
      };
    }
    namespace Math{
      enum{
	add = (label+30),
	mul,
	sub,
	div,
	mod,
	log_or,
	log_and,
	log_lt,
	log_le,
	log_eq,
	log_not,
	neg,
	inc,
	dec,
	log_ne,
	log_ge,
	log_gt,
      };
    }
    namespace ConstTypes{
      enum{
	null = 0 ,boolean,number,string
      };
    }
  }
  /** @c pcode is basic structer that holds VirtualMachine Instructions in @c 8-byte format
   * currently @c 10-byte format, but this is for @c compiler-only.VM _WOULDN'T_ have extra 2 bytes.
   * constants would be evalueated at vm start and then casted to 8 byte structer pointer after
   * @note mmap byte-code file( VirtualAlloc)*/
  typedef struct pcode{
    /** @var op - code of basic instruction in vm
     *  @var subop - is used in instructions like math_op and using only as label in it.
     *  @var dest - number of destination register
     *  @var srcX - source register
     *  @var f|i|c - is used only in constant pool
     * */
    IL::op_t  op ;
    uint8_t subop;
    uint16_t dest;
    union{
      struct{
	uint16_t src1,
		 src2;
      };
      int32_t offset;
      int32_t i;
    };
    pcode(IL::op_t op):op(op),subop(IL::hlt),dest(0),i(0){}
    pcode(IL::op_t op, uint8_t subop):op(op),subop(IL::hlt),dest(0),i(0){}
    pcode(IL::op_t op, uint16_t dest):op(op),subop(IL::hlt),dest(dest),i(0){}
    pcode(IL::op_t op, uint8_t subop,uint16_t dest):op(op),subop(subop),dest(dest),i(0){}
    pcode(IL::op_t op, uint16_t dest, uint16_t src1):op(op),subop(IL::hlt),dest(dest),src1(src1),
    src2(0){}
    pcode(IL::op_t op,uint8_t subop,uint16_t dest,uint16_t src1):
    op(op),subop(subop),dest(dest),src1(src1),src2(0){}
    pcode(IL::op_t op, uint16_t dest, uint16_t src1, uint16_t src2)
    :op(op),subop(IL::hlt),dest(dest),src1(src1),src2(src2){}
    pcode(IL::op_t op, uint8_t subop,uint16_t dest, uint16_t src1, uint16_t src2)
    :op(op),subop(subop),dest(dest),src1(src1),src2(src2){}
    pcode():op(IL::hlt),subop(0),dest(0),i(0){}
    ~pcode(){}
  }pcode;

  typedef struct constpcode{
    IL::op_t  op;
    uint8_t subop;
    uint16_t dest;
    union{
      long double f;
      int64_t i;
      char *c;
    };
    ~constpcode(){}
    constpcode():op(IL::hlt),subop(0),dest(0),i(0){}
    constpcode(IL::op_t op,uint8_t subop,uint16_t dest,long double f):op(op),subop(subop),dest(dest),f(f){}
    constpcode(IL::op_t op, uint8_t subop, uint16_t dest,int64_t i):op(op),subop(subop),dest(dest),i(i){}
    constpcode(IL::op_t op, uint8_t subop,uint16_t dest,char*c):op(op),subop(subop),dest(dest),i(0){
      this->c=c;
    }
  }constpcode;
  /** @class  BasicBlock is function-pcode holder and optimizer
   */
  struct RefHolder{
    std::vector<char*> ptr;
    void add(char*ptr){
      this->ptr.push_back(ptr);
    }
    void deleteAll(){
      for(auto&x:ptr)
	delete []x;
      ptr.clear();
    }
    RefHolder(){}
    ~RefHolder(){deleteAll();}

  };
#define op_list std::vector<pcode>
  class BasicBlock{
      op_list code;
      std::vector<constpcode> const_pool;
      uint16_t registers_count = 0;
      std::unordered_map<std::string, uint16_t> locals;
      BasicBlock* global_scope;
      std::unordered_map<std::string, uint16_t> constants_map;
      uint16_t labels = 0;
      std::map<uint,uint> LabelsOffsets;
      std::map<uint16_t,bool> constant_var;
      RefHolder* rh;
      std::vector<std::pair<std::string,uint16_t> > meta_variables_info;
      std::set<uint16_t> roots;
  public:
        inline uint16_t getRootsSize(){
                return roots.size();
        }
        inline std::vector<uint16_t> makeRootArray(){
                std::vector<uint16_t> res;
                for(auto&x:roots)
                        res.push_back(x);
                return res;
        }
        // ptr to seriallized info. + sizeof info.
        std::vector<uint16_t> SeriallizeMetaInfo(){
                std::vector<uint16_t> res;
                for(auto&x:meta_variables_info){
                        res.push_back(x.second);
                        res.push_back(ConstReg(x.first,3));
                }
                return res;
        }
        decltype(meta_variables_info)& getMetaVarInfo(){
                return meta_variables_info;
        }
    BasicBlock* get_global(){
      return global_scope;
    }
    void CacheLabels(){
      uint offs = 0;
      for(auto&x:code){
	if(x.op == IL::label)
	  LabelsOffsets[x.dest] = offs;
	++offs;
      }
    }
    void EvalOffsets(){
      for(auto&x:code){
	if(x.op == IL::jcc || x.op == IL::createFunc || x.op==IL::set_try)
	  x.offset = LabelsOffsets[x.src1];
      }
    }
    void ReplaceLabels(){
      uint offs = 0;
      decltype(code) _code;
      for(auto&x:code){
	if(x.op == IL::label)
	  LabelsOffsets[x.dest] = offs-1;
	else{
          if(x.op==IL::objAlloc || x.op == IL::arrAlloc || x.op==IL::createFunc)
                roots.insert(x.dest);
	  _code.push_back(x);
	  ++offs;
	}
      }
      code = _code;
      EvalOffsets();
    }
    void MergeToGlobal(){
      assert(global_scope!=nullptr);
      global_scope->code.insert(
				global_scope->code.end(),
				this->code.begin(),
				this->code.end());
      global_scope->const_pool.insert(
				global_scope->const_pool.end(),
				this->const_pool.begin(),
				this->const_pool.end());
      global_scope->labels=this->labels;
      global_scope->roots.insert(this->roots.begin(), this->roots.end());
      global_scope->registers_count=this->registers_count;
      for(auto&x:constants_map)
	global_scope->constants_map[x.first] = x.second;
    }
    void RegistryVariable(std::string name,uint16_t reg){
      locals[name]=reg;
    }
    decltype(registers_count) getRegCount(){
      return registers_count;
    }
    uint16_t first_reg(){
      if(!global_scope)
	return 1;
      return global_scope->last_reg()+1;
    }
    uint16_t last_reg(){
      return registers_count;
    }
    BasicBlock(BasicBlock* global_scope):
		registers_count(global_scope->registers_count),
		global_scope(global_scope),
		labels(global_scope->labels),
		constant_var(global_scope->constant_var),
                roots(global_scope->roots){
		  rh=global_scope->rh;
		}
    BasicBlock(RefHolder* rh):registers_count(0),global_scope(nullptr),labels(0),rh(rh){
      newVarReg("System");
    }
    ~BasicBlock(){}
    void setRegVar(uint16_t reg, std::string name){
      locals[name]= reg;
      roots.insert(reg);
    }
    /** @param  @name of variable
     *  @return -  register number | if non-exist - 0
     */
    uint16_t getVar(std::string name){
      if(!locals[name]){
	if(global_scope)
	  return global_scope->getVar(name);
	return 0;
      }
      return locals[name];
    }
    /** @param string @name -> variable @name
     *  @return - @e 15-bit register number, first bit uses as flag of global scope
     */
    bool exist_var(std::string name){
      if(locals[name])
	return locals[name];
      if(!global_scope)
	return false;
      return global_scope->exist_var(name);
    }
    /** @return: "allocates" new register for current scope.and @return from range 1..2^15-1.
     */
    uint16_t newReg(){
	return ++registers_count; //! @c zero registers uses for @return ing value of functions
    }
    /** @param variable @name
     *  @return : new or old register @if redefining
     * */
    void makeConstant(uint16_t reg ){
      constant_var[reg]=true;
    }
    uint16_t newVarReg(std::string name){
      auto reg = (locals[name] = newReg());
      roots.insert(reg);
      if(!global_scope){
        ConstReg(name,3);
        meta_variables_info.push_back(std::make_pair(name, reg));
      }
      return reg;
    }
    bool is_const_reg(uint16_t reg){
      return constant_var[reg];
    }
    /** @param const value in string and it's type
     *  @return: register of this const. !_GLOBAL_ register
    */
    uint16_t ContainsConst(std::string _name){
      if(constants_map[_name])
	return constants_map[_name];
      if(!global_scope)
	return 0;
      return global_scope->ContainsConst(_name);
    }
    uint16_t ConstReg(std::string name, uint8_t type){
	std::string _name;
	switch (type){
	  case 0: _name="null"; break;
	  case 1: _name="bool"; break;
	  case 2: _name="num"; break;
	  case 3: _name="str"; break;
	  default: assert(false && "type undefined");
	}
	_name+=name;
	auto _check = ContainsConst(_name);
	if(_check)
	  return _check;
	auto tmp = constants_map[_name] = newReg();
	if(type == IL::ConstTypes::null)
	  constemit(IL::loadConstcc,(uint8_t)type,tmp,(int64_t)0ll);
	else if(type == IL::ConstTypes::boolean)
	  constemit(IL::loadConstcc,(uint8_t)type,tmp,static_cast<int64_t>(name == "true"));
	else if(type == IL::ConstTypes::number)
	  constemit(IL::loadConstcc,(uint8_t)type,tmp,std::strtold(name.c_str(),nullptr));
	else{
	  char* str = new char[name.length()+1];
	  strcpy(str,name.c_str());
	  constemit(IL::loadConstcc,type,tmp,str);
	  rh->add(str);
	}
        roots.insert(tmp);
	return tmp;
    }
    void evalLoopControls(uint16_t cond,uint16_t after){
      for(auto&x:code)
	if(x.op==IL::brk)
	  x={IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,after};
	else if(x.op==IL::cnt)
	  x={IL::jcc,(uint8_t)IL::Jump::jmp,(uint16_t)0,cond};
    }
     /** @param struct-of-pcode(vm command)
     *   @return: pushes it to global scope @c const_pool, @if current-is-global->to-this->const_pool
     * */
    void emit( constpcode pc){
	  const_pool.push_back(pc);
    }
    void emit( pcode pc){
      code.push_back(pc);
    }
    /** @a sugar-for-emit, typename ... argsT template used for pcode struct's constructor.
     * @if compiltion-fails -> see constructors of struct pcode.
     * */
    template <typename ... argsT> auto emit( argsT ... args)->void{
      emit(pcode(args ... ));
    }
    template <typename ... argsT> auto constemit( argsT ... args)->void{
      emit(constpcode(args ... ));
    }
    /** @param const char* name - label @name
     * using for @a Control-Flow-Graph for evaluating offset for jcc/call instructions
     */
    uint16_t createLabel(){
      emit(pcode(IL::label,++labels));
      return labels;
    }
    /** @return pointer to top of code
     */
    uint16_t getTop(){
      assert(code.size() != 0);
      return code.size()-1;
    }
    auto getTopPtr()-> decltype(&(code[0])){
      return &code.back();
    }
    /** @param: @a pcode pc - command to replate, int @a idx - index at.
     * @return: replacing command at index
     */
    void SetAt(pcode pc, uint32_t idx){
      assert( idx >=0 && idx < code.size());
      code[idx] = pc;
    }
    /** @return vectors of code
     * getCPool actual only for global_scope*/
    auto getCode()->decltype(code){
      return code;
    }
    auto getCPool()->decltype(const_pool){
      assert(global_scope == nullptr);
      return const_pool;
    }
  };//END OF BasicBlock
#undef op_list
}
