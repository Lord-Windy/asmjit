// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../x86/x86func_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86FuncUtils - Helpers]
// ============================================================================

static ASMJIT_INLINE uint32_t x86VecTypeIdToRegType(uint32_t typeId) noexcept {
  return typeId <= TypeId::_kVec128End ? X86Reg::kRegXmm :
         typeId <= TypeId::_kVec256End ? X86Reg::kRegYmm : X86Reg::kRegZmm;
}

// ============================================================================
// [asmjit::X86FuncUtils - CallConv]
// ============================================================================

Error X86FuncUtils::initCallConv(CallConv& cc, uint32_t ccId) noexcept {
  const uint32_t kKindGp  = X86Reg::kKindGp;
  const uint32_t kKindMm  = X86Reg::kKindMm;
  const uint32_t kKindK   = X86Reg::kKindK;
  const uint32_t kKindXyz = X86Reg::kKindXyz;

  const uint32_t kAx = X86Gp::kIdAx;
  const uint32_t kBx = X86Gp::kIdBx;
  const uint32_t kCx = X86Gp::kIdCx;
  const uint32_t kDx = X86Gp::kIdDx;
  const uint32_t kSp = X86Gp::kIdSp;
  const uint32_t kBp = X86Gp::kIdBp;
  const uint32_t kSi = X86Gp::kIdSi;
  const uint32_t kDi = X86Gp::kIdDi;

  switch (ccId) {
    case CallConv::kIdX86StdCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      goto X86CallConv;

    case CallConv::kIdX86MsThisCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kKindGp, kCx);
      goto X86CallConv;

    case CallConv::kIdX86MsFastCall:
    case CallConv::kIdX86GccFastCall:
      cc.setFlags(CallConv::kFlagCalleePopsStack);
      cc.setPassedOrder(kKindGp, kCx, kDx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm1:
      cc.setPassedOrder(kKindGp, kAx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm2:
      cc.setPassedOrder(kKindGp, kAx, kDx);
      goto X86CallConv;

    case CallConv::kIdX86GccRegParm3:
      cc.setPassedOrder(kKindGp, kAx, kDx, kCx);
      goto X86CallConv;

    case CallConv::kIdX86CDecl:
X86CallConv:
      cc.setArchType(Arch::kTypeX86);
      cc.setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, kSi, kDi));
      break;

    case CallConv::kIdX86Win64:
      cc.setArchType(Arch::kTypeX64);
      cc.setAlgorithm(CallConv::kAlgorithmWin64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec | CallConv::kFlagIndirectVecArgs);
      cc.setSpillZoneSize(32);
      cc.setPassedOrder(kKindGp, kCx, kDx, 8, 9);
      cc.setPassedOrder(kKindXyz, 0, 1, 2, 3);
      cc.setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, kSi, kDi, 12, 13, 14, 15));
      cc.setPreservedRegs(kKindXyz, Utils::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
      break;

    case CallConv::kIdX86Unix64:
      cc.setArchType(Arch::kTypeX64);
      cc.setFlags(CallConv::kFlagPassFloatsByVec);
      cc.setRedZoneSize(128);
      cc.setPassedOrder(kKindGp, kDi, kSi, kDx, kCx, 8, 9);
      cc.setPassedOrder(kKindXyz, 0, 1, 2, 3, 4, 5, 6, 7);
      cc.setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, 12, 13, 14, 15));
      break;

    default:
      return DebugUtils::errored(kErrorInvalidArgument);
  }

  cc.setId(ccId);
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncUtils - FuncDecl]
// ============================================================================

