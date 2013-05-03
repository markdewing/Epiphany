//===- EpiphanyInstrInfo.h - Epiphany Instruction Information -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Epiphany implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_EPIPHANYINSTRINFO_H
#define LLVM_TARGET_EPIPHANYINSTRINFO_H

#include "llvm/Target/TargetInstrInfo.h"
#include "EpiphanyRegisterInfo.h"

#define GET_INSTRINFO_HEADER
#include "EpiphanyGenInstrInfo.inc"

namespace llvm {

class EpiphanySubtarget;

class EpiphanyInstrInfo : public EpiphanyGenInstrInfo {
  const EpiphanyRegisterInfo RI;
  const EpiphanySubtarget &Subtarget;
public:
  explicit EpiphanyInstrInfo(const EpiphanySubtarget &TM);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const TargetRegisterInfo &getRegisterInfo() const { return RI; }

  const EpiphanySubtarget &getSubTarget() const { return Subtarget; }

  void copyPhysReg(MachineBasicBlock &MBB,
                   MachineBasicBlock::iterator I, DebugLoc DL,
                   unsigned DestReg, unsigned SrcReg,
                   bool KillSrc) const;

  MachineInstr *emitFrameIndexDebugValue(MachineFunction &MF, int FrameIx,
                                         uint64_t Offset, const MDNode *MDPtr,
                                         DebugLoc DL) const;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI,
                           unsigned SrcReg, bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const;
  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            unsigned DestReg, int FrameIdx,
                            const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const;

  bool analyzeCompare(const MachineInstr *MI, unsigned &SrcReg,
                              unsigned &SrcReg2,
                              int &CmpMask, int &CmpValue) const;

  bool optimizeCompareInstr(MachineInstr *CmpInstr, unsigned SrcReg,
                                    unsigned SrcReg2, int CmpMask, int CmpValue,
                                    const MachineRegisterInfo *MRI) const;


  bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const;
  unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB,
                        const SmallVectorImpl<MachineOperand> &Cond,
                        DebugLoc DL) const;
  unsigned RemoveBranch(MachineBasicBlock &MBB) const;

  bool expandPostRAPseudo(MachineBasicBlock::iterator MI) const;

  /// Look through the instructions in this function and work out the largest
  /// the stack frame can be while maintaining the ability to address local
  /// slots with no complexities.
  unsigned estimateRSStackLimit(MachineFunction &MF) const;

  /// getAddressConstraints - For loads and stores (and PRFMs) taking an
  /// immediate offset, this function determines the constraints required for
  /// the immediate. It must satisfy:
  ///    + MinOffset <= imm <= MaxOffset
  ///    + imm % OffsetScale == 0
  void getAddressConstraints(const MachineInstr &MI, int &AccessScale,
                             int &MinOffset, int &MaxOffset) const;


  unsigned getInstSizeInBytes(const MachineInstr &MI) const;

  unsigned getInstBundleLength(const MachineInstr &MI) const;

};

bool rewriteA64FrameIndex(MachineInstr &MI, unsigned FrameRegIdx,
                          unsigned FrameReg, int &Offset,
                          const EpiphanyInstrInfo &TII);


void EPIPHemitRegUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   DebugLoc dl, const TargetInstrInfo &TII,
                   unsigned DstReg, unsigned SrcReg, unsigned ScratchReg,
                   int64_t NumBytes,
                   MachineInstr::MIFlag MIFlags = MachineInstr::NoFlags);

void EPIPHemitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                  DebugLoc dl, const TargetInstrInfo &TII,
                  unsigned ScratchReg, int64_t NumBytes,
                  MachineInstr::MIFlag MIFlags = MachineInstr::NoFlags);

}

#endif