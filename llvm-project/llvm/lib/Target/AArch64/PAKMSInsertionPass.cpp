#include "AArch64.h"
#include "AArch64Subtarget.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MD5.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include <chrono>
#include <random>

#define DEBUG_TYPE "pakms-insertion"

using namespace llvm;

namespace {
    class PAKMSInsertionPass : public MachineFunctionPass {

    public:
        static char ID;

        PAKMSInsertionPass() : MachineFunctionPass(ID) {}
        virtual bool runOnMachineFunction(MachineFunction &MF) {

	    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	    std::mt19937_64 g2 (seed);//g2() use seed to return a u64 random number

            uint64_t fid = MD5Hash(MF.getName())^g2();

	    const AArch64InstrInfo *TII = static_cast<const AArch64InstrInfo *>(
		MF.getSubtarget().getInstrInfo());

	    //insert instructions at the beginning
	    MachineBasicBlock& firstMBB = *MF.begin();
	    MachineInstr& firstMI = *firstMBB.begin();
/*
 *
 *	    "str x0,[sp,#-16]!;"//store params x0
 *	    "mov x8,#430;"//syscall number
 *	    "ldr x0,=fid;"//x0:fid
 *	    "svc #0;"
 *	    "mov x8,x0;"
 *	    "ldr x0,[sp],#16;"
 *	    "paciasp;"
 *	    "str x8,[sp,#-16]!;"//store Gn(return by svc in x0) on stack before use pac* instr
 */
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::STRXpre))
				.addReg(AArch64::SP, RegState::Define)
                                .addReg(AArch64::X0)
                                .addReg(AArch64::SP)
                                .addImm(-16);
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::MOVZXi))
                                .addReg(AArch64::X8)
                                .addImm(430)
                                .addImm(0);
/*
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXl))
                                .addReg(AArch64::X0)
				.addImm(fid);//???
*/
            BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              			.addReg(AArch64::X0).addReg(AArch64::X0)
              			.addImm(fid&0xffff)
              			.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 0));
            BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              			.addReg(AArch64::X0).addReg(AArch64::X0)
              			.addImm((fid>>16)&0xffff)
              			.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 16));
            BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              			.addReg(AArch64::X0).addReg(AArch64::X0)
              			.addImm((fid>>32)&0xffff)
              			.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 32));
            BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              			.addReg(AArch64::X0).addReg(AArch64::X0)
              			.addImm((fid>>48)&0xffff)
              			.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 48));

	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::SVC))
				.addImm(0);
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::ORRXrs))
                            	.addReg(AArch64::X8)
				.addReg(AArch64::XZR)
				.addReg(AArch64::X0)
                                .addImm(0);
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXpost))
				.addReg(AArch64::SP, RegState::Define)
                            	.addReg(AArch64::X0, RegState::Define)
				.addReg(AArch64::SP)
                                .addImm(16);
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::PACIASP));
	    BuildMI(firstMBB, firstMI, firstMBB.findDebugLoc(firstMI), TII->get(AArch64::STRXpre))
				.addReg(AArch64::SP, RegState::Define)
                                .addReg(AArch64::X8)
                                .addReg(AArch64::SP)
                                .addImm(-16);

	    //insert instructions before the function return
	    for (auto &MBB : MF){
		if(MBB.isReturnBlock()){
		    outs() << "deal with the Return Block\n";
		    for(auto &MI : MBB)
			if(MI.isReturn()){
			    outs() << "deal with the Return Instr\n";
/*
 *	    "ldr x8,[sp];"//load Gn to x8
 *	    "str x1,[sp];"//store x0 and x1
 *	    "str x0,[sp,#-16]!;"
 *	    "mov x1,x8;"
 *	    "mov x8,#431;"//syscall number
 *	    "ldr x0,=fid;"//x0:fid
 *	    "svc #0;"
 *	    "ldr x0,[sp],#16;"
 *	    "ldr x1,[sp],#16;"
 *	    "autiasp;"
 */
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXui))
                            		.addReg(AArch64::X8)
					.addReg(AArch64::SP)
                                	.addImm(0);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::STRXui))
                                	.addReg(AArch64::X1)
                                	.addReg(AArch64::SP)
                                	.addImm(0);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::STRXpre))
					.addReg(AArch64::SP, RegState::Define)
                                	.addReg(AArch64::X0)
                               	 	.addReg(AArch64::SP)
                                	.addImm(-16);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::ORRXrs))
                            		.addReg(AArch64::X1)
					.addReg(AArch64::XZR)
					.addReg(AArch64::X8)
                                	.addImm(0);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::MOVZXi))
					.addReg(AArch64::X8)
                                	.addImm(431)
                                	.addImm(0);
/*
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXl))
					.addReg(AArch64::X0)
                                	.addImm(fid);//???
*/
            		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              				.addReg(AArch64::X0).addReg(AArch64::X0)
              				.addImm(fid&0xffff)
              				.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 0));
            		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              				.addReg(AArch64::X0).addReg(AArch64::X0)
              				.addImm((fid>>16)&0xffff)
              				.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 16));
            		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              				.addReg(AArch64::X0).addReg(AArch64::X0)
              				.addImm((fid>>32)&0xffff)
              				.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 32));
            		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::MOVKXi))
              				.addReg(AArch64::X0).addReg(AArch64::X0)
              				.addImm((fid>>48)&0xffff)
              				.addImm(AArch64_AM::getShifterImm(AArch64_AM::LSL, 48));

	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::SVC))
                                	.addImm(0);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXpost))
					.addReg(AArch64::SP, RegState::Define)
                            		.addReg(AArch64::X0, RegState::Define)
					.addReg(AArch64::SP)
                                	.addImm(16);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::LDRXpost))
					.addReg(AArch64::SP, RegState::Define)
                            		.addReg(AArch64::X1, RegState::Define)
					.addReg(AArch64::SP)
                                	.addImm(16);
	    		    BuildMI(MBB, MI, MBB.findDebugLoc(firstMI), TII->get(AArch64::AUTIASP));


			}
		    
	        }
	    }
            return true;
        }
    };
}

namespace llvm {
    FunctionPass *createPAKMSInsertionPass(){
        return new PAKMSInsertionPass();
    }
}

char PAKMSInsertionPass::ID = 0;
static RegisterPass<PAKMSInsertionPass> X(DEBUG_TYPE, "Insert pakms instructions. ");

