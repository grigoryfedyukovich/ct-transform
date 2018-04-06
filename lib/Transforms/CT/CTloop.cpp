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

#define DEBUG_TYPE "CTloop"

namespace {

  // Constant-time transformation for loops only (under development)
  // works for one loop now with the counter which starts from 0;

  struct CTloop : public FunctionPass {
    static char ID;
    CTloop() : FunctionPass(ID) {}

    std::vector<BasicBlock*> loop;

    void mkConditionalStore (Value *varRet, StoreInst * stIn) {

      // this could be done in two stages: via 1) creating icmp, br, two new blocks and 2) running ct-ite
      // but the current version is an optimized one: using ct_eq + ct_select

      LLVMContext& C = stIn->getContext();
      IRBuilder<> builder(C);
      Module *M = stIn->getParent()->getParent()->getParent();

      Instruction* ldIn = builder.CreateLoad(varRet);
      ldIn->insertBefore(stIn);
      Function * ct_eq = cast<Function>(M->getOrInsertFunction("constant_time_is_zero",
                    Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
      CallInst * grdIn = builder.CreateCall(ct_eq, {ldIn});
      grdIn->insertAfter(ldIn);

      Instruction* ldIn2 = builder.CreateLoad(stIn->getOperand (1));
      ldIn2->insertAfter(grdIn);
      Function * ct_ite = cast<Function>(M->getOrInsertFunction("constant_time_select", Type::getInt32Ty(C),
                    Type::getInt32Ty(C), Type::getInt32Ty(C), Type::getInt32Ty(C), NULL));
      CallInst * clIn = builder.CreateCall(ct_ite, {grdIn, stIn->getOperand (0), ldIn2} );
      clIn->insertAfter(ldIn2);
      Instruction *stIn2 = builder.CreateStore(clIn, stIn->getOperand (1));
      stIn2->insertAfter(clIn);

      stIn->eraseFromParent();
    }

    void rewriteStores (BasicBlock * b, Value *varRet){
      for (auto it = b->rbegin (); it != b->rend (); ++it) {
        Instruction* I = dyn_cast<Instruction> (&*it);
        StoreInst* stIn;
        if ((stIn = dyn_cast<StoreInst>(I)) &&
            (stIn->getOperand (1)->getType()->getTypeID() == Type::PointerTyID))
        {
          mkConditionalStore(varRet, stIn);
        }
      }
    }

    bool isReachable (BasicBlock* next, std::vector<BasicBlock*> prev) {
      prev.push_back(next);
      BranchInst* lastBri;
      if ((lastBri = dyn_cast<BranchInst> (&*(next->rbegin ())))){
        for (unsigned int i = 0; i < lastBri->getNumSuccessors (); i++){
          BasicBlock* bs = lastBri->getSuccessor(i);
          for (unsigned int j = 0; j < prev.size(); j++){
            if (prev[j] == bs) {
              prev.push_back(prev[j]);
              prev.erase(prev.begin(),prev.begin()+j);
              // GF: tbd, cleaning
              if (loop.size() == 0) loop = prev;
              return true;
            }
          }
          
          if (isReachable (bs, prev)){
            return true;
          }
        }
      }
      return false;
    }

    bool isLoop (BranchInst* bri){
      loop.clear();
      return isReachable(bri->getParent(), std::vector<BasicBlock*> ());
    }

    bool replaceValue (Value* val, Value* from, Value* to){
      if (Instruction* instr = dyn_cast<Instruction>(val)) {
        for (auto opnd = instr->op_begin(); opnd < instr->op_end(); ++opnd) {
          if (*opnd == from) {
            *opnd = to;
            return true;
          } else {
            if (replaceValue (*opnd, from, to)) return true;
          }
        }
      }
      return false;
    }

    bool isChainToExit(BasicBlock* start, BasicBlock* end) {
      if (start == end) return true;
      BranchInst* lastBri;
      if ((lastBri = dyn_cast<BranchInst> (&*(start->rbegin ())))){
        if (lastBri->getNumSuccessors() > 1) return false;
          else return isChainToExit (lastBri->getSuccessor(0), end);
      } else return false;
    }

    void balanceLoop (BranchInst* bri, std::vector<BasicBlock*>& loop){
      // currently, bri is used only to insert the initialization of an artificial counter

      BranchInst* headBr;
      if (!(headBr = dyn_cast<BranchInst> (&*(loop[0]->rbegin ())))) return;
      if (headBr->getNumSuccessors() == 1) return;
      BasicBlock* exitBB = (headBr->getSuccessor(0) == loop[1]) ?
                            headBr->getSuccessor(1) : headBr->getSuccessor(0);

      BranchInst* lastBri;
      std::vector<BranchInst* > bris;
      std::vector<BranchInst* > brisSingl;
      for (unsigned int i = 1; i < loop.size() - 1; i++)
      {
        if ((lastBri = dyn_cast<BranchInst> (&*(loop[i]->rbegin ())))){
          if (lastBri->getNumSuccessors() == 1 && lastBri->getSuccessor(0) == loop[0]) {
            brisSingl.push_back(lastBri);
          } else if (lastBri->getNumSuccessors() == 2 &&
             (isChainToExit(lastBri->getSuccessor(0), exitBB) || isChainToExit(lastBri->getSuccessor(1), exitBB))) {
            bris.push_back(lastBri);
          }
        }
        else return;
      }

      if (bris.size() == 0) return;

      IRBuilder<> builder(headBr->getContext());
      ConstantInt *cc = ConstantInt::get(headBr->getContext(), APInt(32, 0));

      AllocaInst* var = new AllocaInst(Type::getInt32Ty(headBr->getContext()), "_tmp_counter");
      var->insertBefore(bri);
      Instruction *strVar = builder.CreateStore(cc, var);
      strVar->insertAfter(var);

      AllocaInst* varRet = new AllocaInst(Type::getInt32Ty(headBr->getContext()), "_tmp_ret");
      varRet->insertBefore(bri);
      Instruction *strVarRet = builder.CreateStore(cc, varRet);
      strVarRet->insertAfter(varRet);

      // then, before each store, we check if the original loop has already terminated
      for (unsigned int i = 1; i < loop.size()-1; i++) rewriteStores(loop[i], varRet);

      // then, we instrument the body with artif. counter placed in a new block
      BasicBlock* bn = BasicBlock::Create(headBr->getContext(), "", loop[0]->getParent());
      BranchInst* brn = BranchInst::Create(loop[0], bn);
      Instruction* varLd = builder.CreateLoad(var);
      BinaryOperator *varInc = BinaryOperator::
            CreateAdd(varLd, ConstantInt::get(headBr->getContext(), APInt(32, 1)));
      Instruction* varStor = builder.CreateStore(varInc, var);

      varLd->insertBefore(brn);               // adding an increment
      varInc->insertAfter(varLd);             // here, we assume the original counter increments by one
      varStor->insertAfter(varInc);             // and starts with zero

      for (auto a : brisSingl) replaceValue(a, loop[0], bn);

      // here, we replace the loop guard:
      ICmpInst *old_icmp = dyn_cast<ICmpInst>(headBr->getCondition ());
      Value* torepl = ((Instruction*)old_icmp->getOperand(0))->getOperand(0); /// currently, assume the counter is the first operand in the loop guard
      replaceValue(old_icmp, torepl, var);
      BranchInst* newBr = BranchInst::Create(headBr->getSuccessor(0), headBr->getSuccessor(1), old_icmp);
      newBr->insertBefore(headBr);
      headBr->eraseFromParent();

      // then, proceed further to the loop
      // previously, we identified the divergence at lastBri = dyn_cast<BranchInst> (&*(loop[i]->rbegin ()))
      // (thus need to keep the edge lastBri->loop[i+1] only)

      for (auto lbri : bris) {
        BranchInst* briToRemove;
        if (isChainToExit(lbri->getSuccessor(0), exitBB)) {
          briToRemove = dyn_cast<BranchInst> (&*(lbri->getSuccessor(0)->rbegin ()));
        } else {
          briToRemove = dyn_cast<BranchInst> (&*(lbri->getSuccessor(1)->rbegin ()));
        }

        ConstantInt *c1 = ConstantInt::get(headBr->getContext(), APInt(32, 1));
        Instruction *retSt = builder.CreateStore(c1, (Value*)varRet);
        retSt->insertBefore(briToRemove);

        Instruction* newBr = BranchInst::Create(bn);
        newBr->insertAfter(briToRemove);
        briToRemove->eraseFromParent();
      }
    }

    bool runOnFunction(Function &F) override {
      bool toRecheck = true;
      bool balanced = false;

      while (toRecheck){

        std::set<Instruction*> toRem;
        toRecheck = false;

        for (BasicBlock &b : F){
          for (User &u : b) {
            BranchInst* bri;
            if ((bri = dyn_cast<BranchInst>(&u)) && isLoop(bri) && !balanced) {
               balanced = true;
               balanceLoop(bri, loop);
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

char CTloop::ID = 0;
static RegisterPass<CTloop> X("ct-loop", "CTloop Pass");