Error X86FuncUtils::initFuncDecl(FuncDecl& decl, const FuncSignature& sign, uint32_t gpSize) noexcept {
  const CallConv& cc = decl.getCallConv();
  uint32_t archType = cc.getArchType();

  uint32_t argCount = sign.getArgCount();
  const uint8_t* args = sign.getArgs();

  uint32_t i;

  if (decl.getRetCount() != 0) {
    uint32_t typeId = decl._rets[0].getTypeId();
    switch (typeId) {
      case TypeId::kI64:
      case TypeId::kU64: {
        if (archType == Arch::kTypeX86) {
          // Convert a 64-bit return to two 32-bit returns.
          decl._retCount = 2;
          typeId -= 2;

          // 64-bit value is returned in EDX:EAX on X86.
          decl._rets[0].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdAx);
          decl._rets[1].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdDx);
          break;
        }
        else {
          decl._rets[0].initReg(typeId, X86Gp::kRegGpq, X86Gp::kIdAx);
        }
        break;
      }

      case TypeId::kI8:
      case TypeId::kU8:
      case TypeId::kI16:
      case TypeId::kU16:
      case TypeId::kI32:
      case TypeId::kU32: {
        decl._rets[0].assignToReg(X86Gp::kRegGpd, X86Gp::kIdAx);
        break;
      }

      case TypeId::kF32:
      case TypeId::kF64: {
        uint32_t regType = (archType == Arch::kTypeX86) ? X86Reg::kRegFp : X86Reg::kRegXmm;
        decl._rets[0].assignToReg(regType, 0);
        break;
      }

      case TypeId::kF80: {
        // 80-bit floats are always returned by FP0.
        decl._rets[0].assignToReg(X86Reg::kRegFp, 0);
        break;
      }

      case TypeId::kMmx32:
      case TypeId::kMmx64: {
        // On X64 MM register(s) are returned through XMM or GPQ (Win64).
        uint32_t regType = X86Reg::kRegMm;
        if (archType != Arch::kTypeX86)
          regType = cc.getAlgorithm() == CallConv::kAlgorithmDefault ? X86Reg::kRegXmm : X86Reg::kRegGpq;

        decl._rets[0].assignToReg(regType, 0);
        break;
      }

      default: {
        decl._rets[0].assignToReg(x86VecTypeIdToRegType(typeId), 0);
        break;
      }
    }
  }

  uint32_t stackBase = gpSize;
  uint32_t stackOffset = stackBase + cc._spillZoneSize;

  if (cc.getAlgorithm() == CallConv::kAlgorithmDefault) {
    uint32_t gpzPos = 0;
    uint32_t xyzPos = 0;

    for (i = 0; i < argCount; i++) {
      FuncInOut& arg = decl._args[i];
      uint32_t typeId = arg.getTypeId();

      if (TypeId::isInt(typeId)) {
        uint32_t regId = gpzPos < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[gpzPos] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (typeId <= TypeId::kU32)
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;
          arg.assignToReg(regType, regId);
          decl._usedRegs[X86Reg::kKindGp] |= Utils::mask(regId);
          gpzPos++;
        }
        else {
          uint32_t size = Utils::iMax<uint32_t>(TypeId::sizeOf(typeId), gpSize);
          arg.assignToStack(stackOffset);
          stackOffset += size;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = xyzPos < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindXyz].id[xyzPos] : kInvalidReg;

        // If this is a float, but `floatByVec` is false, we have to pass by stack.
        if (TypeId::isFloat(typeId) && !cc.hasFlag(CallConv::kFlagPassFloatsByVec))
          regId = kInvalidReg;

        if (regId != kInvalidReg) {
          arg.initReg(typeId, x86VecTypeIdToRegType(typeId), regId);
          decl._usedRegs[X86Reg::kKindXyz] |= Utils::mask(regId);
          xyzPos++;
        }
        else {
          int32_t size = TypeId::sizeOf(typeId);
          arg.assignToStack(stackOffset);
          stackOffset += size;
        }
        continue;
      }
    }
  }

  if (cc.getAlgorithm() == CallConv::kAlgorithmWin64) {
    for (i = 0; i < argCount; i++) {
      FuncInOut& arg = decl._args[i];

      uint32_t typeId = arg.getTypeId();
      uint32_t size = TypeId::sizeOf(typeId);

      if (TypeId::isInt(typeId) || TypeId::isMmx(typeId)) {
        uint32_t regId = i < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[i] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (size <= 4 && !TypeId::isMmx(typeId))
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;

          arg.assignToReg(regType, regId);
          decl._usedRegs[X86Reg::kKindGp] |= Utils::mask(regId);
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += gpSize;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = i < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindXyz].id[i] : kInvalidReg;
        if (regId != kInvalidReg && (TypeId::isFloat(typeId) || cc.hasFlag(CallConv::kFlagVectorCall))) {
          uint32_t regType = x86VecTypeIdToRegType(typeId);
          uint32_t regId = cc._passedOrder[X86Reg::kKindXyz].id[i];

          arg.assignToReg(regType, regId);
          decl._usedRegs[X86Reg::kKindXyz] |= Utils::mask(regId);
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += 8; // Always 8 bytes (float/double).
        }
        continue;
      }
    }
  }

  decl._argStackSize = stackOffset - stackBase;
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncUtils - FuncLayout]
// ============================================================================

