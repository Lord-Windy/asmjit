// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/func.h"

#if defined(ASMJIT_BUILD_X86)
#include "../x86/x86operand.h"
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
#include "../arm/armoperand.h"
#endif // ASMJIT_BUILD_ARM

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CallConv - Init / Reset]
// ============================================================================

Error CallConv::init(uint32_t ccId) noexcept {
  reset();

#if defined(ASMJIT_BUILD_X86)
  if (ccId >= _kIdX86Start && ccId <= _kIdX64End) {
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
      case kIdX86StdCall:
        setFlags(kFlagCalleePopsStack);
        goto X86CallConv;

      case kIdX86MsThisCall:
        setFlags(kFlagCalleePopsStack);
        setPassedOrder(kKindGp, kCx);
        goto X86CallConv;

      case kIdX86MsFastCall:
      case kIdX86GccFastCall:
        setFlags(kFlagCalleePopsStack);
        setPassedOrder(kKindGp, kCx, kDx);
        goto X86CallConv;

      case kIdX86GccRegParm1:
        setPassedOrder(kKindGp, kAx);
        goto X86CallConv;

      case kIdX86GccRegParm2:
        setPassedOrder(kKindGp, kAx, kDx);
        goto X86CallConv;

      case kIdX86GccRegParm3:
        setPassedOrder(kKindGp, kAx, kDx, kCx);
        goto X86CallConv;

      case kIdX86CDecl:
X86CallConv:
        setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, kSi, kDi));
        break;

      case kIdX86Win64:
        setAlgorithm(kAlgorithmWin64);
        setFlags(kFlagPassFloatsByVec | kFlagIndirectVecArgs);
        setSpillZoneSize(32);
        setPassedOrder(kKindGp, kCx, kDx, 8, 9);
        setPassedOrder(kKindXyz, 0, 1, 2, 3);
        setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, kSi, kDi, 12, 13, 14, 15));
        setPreservedRegs(kKindXyz, Utils::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
        break;

      case kIdX86Unix64:
        setFlags(kFlagPassFloatsByVec);
        setRedZoneSize(128);
        setPassedOrder(kKindGp, kDi, kSi, kDx, kCx, 8, 9);
        setPassedOrder(kKindXyz, 0, 1, 2, 3, 4, 5, 6, 7);
        setPreservedRegs(kKindGp, Utils::mask(kBx, kSp, kBp, 12, 13, 14, 15));
        break;

      default:
        return DebugUtils::errored(kErrorInvalidArgument);
    }

    _id = static_cast<uint8_t>(ccId);
    return kErrorOk;
  }
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncDecl - Init / Reset]
// ============================================================================

//! Get an architecture depending on the calling convention `callConv`.
static ASMJIT_INLINE uint32_t x86GetArchFromCConv(uint32_t ccId) noexcept {
  if (Utils::inInterval<uint32_t>(ccId, CallConv::_kIdX86Start, CallConv::_kIdX86End)) return Arch::kTypeX86;
  if (Utils::inInterval<uint32_t>(ccId, CallConv::_kIdX64Start, CallConv::_kIdX64End)) return Arch::kTypeX64;

  return Arch::kTypeNone;
}

static ASMJIT_INLINE uint32_t x86VecTypeIdToRegType(uint32_t typeId) noexcept {
  return typeId <= TypeId::_kVec128End ? X86Reg::kRegXmm :
         typeId <= TypeId::_kVec256End ? X86Reg::kRegYmm : X86Reg::kRegZmm;
}

