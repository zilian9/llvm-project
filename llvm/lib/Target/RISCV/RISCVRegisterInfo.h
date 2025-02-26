//===-- RISCVRegisterInfo.h - RISCV Register Information Impl ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the RISCV implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVREGISTERINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVREGISTERINFO_H

#include "VentusProgramInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "RISCVGenRegisterInfo.inc"

namespace llvm {

// This needs to be kept in sync with the field bits in VentusRegisterClass.
enum RISCVRCFlags {
  IsVGPR = 1 << 0,
  IsSGPR = 1 << 1,
  IsFGPR = 1 << 2
}; // enum RISCVRCFlags

struct RISCVRegisterInfo : public RISCVGenRegisterInfo {

  RISCVRegisterInfo(unsigned HwMode);

  /// \returns true if this class contains VGPR registers.
  static bool hasVGPRs(const TargetRegisterClass *RC) {
    return RC->TSFlags & RISCVRCFlags::IsVGPR;
  }

  /// \returns true if this class contains SGPR registers.
  static bool hasSGPRs(const TargetRegisterClass *RC) {
    return RC->TSFlags & RISCVRCFlags::IsSGPR;
  }

  /// \returns true if this class contains FGPR registers.
  static bool hasFGPRs(const TargetRegisterClass *RC) {
    return RC->TSFlags & RISCVRCFlags::IsFGPR;
  }

  /// Return the 'base' register class for this register.
  /// e.g. X5 => SReg_32, V3 => VGPR_32, X5_X6 -> SReg_32, etc.
  const TargetRegisterClass *getPhysRegClass(MCRegister Reg) const;

  /// \returns true if this class contains only SGPR registers
  static bool isSGPRClass(const TargetRegisterClass *RC) {
    return hasSGPRs(RC) && !hasVGPRs(RC) && !hasFGPRs(RC);
  }

  static bool isFPRClass(const TargetRegisterClass *RC) {
    return hasFGPRs(RC) && !hasVGPRs(RC) && !hasSGPRs(RC);
  }

  /// \returns true if this class ID contains only SGPR registers
  bool isSGPRClassID(unsigned RCID) const {
    return isSGPRClass(getRegClass(RCID));
  }

  bool isSGPRReg(const MachineRegisterInfo &MRI, Register Reg) const;

  void insertRegToSet(const MachineRegisterInfo &MRI,
                      DenseSet<unsigned int> *CurrentRegUsageSet,
                      SubVentusProgramInfo *CurrentSubProgramInfo,
                      Register Reg) const;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool isAsmClobberable(const MachineFunction &MF,
                        MCRegister PhysReg) const override;

  bool isDivergentRegClass(const TargetRegisterClass *RC) const override {
    return !isSGPRClass(RC);
  }

  const uint32_t *getNoPreservedMask() const override;

  bool hasReservedSpillSlot(const MachineFunction &MF, Register Reg,
                            int &FrameIdx) const override;

  // Update DestReg to have the value SrcReg plus an offset.  This is
  // used during frame layout, and we may need to ensure that if we
  // split the offset internally that the DestReg is always aligned,
  // assuming that source reg was.
  void adjustReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator II,
                 const DebugLoc &DL, Register DestReg, Register SrcReg,
                 StackOffset Offset, MachineInstr::MIFlag Flag,
                 MaybeAlign RequiredAlign) const;

  /// Adjust private memory offset which is supposed to be simm11, when offset is
  /// beyond the range, we need to legalize the offset
  void adjustPriMemRegOffset(MachineFunction &MF, MachineBasicBlock &MBB,
          MachineInstr &MI, int64_t offset, Register PriMemReg,
                                  unsigned FIOperandNum) const;

  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;

  /// In Ventus, private memory access are based on TP, but the memory access
  /// instructions are based on VGPR, we need to define a VGPR register for
  /// private memory access
  const Register getPrivateMemoryBaseRegister(const MachineFunction &MF) const;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }

  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override {
    return true;
  }

  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF,
                     unsigned Kind = 0) const override {
    return &RISCV::GPRRegClass;
  }

  const TargetRegisterClass *
  getLargestLegalSuperClass(const TargetRegisterClass *RC,
                            const MachineFunction &) const override;

  void getOffsetOpcodes(const StackOffset &Offset,
                        SmallVectorImpl<uint64_t> &Ops) const override;

  MCRegister findUnusedRegister(const MachineRegisterInfo &MRI,
                                const TargetRegisterClass *RC,
                                const MachineFunction &MF,
                                bool ReserveHighestVGPR = false) const;

  unsigned getRegisterCostTableIndex(const MachineFunction &MF) const override;

  bool getRegAllocationHints(Register VirtReg, ArrayRef<MCPhysReg> Order,
                             SmallVectorImpl<MCPhysReg> &Hints,
                             const MachineFunction &MF, const VirtRegMap *VRM,
                             const LiveRegMatrix *Matrix) const override;
};
} // namespace llvm

#endif