Error X86FuncUtils::initFuncLayout(FuncLayout& layout, const FuncDecl& decl, const FuncFrame& frame) noexcept {
  layout.reset();

  uint32_t kind;
  uint32_t gpSize = decl.getCallConv().getArchType() == Arch::kTypeX86 ? 4 : 8;

  // Calculate a bit-mask of all registers that must be saved & restored.
  for (kind = 0; kind < CallConv::kNumRegKinds; kind++)
    layout._savedRegs[kind] = frame.getDirtyRegs(kind) & decl.getPreservedRegs(kind);

  // Include EBP|RBP if the function preserves the frame-pointer.
  if (frame.hasPreservedFP()) {
    layout._preservedFP = true;
    layout._savedRegs[X86Reg::kKindGp] |= Utils::mask(X86Gp::kIdBp);
  }

  // Exclude ESP/RSP - this register is never included in saved-regs.
  layout._savedRegs[X86Reg::kKindGp] &= ~Utils::mask(X86Gp::kIdSp);

  // Calculate the final stack alignment.
  uint32_t stackAlignment =
    Utils::iMax<uint32_t>(
      Utils::iMax<uint32_t>(
        frame.getStackFrameAlignment(),
        frame.getCallFrameAlignment()),
      frame.getNaturalStackAlignment());
  layout._stackAlignment = static_cast<uint8_t>(stackAlignment);

  // Calculate if dynamic stack alignment is required. If true the function has
  // to align stack dynamically to match `_stackAlignment` and would require to
  // access its stack-based arguments through `_stackArgsRegId`.
  bool dsa = stackAlignment > frame._naturalStackAlignment && stackAlignment >= 16;
  layout._dynamicAlignment = dsa;

  // This flag describes if the prolog inserter must store the previous ESP|RSP
  // to stack so the epilog inserter can load the stack from it before returning.
  bool dsaSlotUsed = dsa && frame.isNaked();
  layout._dsaSlotUsed = dsaSlotUsed;

  // These two are identical if the function doesn't align its stack dynamically.
  uint32_t stackArgsRegId = frame.getStackArgsRegId();
  if (stackArgsRegId == kInvalidReg)
    stackArgsRegId = X86Gp::kIdSp;

  // Fix stack arguments base-register from ESP|RSP to EBP|RBP in case it was
  // not picked before and the function performs dynamic stack alignment.
  if (dsa && stackArgsRegId == X86Gp::kIdSp)
    stackArgsRegId = X86Gp::kIdBp;

  if (stackArgsRegId != X86Gp::kIdSp)
    layout._savedRegs[X86Reg::kKindGp] |= Utils::mask(stackArgsRegId) & decl.getPreservedRegs(X86Gp::kKindGp);

  layout._stackBaseRegId = X86Gp::kIdSp;
  layout._stackArgsRegId = static_cast<uint8_t>(stackArgsRegId);

  // Setup stack size used to save preserved registers.
  layout._gpStackSize  = Utils::bitCount(layout.getSavedRegs(X86Reg::kKindGp )) * gpSize;
  layout._vecStackSize = Utils::bitCount(layout.getSavedRegs(X86Reg::kKindXyz)) * 16 +
                         Utils::bitCount(layout.getSavedRegs(X86Reg::kKindMm )) *  8 ;

  uint32_t v = 0;                                    // The beginning of the stack frame, aligned to CallFrame alignment.
  v += frame._callFrameSize;                         // Count '_callFrameSize'  <- This is used to call functions.
  v  = Utils::alignTo(v, stackAlignment);            // Align to function's SA

  layout._stackBaseOffset = v;                       // Store '_stackBaseOffset'<- Function's own stack starts here..
  v += frame._stackFrameSize;                        // Count '_stackFrameSize' <- Function's own stack ends here.

  // If the function is aligned, calculate the alignment necessary to store
  // vector registers, and set `FuncFrame::kX86FlagAlignedVecSR` to inform
  // PrologEpilog inserter that it can use instructions to perform aligned
  // stores/loads to save/restore VEC registers.
  if (stackAlignment >= 16 && layout._vecStackSize) {
    v = Utils::alignTo(v, 16);                       // Align '_vecStackOffset'.
    layout._alignedVecSR = true;
  }

  layout._vecStackOffset = v;                        // Store '_vecStackOffset' <- Functions VEC Save|Restore starts here.
  v += layout._vecStackSize;                         // Count '_vecStackSize'   <- Functions VEC Save|Restore ends here.

  if (dsaSlotUsed) {
    layout._dsaSlot = v;                             // Store '_dsaSlot'        <- Old stack pointer is stored here.
    v += gpSize;
  }

  // The return address should be stored after GP save/restore regs. It has
  // the same size as `gpSize` (basically the native register/pointer size).
  // We don't adjust it now as `v` now contains the exact size that the
  // function requires to adjust (call frame + stack frame, vec stack size).
  // The stack (if we consider this size) is misaligned now, as it's always
  // aligned before the function call - when `call()` is executed it pushes
  // the current EIP|RIP onto the stack, and misaligns it by 12 or 8 bytes
  // (depending on the architecture). So count number of bytes needed to align
  // it up to the function's CallFrame (the beginning).
  if (v || frame.hasFlag(FuncFrame::kFlagHasCalls))
    v += Utils::alignDiff(v + layout._gpStackSize + gpSize, stackAlignment);

  layout._stackAdjustment = v;                       // Store '_stackAdjustment'<- SA used by 'add zsp, SA' and 'sub zsp, SA'.
  layout._gpStackOffset = v;                         // Store '_gpStackOffset'  <- Functions GP Save|Restore starts here.
  v += layout._gpStackSize;                          // Count '_gpStackSize'    <- Functions GP Save|Restore ends here.

  v += gpSize;                                       // Count 'ReturnAddress'.
  v += decl.getSpillZoneSize();                      // Count 'SpillZoneSize'.

  // Calculate where function arguments start, relative to the stackArgsRegId.
  // If the register that will be used to access arguments passed by stack is
  // ESP|RSP then it's exactly where we are now, otherwise we must calculate
  // how many 'push regs' we did and adjust it based on that.
  uint32_t stackArgsOffset = v;
  if (stackArgsRegId != X86Gp::kIdSp) {
    if (frame.hasPreservedFP())
      stackArgsOffset = gpSize;                      // Count one 'push'.
    else
      stackArgsOffset = layout._gpStackSize;         // Count the whole 'push' sequence.
  }
  layout._stackArgsOffset = stackArgsOffset;

  // If the function does dynamic stack adjustment then the stack-adjustment
  // must be aligned.
  if (dsa)
    layout._stackAdjustment = Utils::alignTo(layout._stackAdjustment, stackAlignment);

  // Initialize variables based on CallConv flags.
  if (decl.hasFlag(CallConv::kFlagCalleePopsStack))
    layout._calleeStackCleanup = static_cast<uint16_t>(decl.getArgStackSize());

  // Initialize variables based on FuncFrame flags.
  if (frame.hasFlag(FuncFrame::kX86FlagMmxCleanup)) layout._x86MmxCleanup = true;
  if (frame.hasFlag(FuncFrame::kX86FlagAvxCleanup)) layout._x86AvxCleanup = true;

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncUtils - InsertProlog / InsertEpilog]
// ============================================================================

