
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
using namespace llvm;

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct Hello : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    Hello() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {

      errs() << "Hello: ";
      errs().write_escaped(F.getName()) << '\n';
	LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
	for(Loop *L: *LI)
	{
		unsigned int count=0;
		Loop::block_iterator bb;
		for(bb=L->block_begin();bb!=L->block_end();++bb)count++;
		errs()<<"Loop has "<<count<<" blocks\n";
	}
      return false;
    }
    void getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
        AU.addRequired<LoopInfoWrapperPass>();
    }
  };
}

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass",false,false);

