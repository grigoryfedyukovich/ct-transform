#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/IRBuilder.h"
#include <map>
#include <set>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "CTite"

namespace {

  // Constant-time transformation for if-then-elses only

  struct CTite : public FunctionPass {
    static char ID;
    CTite() : FunctionPass(ID) {}

    bool isEquiv (Value* in1, Value* in2){
      if (in1 == in2) return true;
      if ((in1->getName() != in2->getName())) return false;
      Instruction* in1i;
      Instruction* in2i;
      if ((in1i = dyn_cast<Instruction> (in1)) && (in2i = dyn_cast<Instruction> (in2))){
        if (in1i->getNumOperands() != in2i->getNumOperands()) return false;

        for (unsigned i = 0; i < in1i->getNumOperands (); i++)
          if (!isEquiv(in1i->getOperand (i), in2i->getOperand (i))) return false;

      } else {
        return false;
      }
      return true;
    }

    void getLastStores (BasicBlock * b, std::map<Value *, Value* >& stores){
      for (auto it = b->rbegin (); it != b->rend (); ++it) {
        Instruction* I = dyn_cast<Instruction> (&*it);
        StoreInst* stIn;
        if ((stIn = dyn_cast<StoreInst>(I)) &&
            (stIn->getOperand (1)->getType()->getTypeID() == Type::PointerTyID))
        {
          if (stores[stIn->getOperand (1)] == NULL)
            stores[stIn->getOperand (1)] = stIn->getOperand (0);
        }
      }
    }

    Value* getLoad (std::vector<BasicBlock*> chain, Value* ptr){
      for (auto it = chain.begin(); it != chain.end()-1; ++it){
        auto & b = *it;
        for (auto it = b->begin (); it != b->end (); ++it) {
          Instruction* I = dyn_cast<Instruction> (&*it);
          LoadInst* loIn;
          if ((loIn = dyn_cast<LoadInst>(I)) &&
              (loIn->getOperand (0)->getType()->getTypeID() == Type::PointerTyID) &&
              (loIn->getOperand (0) == ptr))
          {
            return loIn;
          }
        }
      }
      return NULL;
    }

    void rewriteStores (BasicBlock * b, Value* ptr, Value* &vs, Value* &vl, std::vector<Instruction*>& toRemove) {
      // for arrays (some very basic support):
      GetElementPtrInst* gep;
      if ((gep = dyn_cast<GetElementPtrInst>(ptr))){
        rewriteStoresGEP(b, gep, vs, vl, toRemove);
        return;
      }

      for (auto it = b->begin (); it != b->end (); ++it) {
        Instruction* I = dyn_cast<Instruction> (&*it);
        StoreInst* stIn;
        LoadInst *loIn;

        if ((stIn = dyn_cast<StoreInst>(I)) &&
            (stIn->getOperand (1)->getType()->getTypeID() == Type::PointerTyID) &&
            ptr == stIn->getOperand (1))
        {
          vs = stIn->getOperand (0);
          toRemove.push_back(stIn);
        }
        else if ((loIn = dyn_cast<LoadInst>(I)) &&
                 (loIn->getOperand (0)->getType()->getTypeID() == Type::PointerTyID) &&
                 ptr == loIn->getOperand (0) &&
                 vs != NULL)
        {
          vl = loIn;
          toRemove.push_back(loIn);
        } else {
          for (auto opnd = I->op_begin(); opnd < I->op_end(); ++opnd) {
            if (*opnd == vl) {
              *opnd = vs;
            }
          }
        }
      }
    }

    void rewriteStoresGEP (BasicBlock * b, GetElementPtrInst* ptr, Value* &vs, Value* &vl, std::vector<Instruction*>& toRemove) {

      for (auto it = b->begin (); it != b->end (); ++it) {
        Instruction* I = dyn_cast<Instruction> (&*it);
        StoreInst* stIn;
        LoadInst *loIn;
        GetElementPtrInst *gep;

        if ((stIn = dyn_cast<StoreInst>(I)) &&
            (stIn->getOperand (1)->getType()->getTypeID() == Type::PointerTyID) &&
            (gep = dyn_cast<GetElementPtrInst>(stIn->getOperand (1))) &&
            isEquiv(gep, ptr))
        {
          vs = gep;
          toRemove.push_back(stIn);
        }
        else if ((loIn = dyn_cast<LoadInst>(I)) &&
                 (loIn->getOperand (0)->getType()->getTypeID() == Type::PointerTyID) &&
                 (gep = dyn_cast<GetElementPtrInst>(loIn->getOperand (0))) &&
                 isEquiv(gep, ptr) &&
                 vs != NULL)
        {
          vl = loIn;
          toRemove.push_back(loIn);
        }
      }
    }

