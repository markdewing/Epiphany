//===-- EpiphanyTargetMachine.cpp - Define TargetMachine for Epiphany -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the EpiphanyTargetMachine
// methods. Principally just setting up the passes needed to generate correct
// code on this architecture.
//
//===----------------------------------------------------------------------===//

#include "Epiphany.h"
#include "EpiphanyTargetMachine.h"
#include "EpiphanyTargetObjectFile.h"
#include "MCTargetDesc/EpiphanyMCTargetDesc.h"
#include "llvm/PassManager.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<bool>
EnableLSD("double-ls", cl::Hidden,
                  cl::desc("Enable double loads and stores"),
                  cl::init(false));


extern "C" void LLVMInitializeEpiphanyTarget() {
  RegisterTargetMachine<EpiphanyTargetMachine> X(TheEpiphanyTarget);
}

static std::string computeDataLayout()
{
    return "e-p:32:32-i8:8:8-i16:16:16-i32:32:32-f32:32:32-i64:64:64-f64:64:64-s64:64:64-S64:64:64-a0:32:32";
}

EpiphanyTargetMachine::EpiphanyTargetMachine(const Target &T, const Triple &TT,
                                           StringRef CPU, StringRef FS,
                                           const TargetOptions &Options,
                                           Reloc::Model RM, CodeModel::Model CM,
                                           CodeGenOpt::Level OL)
  : LLVMTargetMachine(T, computeDataLayout(), TT, CPU, FS, Options, RM, CM, OL),
    Subtarget(TT, CPU, FS, *this),
    InstrInfo(Subtarget),
    DL("e-p:32:32-i8:8:8-i16:16:16-i32:32:32-f32:32:32-i64:64:64-f64:64:64-s64:64:64-S64:64:64-a0:32:32"),
    TLOF(make_unique<EpiphanyLinuxTargetObjectFile>()) {
      initAsmInfo();
}

void EpiphanyTargetMachine::resetSubtarget(MachineFunction *MF)
{
    MF->setSubtarget(&Subtarget);
}

namespace {
/// Epiphany Code Generator Pass Configuration Options.
class EpiphanyPassConfig : public TargetPassConfig {
public:
  EpiphanyPassConfig(EpiphanyTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  EpiphanyTargetMachine &getEpiphanyTargetMachine() const {
    return getTM<EpiphanyTargetMachine>();
  }


  bool addInstSelector() override;
  void addPreEmitPass() override;
  void addPreRegAlloc() override;
  void addPostRegAlloc() override;
};
} // namespace

TargetPassConfig *EpiphanyTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new EpiphanyPassConfig(this, PM);
}

void EpiphanyPassConfig::addPreEmitPass() {
  addPass(&UnpackMachineBundlesID);
}

bool EpiphanyPassConfig::addInstSelector() {
  addPass(createEpiphanyISelDAG(getEpiphanyTargetMachine(), getOptLevel()));
    return false;
}

void EpiphanyPassConfig::addPreRegAlloc() {
	if (EnableLSD)
		addPass(createEpiphanyLSOptPass());
}

void EpiphanyPassConfig::addPostRegAlloc() {
  addPass(createEpiphanyCondMovPass(getEpiphanyTargetMachine()));
}
