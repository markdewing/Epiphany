//===-- EpiphanyISelDAGToDAG.cpp - A dag to dag inst selector for Epiphany --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the Epiphany target.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "epiphany-isel"
#include "Epiphany.h"
#include "EpiphanyInstrInfo.h"
#include "EpiphanySubtarget.h"
#include "EpiphanyTargetMachine.h"
#include "Utils/EpiphanyBaseInfo.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//===--------------------------------------------------------------------===//
/// Epiphany specific code to select Epiphany machine instructions for
/// SelectionDAG operations.
///
namespace {

class EpiphanyDAGToDAGISel : public SelectionDAGISel {
  EpiphanyTargetMachine &TM;
  const EpiphanyInstrInfo *TII;

  /// Keep a pointer to the EpiphanySubtarget around so that we can
  /// make the right decision when generating code for different targets.
  const EpiphanySubtarget *Subtarget;

public:
  explicit EpiphanyDAGToDAGISel(EpiphanyTargetMachine &tm,
                               CodeGenOpt::Level OptLevel)
    : SelectionDAGISel(tm, OptLevel), TM(tm),
      TII(static_cast<const EpiphanyInstrInfo*>(TM.getInstrInfo())),
      Subtarget(nullptr) {
  }

  virtual const char *getPassName() const {
    return "Epiphany Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    //Subtarget = &MF.getSubtarget<EpiphanySubtarget>();
    TM.resetSubtarget(&MF);
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  // Include the pieces autogenerated from the target description.
#include "EpiphanyGenDAGISel.inc"

  // getImm - Return a target constant with the specified value.
  inline SDValue getImm(const SDNode *Node, unsigned Imm) {
    return CurDAG->getTargetConstant(Imm, SDLoc(Node), Node->getValueType(0));
  }

  template<unsigned MemSize>
  bool SelectOffsetUImm11(SDValue N, SDValue &UImm12) {
	  const ConstantSDNode *CN = dyn_cast<ConstantSDNode>(N);
	  const int maxv = ((2<<10)-1); // = 2^11-1 -> 11 bit
	  if (!CN || CN->getSExtValue() % MemSize != 0 || !(CN->getSExtValue() / MemSize >= -maxv && CN->getSExtValue() / MemSize <= maxv))
		  return false;

	  UImm12 =  CurDAG->getTargetConstant(CN->getSExtValue() / MemSize, SDLoc(N), MVT::i32);
	  return true;
  }

  SDNode *TrySelectToMoveImm(SDNode *N);
  SDNode *LowerToFPLitPool(SDNode *Node);
  SDNode *SelectToLitPool(SDNode *N);

  SDNode* Select(SDNode*);
private:
};
}

SDNode *EpiphanyDAGToDAGISel::TrySelectToMoveImm(SDNode *Node) {
  SDNode *ResNode;
  SDLoc dl(Node);
  EVT DestType = Node->getValueType(0);
  unsigned DestWidth = DestType.getSizeInBits();

	if (DestWidth != 32)
		  llvm_unreachable("move to dst !=32 ?!");

	if(DestType.isInteger()){
		uint64_t BitPat = cast<ConstantSDNode>(Node)->getZExtValue();
		if (BitPat & ~0xffffffffULL){
				llvm_unreachable("wat.");
		}

	// 0 or 16 lower bits
		ResNode = CurDAG->getMachineNode(Epiphany::MOVri, dl, DestType, CurDAG->getTargetConstant(BitPat, dl, MVT::i32));

		if (BitPat & 0xffff0000ULL){// 16 upper bits
			//this is LUi(LLi(val{15-0}), val{31-16})
			ResNode = CurDAG->getMachineNode(Epiphany::MOVTri, dl, DestType, SDValue(ResNode, 0), CurDAG->getTargetConstant(BitPat, dl, MVT::i32));	 
		}
	} else if(DestType.isFloatingPoint()){
		APFloat BitPat = cast<ConstantFPSDNode>(Node)->getValueAPF();

		ResNode = CurDAG->getMachineNode(Epiphany::MOVri_nopat_f, dl, DestType, CurDAG->getTargetConstantFP(BitPat, dl, MVT::f32));
		//this is LUi(LLi(val{15-0}), val{31-16})
		if(!BitPat.isPosZero())
		ResNode = CurDAG->getMachineNode(Epiphany::MOVTri_nopat_f, dl, DestType, SDValue(ResNode, 0), CurDAG->getTargetConstantFP(BitPat, dl, MVT::f32));	 

	}

	ReplaceUses(Node, ResNode);
	return ResNode;
}

SDNode *EpiphanyDAGToDAGISel::SelectToLitPool(SDNode *Node) {
  SDLoc dl(Node);
  const DataLayout &DL = CurDAG->getDataLayout();
  uint64_t UnsignedVal = cast<ConstantSDNode>(Node)->getZExtValue();
  int64_t SignedVal = cast<ConstantSDNode>(Node)->getSExtValue();
  EVT DestType = Node->getValueType(0);
  EVT PtrVT = TLI->getPointerTy(DL);

  // Since we may end up loading a 64-bit constant from a 32-bit entry the
  // constant in the pool may have a different type to the eventual node.
  ISD::LoadExtType Extension;
  EVT MemType;

  assert((DestType == MVT::i64 || DestType == MVT::i32)
         && "Only expect integer constants at the moment");

  if (DestType == MVT::i32) {
    Extension = ISD::NON_EXTLOAD;
    MemType = MVT::i32;
  } else if (UnsignedVal <= UINT32_MAX) {
    Extension = ISD::ZEXTLOAD;
    MemType = MVT::i32;
  } else if (SignedVal >= INT32_MIN && SignedVal <= INT32_MAX) {
    Extension = ISD::SEXTLOAD;
    MemType = MVT::i32;
  } else {
    Extension = ISD::NON_EXTLOAD;
    MemType = MVT::i32;
  }

  Constant *CV = ConstantInt::get(Type::getIntNTy(*CurDAG->getContext(),
                                                  MemType.getSizeInBits()),
                                  UnsignedVal);
  SDValue PoolAddr;
  unsigned Alignment = CurDAG->getDataLayout().getABITypeAlignment(CV->getType());
  PoolAddr = CurDAG->getNode(EpiphanyISD::WrapperSmall, dl, PtrVT,
                             CurDAG->getTargetConstantPool(CV, PtrVT, 0, 0,
                                                         EpiphanyII::MO_HI16),
                             CurDAG->getTargetConstantPool(CV, PtrVT, 0, 0,
                                                           EpiphanyII::MO_LO16),
                             CurDAG->getConstant(Alignment, SDLoc(Node), MVT::i32));

  return CurDAG->getExtLoad(Extension, dl, DestType, CurDAG->getEntryNode(),
                            PoolAddr,
                            MachinePointerInfo::getConstantPool(), MemType,
                            /* isVolatile = */ false,
                            /* isNonTemporal = */ false,
                            /* isInvariant = */ false,
                            Alignment).getNode();
}

SDNode *EpiphanyDAGToDAGISel::LowerToFPLitPool(SDNode *Node) {
  SDLoc dl(Node);
  const DataLayout &DL = CurDAG->getDataLayout();
  const ConstantFP *FV = cast<ConstantFPSDNode>(Node)->getConstantFPValue();
  EVT PtrVT = TLI->getPointerTy(DL);
  EVT DestType = Node->getValueType(0);

  unsigned Alignment = CurDAG->getDataLayout().getABITypeAlignment(FV->getType());
  SDValue PoolAddr;

  assert(TM.getCodeModel() == CodeModel::Small &&
         "Only small code model supported");
  PoolAddr = CurDAG->getNode(EpiphanyISD::WrapperSmall, dl, PtrVT,
                             CurDAG->getTargetConstantPool(FV, PtrVT, 0, 0,
                                                         EpiphanyII::MO_HI16),
                             CurDAG->getTargetConstantPool(FV, PtrVT, 0, 0,
                                                           EpiphanyII::MO_LO16),
                             CurDAG->getConstant(Alignment, dl, MVT::i32));

  return CurDAG->getLoad(DestType, dl, CurDAG->getEntryNode(), PoolAddr,
                         MachinePointerInfo::getConstantPool(),
                         /* isVolatile = */ false,
                         /* isNonTemporal = */ false,
                         /* isInvariant = */ true,
                         Alignment).getNode();
}

SDNode *EpiphanyDAGToDAGISel::Select(SDNode *Node) {
  // Dump information about the Node being selected
  DEBUG(dbgs() << "Selecting: "; Node->dump(CurDAG); dbgs() << "\n");

  if (Node->isMachineOpcode()) {
    DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << "\n");
    return NULL;
  }