Error X86FuncUtils::insertProlog(X86Emitter* emitter, const FuncLayout& layout) {
  uint32_t i;
  uint32_t regId;

  uint32_t gpSize = emitter->getGpSize();
  uint32_t gpSaved = layout.getSavedRegs(X86Reg::kKindGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).
  X86Gp saReg = emitter->zsp(); // Stack-arguments base register.

  // Emit: 'push zbp'
  //       'mov  zbp, zsp'.
  if (layout.hasPreservedFP()) {
    gpSaved &= ~Utils::mask(X86Gp::kIdBp);
    ASMJIT_PROPAGATE(emitter->push(zbp));
    ASMJIT_PROPAGATE(emitter->mov(zbp, zsp));
  }

  // Emit: 'push gp' sequence.
  if (gpSaved) {
    for (i = gpSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      gpReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->push(gpReg));
    }
  }

  // Emit: 'mov saReg, zsp'.
  uint32_t stackArgsRegId = layout.getStackArgsRegId();
  if (stackArgsRegId != kInvalidReg && stackArgsRegId != X86Gp::kIdSp) {
    saReg.setId(stackArgsRegId);
    if (!(layout.hasPreservedFP() && stackArgsRegId == X86Gp::kIdBp))
      ASMJIT_PROPAGATE(emitter->mov(saReg, zsp));
  }

  // Emit: 'and zsp, StackAlignment'.
  if (layout.hasDynamicAlignment())
    ASMJIT_PROPAGATE(emitter->and_(zsp, -static_cast<int32_t>(layout.getStackAlignment())));

  // Emit: 'sub zsp, StackAdjustment'.
  if (layout.hasStackAdjustment())
    ASMJIT_PROPAGATE(emitter->sub(zsp, layout.getStackAdjustment()));

  // Emit: 'mov [zsp + dsaSlot], saReg'.
  if (layout.hasDynamicAlignment() && layout.hasDsaSlotUsed()) {
    X86Mem saMem = x86::ptr(zsp, layout._dsaSlot);
    ASMJIT_PROPAGATE(emitter->mov(saMem, saReg));
  }

  // Emit 'movaps|movups [zsp + X], xmm0..15'.
  uint32_t xmmSaved = layout.getSavedRegs(X86Reg::kKindXyz);
  if (xmmSaved) {
    X86Mem vecBase = x86::ptr(zsp, layout.getVecStackOffset());
    X86Reg vecReg = x86::xmm(0);

    uint32_t vecInst = layout.hasAlignedVecSR() ? X86Inst::kIdMovaps : X86Inst::kIdMovups;
    uint32_t vecSize = 16;

    for (i = xmmSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      vecReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->emit(vecInst, vecBase, vecReg));
      vecBase.addOffsetLo32(static_cast<int32_t>(vecSize));
    }
  }

  return kErrorOk;
}

