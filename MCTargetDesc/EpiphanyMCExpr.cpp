//===-- EpiphanyMCExpr.cpp - Epiphany specific MC expression classes --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the assembly expression modifiers
// accepted by the Epiphany architecture (e.g. ":lo12:", ":gottprel_g1:", ...).
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "epiphanymcexpr"
#include "EpiphanyMCExpr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Object/ELF.h"

using namespace llvm;

const EpiphanyMCExpr*
EpiphanyMCExpr::Create(VariantKind Kind, const MCExpr *Expr,
                      MCContext &Ctx) {
  return new (Ctx) EpiphanyMCExpr(Kind, Expr);
}

void EpiphanyMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  switch (Kind) {
  default: llvm_unreachable("Invalid kind!");
  case VK_EPIPHANY_LO16:             OS << "%low("; break;
  case VK_EPIPHANY_HI16:             OS << "%high("; break;
  }

  const MCExpr *Expr = getSubExpr();
  if (Expr->getKind() != MCExpr::SymbolRef)
    OS << '(';
  Expr->print(OS, MAI);
  if (Expr->getKind() != MCExpr::SymbolRef)
    OS << ')';

	switch (Kind) {
		case VK_EPIPHANY_LO16:
		case VK_EPIPHANY_HI16:             OS << ")";
		default: break;
	}
}

void EpiphanyMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
    Streamer.visitUsedExpr(*getSubExpr());
}

MCSection *EpiphanyMCExpr::findAssociatedSection() const {
    llvm_unreachable("TODO: add code");
}

bool
EpiphanyMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                         const MCAsmLayout *Layout,
                                         const MCFixup *Fixup) const {
  return getSubExpr()->evaluateAsRelocatable(Res, Layout, Fixup);
}


void EpiphanyMCExpr::fixELFSymbolsInTLSFixups(MCAssembler &Asm) const {
}

// FIXME: This basically copies MCObjectStreamer::AddValueSymbols. Perhaps
// that method should be made public?
// FIXME: really do above: now that two backends are using it.
static void AddValueSymbolsImpl(const MCExpr *Value, MCAssembler *Asm) {
  switch (Value->getKind()) {
  case MCExpr::Target:
    llvm_unreachable("Can't handle nested target expr!");
    break;

  case MCExpr::Constant:
    break;

  case MCExpr::Binary: {
    const MCBinaryExpr *BE = cast<MCBinaryExpr>(Value);
    AddValueSymbolsImpl(BE->getLHS(), Asm);
    AddValueSymbolsImpl(BE->getRHS(), Asm);
    break;
  }

  case MCExpr::SymbolRef:
    //TODO: Figure out what this should do
    //Asm->getOrCreateSymbolData(cast<MCSymbolRefExpr>(Value)->getSymbol());
    break;

  case MCExpr::Unary:
    AddValueSymbolsImpl(cast<MCUnaryExpr>(Value)->getSubExpr(), Asm);
    break;
  }
}

void EpiphanyMCExpr::AddValueSymbols(MCAssembler *Asm) const {
  AddValueSymbolsImpl(getSubExpr(), Asm);
}
