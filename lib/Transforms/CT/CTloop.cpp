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

    void balanceLoop (BranchInst* bri, std::vector<BasicBlock*>& loop){
      // currently, bri is used only to insert the initialization of an artificial counter

      BranchInst* lastBri;
      unsigned int i = 0;
      bool found = false;
      for (; i < loop.size() - 1 && !found; i++)
      {
        if ((lastBri = dyn_cast<BranchInst> (&*(loop[i]->rbegin ())))){
          if (i == 0 && lastBri->getNumSuccessors() == 1) return;
          if (i > 0 && lastBri->getNumSuccessors() == 2) {found = true; break;}
        }
        else return;
      }

      if (!found) return;
      errs() << "Balancing [" << i << "th node]\n";

      BranchInst* headBr = dyn_cast<BranchInst> (&*(loop[0]->rbegin ()));

      ConstantInt *cc = ConstantInt::get(headBr->getContext(), APInt(32, 0));

      GlobalVariable *var = new GlobalVariable(
              *headBr->getParent()->getParent()->getParent(),
              Type::getInt32Ty(headBr->getContext()),
              false, GlobalValue::ExternalLinkage, nullptr, "_tmp_counter");
      GlobalVariable *varRet = new GlobalVariable(
              *headBr->getParent()->getParent()->getParent(),
              Type::getInt32Ty(headBr->getContext()),
              false, GlobalValue::ExternalLinkage, nullptr, "_tmp_ret");
      var->setInitializer(cc);
      varRet->setInitializer(cc);

      IRBuilder<> builder(headBr->getContext());
      Instruction *I2 = builder.CreateStore(cc, (Value*)var);         // here; we assume the original counter starts with 0
      Instruction *I2ret = builder.CreateStore(cc, (Value*)varRet);   // here; artificial return always starts with zero
      I2->insertBefore(bri);
      I2ret->insertBefore(I2);

      //
      // here, we instrument both branches with artif. counter:

      for (int j = 0; j < 2; j++){
        
        BasicBlock* b1 = headBr->getSuccessor(j);
        if (b1 == loop[1]) {
          // loop iteration
          
          Instruction* loop1 = builder.CreateLoad(var);
          
          BinaryOperator *loop2 = BinaryOperator::
          CreateAdd(loop1, ConstantInt::get(headBr->getContext(), APInt(32, 1)));
          
          Instruction* loop3 = builder.CreateStore(loop2, var);
          
          loop1->insertBefore(&*(b1->begin ()));  // adding an increment
          loop2->insertAfter(loop1);              // here, we assume the original counter increments by one
          loop3->insertAfter(loop2);              //
          
        }
      }

      // here, we replace the loop guard:
      ICmpInst *old_icmp = dyn_cast<ICmpInst>(headBr->getCondition ());
      Instruction* loadTmp = builder.CreateLoad(var);
      loadTmp->insertBefore(headBr);
      ICmpInst *inIcmp = new ICmpInst(old_icmp->getPredicate(), loadTmp, old_icmp->getOperand(1));
      // tested for ICmpInst::ICMP_ULE
      inIcmp->insertAfter(loadTmp);

      BranchInst* newBr = BranchInst::Create(headBr->getSuccessor(0), headBr->getSuccessor(1), inIcmp);
      newBr->insertBefore(headBr);
      headBr->eraseFromParent();
      old_icmp->eraseFromParent();
      loop[0]->begin()->eraseFromParent(); // erase "load s"

      
      // then, proceed further to the loop
      // previously, we identified the divergence at lastBri = dyn_cast<BranchInst> (&*(loop[i]->rbegin ()))
      // (thus need to keep the edge lastBri->loop[i+1] only)

      {
        // in the future, it will be in the loop for each "return"
        BasicBlock* toRemove;
        for (unsigned int j = 0; j < lastBri->getNumSuccessors (); j++){
          if (loop[i+1] != lastBri->getSuccessor(j)) toRemove = lastBri->getSuccessor(j);
        }
        
        BranchInst* oldBr = dyn_cast<BranchInst> (&*(toRemove->rbegin ()));
        Instruction* newBr = loop[i+1]->rbegin ()->clone();
        newBr->insertAfter(oldBr);
        oldBr->eraseFromParent();
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