  switch (Node->getOpcode()) {
  case ISD::FrameIndex: {
    int FI = cast<FrameIndexSDNode>(Node)->getIndex();
    EVT PtrTy = TLI->getPointerTy(CurDAG->getDataLayout());
    SDValue TFI = CurDAG->getTargetFrameIndex(FI, PtrTy);
    return CurDAG->SelectNodeTo(Node, Epiphany::ADDri, PtrTy,
                                TFI, CurDAG->getTargetConstant(0, SDLoc(Node), PtrTy));
  }
  case ISD::ConstantPool: {
    // Constant pools are fine, just create a Target entry.
    ConstantPoolSDNode *CN = cast<ConstantPoolSDNode>(Node);
    const Constant *C = CN->getConstVal();
    SDValue CP = CurDAG->getTargetConstantPool(C, CN->getValueType(0));

    ReplaceUses(SDValue(Node, 0), CP);
    return NULL;
  }
  case ISD::Constant: {
    SDNode *ResNode = 0;
    ResNode = TrySelectToMoveImm(Node);
    if (ResNode)
      return ResNode;

	 llvm_unreachable("contant fail.");
    //ResNode = SelectToLitPool(Node);
    //assert(ResNode && "We need *some* way to materialise a constant");

    //// We want to continue selection at this point since the litpool access
    //// generated used generic nodes for simplicity.
    //ReplaceUses(SDValue(Node, 0), SDValue(ResNode, 0));
    //Node = ResNode;
    //break;
  }
  case ISD::ConstantFP: {
    //SDNode *ResNode = LowerToFPLitPool(Node);
    //ReplaceUses(SDValue(Node, 0), SDValue(ResNode, 0));

    //// We want to continue selection at this point since the litpool access
    //// generated used generic nodes for simplicity.
    //Node = ResNode;
    //break;
	return TrySelectToMoveImm(Node);
  }
  default:
    break; // Let generic code handle it
  }

  SDNode *ResNode = SelectCode(Node);

  DEBUG(dbgs() << "=> ";
        if (ResNode == NULL || ResNode == Node)
          Node->dump(CurDAG);
        else
          ResNode->dump(CurDAG);
        dbgs() << "\n");

  return ResNode;
}

/// This pass converts a legalized DAG into a Epiphany-specific DAG, ready for
/// instruction scheduling.
FunctionPass *llvm::createEpiphanyISelDAG(EpiphanyTargetMachine &TM,
                                         CodeGenOpt::Level OptLevel) {
  return new EpiphanyDAGToDAGISel(TM, OptLevel);
}