Error FuncDecl::init(const FuncSignature& sign) {
  uint32_t ccId = sign.getCallConv();
  CallConv& cc = _callConv;

  uint32_t arch = x86GetArchFromCConv(ccId);
  uint32_t argCount = sign.getArgCount();

  if (argCount > kFuncArgCount)
    return DebugUtils::errored(kErrorInvalidArgument);

  ASMJIT_PROPAGATE(cc.init(ccId));

  uint32_t ret = sign.getRet();
  const uint8_t* args = sign.getArgs();

  uint32_t i = 0;
  uint32_t gpSize = (arch == Arch::kTypeX86) ? 4 : 8;
  uint32_t deabstractDelta = TypeId::deabstractDeltaOfSize(gpSize);

  _argCount = static_cast<uint8_t>(argCount);
  for (i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = _args[i];
    arg.initTypeId(TypeId::deabstract(args[i], deabstractDelta));
  }

#if defined(ASMJIT_BUILD_X86)
  if (TypeId::isValid(ret)) {
    ret = TypeId::deabstract(ret, deabstractDelta);
    switch (ret) {
      case TypeId::kI64:
      case TypeId::kU64: {
        if (arch == Arch::kTypeX86) {
          // 64-bit value is returned in EDX:EAX on X86.
          uint32_t typeId = ret - 2;
          _retCount = 2;
          _rets[0].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdAx);
          _rets[1].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdDx);
          break;
        }
        else {
          _retCount = 1;
          _rets[0].initReg(ret, X86Gp::kRegGpq, X86Gp::kIdAx);
        }
        break;
      }

      case TypeId::kI8:
      case TypeId::kU8:
      case TypeId::kI16:
      case TypeId::kU16:
      case TypeId::kI32:
      case TypeId::kU32: {
        _retCount = 1;
        _rets[0].initReg(ret, X86Gp::kRegGpd, X86Gp::kIdAx);
        break;
      }

      case TypeId::kF32:
      case TypeId::kF64: {
        uint32_t regType = (arch == Arch::kTypeX86) ? X86Reg::kRegFp : X86Reg::kRegXmm;
        _retCount = 1;
        _rets[0].initReg(ret, regType, 0);
        break;
      }

      case TypeId::kF80: {
        // 80-bit floats are always returned by FP0.
        _retCount = 1;
        _rets[0].initReg(ret, X86Reg::kRegFp, 0);
        break;
      }

      case TypeId::kMmx32:
      case TypeId::kMmx64: {
        // On X64 MM register(s) are returned through XMM or GPQ (Win64).
        uint32_t regType = X86Reg::kRegMm;
        if (arch != Arch::kTypeX86)
          regType = cc.getAlgorithm() == CallConv::kAlgorithmDefault ? X86Reg::kRegXmm : X86Reg::kRegGpq;

        _retCount = 1;
        _rets[0].initReg(ret, regType, 0);
        break;
      }

      default: {
        uint32_t regType = x86VecTypeIdToRegType(ret);
        _retCount = 1;
        _rets[0].initReg(ret, regType, 0);
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
      FuncInOut& arg = _args[i];
      uint32_t typeId = arg.getTypeId();

      if (TypeId::isInt(typeId)) {
        uint32_t regId = gpzPos < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[gpzPos] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (typeId <= TypeId::kU32)
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;
          arg.assignToReg(regType, regId);
          _usedRegs[X86Reg::kKindGp] |= Utils::mask(regId);
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
          _usedRegs[X86Reg::kKindXyz] |= Utils::mask(regId);
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
      FuncInOut& arg = _args[i];

      uint32_t typeId = arg.getTypeId();
      uint32_t size = TypeId::sizeOf(typeId);

      if (TypeId::isInt(typeId) || TypeId::isMmx(typeId)) {
        uint32_t regId = i < CallConv::kNumRegArgsPerKind ? cc._passedOrder[X86Reg::kKindGp].id[i] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (size <= 4 && !TypeId::isMmx(typeId))
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;

          arg.assignToReg(regType, regId);
          _usedRegs[X86Reg::kKindGp] |= Utils::mask(regId);
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
          _usedRegs[X86Reg::kKindXyz] |= Utils::mask(regId);
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += 8; // Always 8 bytes (float/double).
        }
        continue;
      }
    }
  }

  _argStackSize = stackOffset - stackBase;
  return kErrorOk;
#endif // ASMJIT_BUILD_X86
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