    BasicBlock* getBBsuccessor (BasicBlock* bb){
      BranchInst* lastBri;
      if ((lastBri = dyn_cast<BranchInst> (&*(bb->rbegin ())))){
        if (lastBri->getNumSuccessors () > 1) return NULL;
        return lastBri->getSuccessor(0);
      }
      return NULL;
    }

    void getBBchain(BasicBlock* bb, std::vector<BasicBlock*>& chain){
      if (bb != NULL)
      {
        if (chain.size() > 0 && bb == *chain.begin()) return;
        else
        {
          chain.push_back(bb);
          getBBchain(getBBsuccessor(bb), chain);
        }
      }
    }

    BasicBlock* cutAtCommonEl(std::vector<BasicBlock*>& chain1, std::vector<BasicBlock*>& chain2){
      for (auto it1 = chain1.begin(); it1 != chain1.end(); ++it1){
        for (auto it2 = chain2.begin(); it2 != chain2.end(); ++it2){
          if (*it1 == *it2){
            BasicBlock* bb = *it1;
            chain1.erase (it1+1, chain1.end());
            chain2.erase (it2+1, chain2.end());
            return bb;
          }
        }
      }
      return NULL;
    }

    Function * fun_ct_iszero (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_is_zero",
              Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_eq (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_eq",
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_lt (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_lt",
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_le (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_le",
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_gt (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_gt",
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_ge (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_ge",
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    Function * fun_ct_select (Module* M, LLVMContext& C){
      return cast<Function>(M->
          getOrInsertFunction("constant_time_select", Type::getInt32Ty(C),
              Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
    }

    bool isDiamond (BranchInst* bri){
      if (bri->getNumSuccessors () == 1) return false;

      std::vector<BasicBlock*> chain1;
      std::vector<BasicBlock*> chain2;
      getBBchain (bri->getSuccessor(0), chain1);
      getBBchain (bri->getSuccessor(1), chain2);

      cutAtCommonEl(chain1, chain2);

      return chain1.back() == chain2.back() &&
        chain1.size() > 0 && chain2.size() > 0;
    }

    void flattenDiamond (BranchInst* bri){
      Value * cnd = bri->getCondition ();

      ICmpInst* icmp;
      Instruction* repl;
      if ((icmp = dyn_cast<ICmpInst>(cnd))){
        repl = replaceIcmp(icmp);
      }

      BranchInst* toRepl = bri;

      // here is some redundany with `isDiamond`:
      std::vector<BasicBlock*> chain1;
      std::vector<BasicBlock*> chain2;

      getBBchain (bri->getSuccessor(0), chain1);
      getBBchain (bri->getSuccessor(1), chain2);
      BasicBlock* cmn = cutAtCommonEl(chain1, chain2);

      std::map<Value *, Value* > stores1;
      std::map<Value *, Value* > stores2;

      // process left branch
      if (chain1.size() > 1) {
        for (auto it = chain1.rbegin ()+1; it != chain1.rend (); ++it)
          getLastStores(*it, stores1);
        BranchInst* newIns = BranchInst::Create(*chain1.begin());
        newIns->insertBefore(toRepl);
        toRepl->eraseFromParent();
        toRepl = dyn_cast<BranchInst> (&*((*(++chain1.rbegin ()))->rbegin ()));
      }

      // process right branch
      if (chain2.size() > 1) {
        for (auto it = chain2.rbegin ()+1; it != chain2.rend (); ++it)
          getLastStores(*it, stores2);
        BranchInst* newIns = BranchInst::Create(*chain2.begin());
        newIns->insertBefore(toRepl);
        toRepl->eraseFromParent();
        toRepl = dyn_cast<BranchInst> (&*((*(++chain2.rbegin ()))->rbegin ()));
      }

      // create new artif BB
      BasicBlock* bnew = BasicBlock::Create(toRepl->getContext(), "", toRepl->getParent()->getParent());
      BranchInst* brnew = BranchInst::Create(cmn, bnew);
      BranchInst* newIns = BranchInst::Create(bnew);
      newIns->insertBefore(toRepl);
      toRepl->eraseFromParent();

      // repair the code after the merge
      std::set <Value *> ptrs;
      for (auto & p : stores1) ptrs.insert(p.first);
      for (auto & p : stores2) ptrs.insert(p.first);
      if (ptrs.size() == 0) return;

      BasicBlock* bn = toRepl->getSuccessor(0);
      IRBuilder<> builder(toRepl->getContext());
      Function *ite_func = fun_ct_select(bn->getParent()->getParent(), bn->getContext());

      for (auto & ptr : ptrs) {

        Value* v1 = stores1[ptr];
        Value* v2 = stores2[ptr];

        // check if there exists a branch in which the variable is not reassigned
        // first, try to search for existing loads
        if (v1 == NULL) v1 = getLoad(chain2, ptr);
        if (v2 == NULL) v2 = getLoad(chain1, ptr);

        // second (if no success above) create a new load
        if (v1 == NULL || v2 == NULL){
          assert (v1 != NULL || v2 != NULL);
          LoadInst *redund = NULL;
          redund = builder.CreateLoad((Value*)ptr);
          redund->insertBefore(brnew);
          if (v1 == NULL) { v1 = redund; }
          else { v2 = redund; }
        }

        CallInst *op;
        // little hack: NE is treated in the same way as EQ
        // but with the opposite order of elements
        if (icmp->getPredicate () == llvm::CmpInst::ICMP_NE)
          op = builder.CreateCall(ite_func, { repl, v2, v1});
        else
          op = builder.CreateCall(ite_func, { repl, v1, v2});

        op->insertBefore(brnew);
        StoreInst * s = builder.CreateStore(op, (Value*)ptr);
        s->insertAfter(op);

        Value* vs = NULL;
        Value* vl = NULL;
        std::vector<Instruction*> toRemove;
        for (auto it = chain1.begin (); it != chain1.end ()-1; ++it)
          rewriteStores(*it, ptr, vs, vl, toRemove);
        vs = NULL;
        vl = NULL;
        for (auto it = chain2.begin (); it != chain2.end ()-1; ++it)
          rewriteStores(*it, ptr, vs, vl, toRemove);
        for (auto & a : toRemove) a->eraseFromParent();
      }
      icmp->eraseFromParent();
    }

    CallInst * replaceIcmp(ICmpInst* icmp){
      std::vector<Value*> vars;
      IRBuilder<> builder(icmp->getContext());
      CallInst *op = nullptr;
      Module *M = icmp->getParent()->getParent()->getParent();
      Function *ct_fun;

      switch(icmp->getPredicate ()) {
        case llvm::CmpInst::ICMP_EQ : ct_fun = fun_ct_eq(M, icmp->getContext()); break;
        case llvm::CmpInst::ICMP_NE : ct_fun = fun_ct_eq(M, icmp->getContext()); break;
        case llvm::CmpInst::ICMP_UGT : ct_fun = fun_ct_gt(M, icmp->getContext()); break;
        case llvm::CmpInst::ICMP_UGE : ct_fun = fun_ct_ge(M, icmp->getContext()); break;
        case llvm::CmpInst::ICMP_ULT : ct_fun = fun_ct_lt(M, icmp->getContext()); break;
        case llvm::CmpInst::ICMP_ULE : ct_fun = fun_ct_le(M, icmp->getContext()); break;
        default : assert (0);
      }

      op = builder.CreateCall(ct_fun, {icmp->getOperand (0), icmp->getOperand (1)});
      op->insertAfter(icmp);

      return op;
    }

    bool runOnFunction(Function &F) override {

      bool toRecheck = true;

      while (toRecheck){

        std::set<Instruction*> toRem;
        std::vector<Value*> ptrs;
        
        toRecheck = false;
        
        for (BasicBlock &b : F){
          for (User &u : b) {
            BranchInst* bri;
            if ((bri = dyn_cast<BranchInst>(&u)) && isDiamond(bri)) {
              flattenDiamond(bri);
              toRecheck = true;
              break;
            }
          }
        }
        for (auto & a : toRem) a->eraseFromParent();
      }

      return false;
    }
  };
}

char CTite::ID = 0;
static RegisterPass<CTite> X("ct-ite", "CT Pass For if-then-elses");

