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

#define DEBUG_TYPE "Print"

namespace {

  // Basic-block printing only (no transformation)

  struct Print : public FunctionPass {
    static char ID;
    Print() : FunctionPass(ID) {}

    template<typename Range> int getNum(std::vector<Range*>& ptrs, Value* elem){
      int num = -1;
      int cnt = 0;
      for (auto search = ptrs.begin(); search != ptrs.end(); ++search){
        if (*search == elem) num = cnt;
        cnt++;
      }
      return num;
    }

    template<typename Range> int add(std::vector<Range*>& ptrs, Value* elem){
      int num = getNum(ptrs, elem);
      if (num == -1) {
        ptrs.push_back(elem);
        return ptrs.size()-1;
      }
      else
        return num;
    }

    void printInstr(Instruction* I, std::vector<Value*>& ptrs, std::vector<Instruction*>& instrs){
      errs () << "   Instr # " << instrs.size() << ": " << I->getOpcodeName ();
      ICmpInst* icmp;
      if ((icmp = dyn_cast<ICmpInst>(I))){
        switch(icmp->getPredicate ()) {
          case llvm::CmpInst::ICMP_EQ : errs () << " eq"; break;
          case llvm::CmpInst::ICMP_NE : errs () << " ne"; break;
          case llvm::CmpInst::ICMP_UGT : errs () << " ugt"; break;
          case llvm::CmpInst::ICMP_UGE : errs () << " uge"; break;
          case llvm::CmpInst::ICMP_ULT : errs () << " ult"; break;
          case llvm::CmpInst::ICMP_ULE : errs () << " ule"; break;
          default : break;
        }
      }

      errs () << " (";
      for (unsigned i = 0; i < I->getNumOperands (); i++) {
        Value* op = I->getOperand (i);
        errs () << " [ " << op->getName (); // << ", ";
        
        Constant* c;
        Instruction* in;
        PointerType *PTy;
        AllocaInst* alloIn;
        if ((alloIn = dyn_cast<AllocaInst> (op))){
          int num = getNum(instrs, alloIn);
          if (num >= 0) errs () << " < Instr #" << num << " > ";
        } else if (op->getType()->getTypeID() == Type::IntegerTyID && (c = dyn_cast<Constant> (op))){
          errs () << c->getUniqueInteger ();
        }
        else if (op->getType()->getTypeID() == Type::IntegerTyID && (in = dyn_cast<Instruction> (op))){
          int num = getNum(instrs, in);
          if (num >= 0) errs () << " < Instr #" << num << " > ";
        }
        else if (op->getType()->getTypeID() == Type::PointerTyID && (PTy = dyn_cast<PointerType>(op->getType()))){
          if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(op)){
            int num = getNum(instrs, gep);
            if (num >= 0) errs () << " < Instr #" << num << " > ";
          }
        }
        else if (op->getType()->getTypeID() == Type::LabelTyID){
          errs () << "PTR: " << add(ptrs, op);
        }
        else {
          switch (op->getType()->getTypeID()) {
            case Type::IntegerTyID: errs () << "IntegerTyID"; break;
            case Type::PointerTyID: errs () << "PointerTyID"; break;
            case Type::LabelTyID: errs () << "LabelTyID"; break;
            default: errs () <<" --- Unknown type  type";
          }
          errs () << " - XXX";
        }
        errs () << " ]";
      }
      instrs.push_back(I);
      errs () << " )\n";
    }

    bool runOnFunction(Function &F) override {

      errs () << "RUNNING on: " << F.getName () << "\n";

      std::vector<Value*> ptrs;
      std::vector<Instruction*> instrs;
      for (BasicBlock &b : F){
        errs () << " -- basic block " << getNum(ptrs, dyn_cast<Value> (&b)) << "\n";
        for (User &u : b) {
          printInstr(dyn_cast<Instruction> (&u), ptrs, instrs);
        }
      }

      return false;
    }
  };
}

char Print::ID = 0;
static RegisterPass<Print> X("print", "Print Pass");