Error X86FuncUtils::insertEpilog(X86Emitter* emitter, const FuncLayout& layout) {
  uint32_t i;
  uint32_t regId;

  uint32_t gpSize = emitter->getGpSize();
  uint32_t gpSaved = layout.getSavedRegs(X86Reg::kKindGp);

  X86Gp zsp = emitter->zsp();   // ESP|RSP register.
  X86Gp zbp = emitter->zsp();   // EBP|RBP register.
  zbp.setId(X86Gp::kIdBp);

  X86Gp gpReg = emitter->zsp(); // General purpose register (temporary).

  // Don't emit 'pop zbp' in the pop sequence, this case is handled separately.
  if (layout.hasPreservedFP()) gpSaved &= ~Utils::mask(X86Gp::kIdBp);

  // Emit 'movaps|movups xmm0..15, [zsp + X]'.
  uint32_t xmmSaved = layout.getSavedRegs(X86Reg::kKindXyz);
  if (xmmSaved) {
    X86Mem vecBase = x86::ptr(zsp, layout.getVecStackOffset());
    X86Reg vecReg = x86::xmm(0);

    uint32_t vecInst = layout.hasAlignedVecSR() ? X86Inst::kIdMovaps : X86Inst::kIdMovups;
    uint32_t vecSize = 16;

    for (i = xmmSaved, regId = 0; i; i >>= 1, regId++) {
      if (!(i & 0x1)) continue;
      vecReg.setId(regId);
      ASMJIT_PROPAGATE(emitter->emit(vecInst, vecReg, vecBase));
      vecBase.addOffsetLo32(static_cast<int32_t>(vecSize));
    }
  }

  // Emit 'emms' and 'vzeroupper'.
  if (layout.hasX86MmxCleanup()) ASMJIT_PROPAGATE(emitter->emms());
  if (layout.hasX86AvxCleanup()) ASMJIT_PROPAGATE(emitter->vzeroupper());

  if (layout.hasPreservedFP()) {
    // Emit 'mov zsp, zbp' or 'lea zsp, [zbp - x]'
    int32_t count = static_cast<int32_t>(layout.getGpStackSize() - gpSize);
    if (!count)
      ASMJIT_PROPAGATE(emitter->mov(zsp, zbp));
    else
      ASMJIT_PROPAGATE(emitter->lea(zsp, x86::ptr(zbp, -count)));
  }
  else {
    if (layout.hasDynamicAlignment() && layout.hasDsaSlotUsed()) {
      // Emit 'mov zsp, [zsp + DsaSlot]'.
      X86Mem saMem = x86::ptr(zsp, layout._dsaSlot);
      ASMJIT_PROPAGATE(emitter->mov(zsp, saMem));
    }
    else if (layout.hasStackAdjustment()) {
      // Emit 'add zsp, StackAdjustment'.
      ASMJIT_PROPAGATE(emitter->add(zsp, static_cast<int32_t>(layout.getStackAdjustment())));
    }
  }

  // Emit 'pop gp' sequence.
  if (gpSaved) {
    i = gpSaved;
    regId = 16;

    do {
      regId--;
      if (i & 0x8000) {
        gpReg.setId(regId);
        ASMJIT_PROPAGATE(emitter->pop(gpReg));
      }
      i <<= 1;
    } while (regId != 0);
  }

  // Emit 'pop zbp'.
  if (layout.hasPreservedFP()) ASMJIT_PROPAGATE(emitter->pop(zbp));

  // Emit 'ret' or 'ret x'.
  if (layout.hasCalleeStackCleanup())
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet, static_cast<int>(layout.getCalleeStackCleanup())));
  else
    ASMJIT_PROPAGATE(emitter->emit(X86Inst::kIdRet));

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
