//==- EpiphanyFrameLowering.h - Define frame lowering for Epiphany -*- C++ -*--=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class implements the Epiphany-specific parts of the TargetFrameLowering
// class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EPIPHANY_FRAMEINFO_H
#define LLVM_EPIPHANY_FRAMEINFO_H

#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class EpiphanyFrameLowering : public TargetFrameLowering {
private:
  // In order to unify the spilling and restoring of callee-saved registers into
  // emitFrameMemOps, we need to be able to specify which instructions to use
  // for the relevant memory operations on each register class. An array of the
  // following struct is populated and passed in to achieve this.
  struct LoadStoreMethod {
    const TargetRegisterClass *RegClass; // E.g. GPR64RegClass

    // The preferred instruction.
    unsigned PairOpcode; // E.g. LSPair64_STR

    // Sometimes only a single register can be handled at once.
    unsigned SingleOpcode; // E.g. LS64_STR
  };
protected:

public:
  explicit EpiphanyFrameLowering()
    : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, 8, 0, 4) 
  {
  }

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  virtual void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  virtual void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  /// Decides how much stack adjustment to perform in each phase of the prologue
  /// and epilogue.
  void splitSPAdjustments(uint64_t Total, uint64_t &Initial,
                          uint64_t &Residual) const;

  int64_t resolveFrameIndexReference(MachineFunction &MF, int FrameIndex,
                                     unsigned &FrameReg, int SPAdj,
                                     bool IsCalleeSaveOp) const;

  virtual void processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                                    RegScavenger *RS) const;

  virtual bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        const std::vector<CalleeSavedInfo> &CSI,
                                        const TargetRegisterInfo *TRI) const;
  virtual bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        const std::vector<CalleeSavedInfo> &CSI,
                                        const TargetRegisterInfo *TRI) const;
   	
  void eliminateCallFramePseudoInstr(MachineFunction &MF,
										MachineBasicBlock &MBB,
										MachineBasicBlock::iterator MI) const;

  /// If the register is X30 (i.e. LR) and the return address is used in the
  /// function then the callee-save store doesn't actually kill the register,
  /// otherwise it does.
  bool determinePrologueDeath(MachineBasicBlock &MBB, unsigned Reg) const;

  /// This function emits the loads or stores required during prologue and
  /// epilogue as efficiently as possible.
  ///
  /// The operations involved in setting up and tearing down the frame are
  /// similar enough to warrant a shared function, particularly as discrepancies
  /// between the two would be disastrous.
  void emitFrameMemOps(bool isStore, MachineBasicBlock &MBB,
                       MachineBasicBlock::iterator MI,
                       const std::vector<CalleeSavedInfo> &CSI,
                       const TargetRegisterInfo *TRI) const;


  bool hasFP(const MachineFunction &MF) const override;

  virtual bool useFPForAddressing(const MachineFunction &MF) const;

  /// On AA
  bool hasReservedCallFrame(const MachineFunction &MF) const override;

};

} // End llvm namespace

#endif
