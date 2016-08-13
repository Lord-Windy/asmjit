// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER) && defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../x86/x86compiler.h"
#include "../x86/x86func.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86FuncDecl - Helpers]
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

// ============================================================================
// [asmjit::X86FuncDecl - SetSignature]
// ============================================================================

static Error X86FuncDecl_initFunc(X86FuncDecl* self, uint32_t arch,
  uint32_t ret, const uint8_t* args, uint32_t argCount) noexcept {

  ASMJIT_ASSERT(argCount <= kFuncArgCount);

  const CallConv& cc = self->_callConv;
  uint32_t ccId = cc.getId();

  uint32_t i = 0;
  uint32_t gpSize = (arch == Arch::kTypeX86) ? 4 : 8;
  uint32_t deabstractDelta = TypeId::deabstractDeltaOfSize(gpSize);

  self->_argCount = static_cast<uint8_t>(argCount);
  for (i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = self->getArg(i);
    arg.initTypeId(TypeId::deabstract(args[i], deabstractDelta));
  }

  if (TypeId::isValid(ret)) {
    ret = TypeId::deabstract(ret, deabstractDelta);
    switch (ret) {
      case TypeId::kI64:
      case TypeId::kU64: {
        if (arch == Arch::kTypeX86) {
          // 64-bit value is returned in EDX:EAX on X86.
          uint32_t typeId = ret - 2;
          self->_retCount = 2;
          self->_rets[0].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdAx);
          self->_rets[1].initReg(typeId, X86Gp::kRegGpd, X86Gp::kIdDx);
          break;
        }
        else {
          self->_retCount = 1;
          self->_rets[0].initReg(ret, X86Gp::kRegGpq, X86Gp::kIdAx);
        }
        break;
      }

      case TypeId::kI8:
      case TypeId::kU8:
      case TypeId::kI16:
      case TypeId::kU16:
      case TypeId::kI32:
      case TypeId::kU32: {
        self->_retCount = 1;
        self->_rets[0].initReg(ret, X86Gp::kRegGpd, X86Gp::kIdAx);
        break;
      }

      case TypeId::kF32:
      case TypeId::kF64: {
        uint32_t regType = (arch == Arch::kTypeX86) ? X86Reg::kRegFp : X86Reg::kRegXmm;
        self->_retCount = 1;
        self->_rets[0].initReg(ret, regType, 0);
        break;
      }

      case TypeId::kF80: {
        // 80-bit floats are always returned by FP0.
        self->_retCount = 1;
        self->_rets[0].initReg(ret, X86Reg::kRegFp, 0);
        break;
      }

      case TypeId::kMmx32:
      case TypeId::kMmx64: {
        // On X64 MM register(s) are returned through XMM or GPQ (Win64).
        uint32_t regType = X86Reg::kRegMm;
        if (arch != Arch::kTypeX86)
          regType = cc.getAlgorithm() == CallConv::kAlgorithmDefault ? X86Reg::kRegXmm : X86Reg::kRegGpq;

        self->_retCount = 1;
        self->_rets[0].initReg(ret, regType, 0);
        break;
      }

      default: {
        uint32_t regType = x86VecTypeIdToRegType(ret);
        self->_retCount = 1;
        self->_rets[0].initReg(ret, regType, 0);
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
      FuncInOut& arg = self->getArg(i);
      uint32_t typeId = arg.getTypeId();

      if (TypeId::isInt(typeId)) {
        uint32_t regId = gpzPos < CallConv::kMaxRegArgsPerKind ? cc._group[X86Reg::kKindGp].passed[gpzPos] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (typeId <= TypeId::kU32)
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;
          arg.assignToReg(regType, regId);
          self->_usedMask[X86Reg::kKindGp] |= Utils::mask(regId);
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
        uint32_t regId = xyzPos < CallConv::kMaxRegArgsPerKind ? cc._group[X86Reg::kKindXyz].passed[xyzPos] : kInvalidReg;

        // If this is a float, but `floatByVec` is false, we have to pass by stack.
        if (TypeId::isFloat(typeId) && !cc.floatByVec()) regId = kInvalidReg;

        if (regId != kInvalidReg) {
          arg.initReg(typeId, x86VecTypeIdToRegType(typeId), regId);
          self->_usedMask[X86Reg::kKindXyz] |= Utils::mask(regId);
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
      FuncInOut& arg = self->getArg(i);

      uint32_t typeId = arg.getTypeId();
      uint32_t size = TypeId::sizeOf(typeId);

      if (TypeId::isInt(typeId) || TypeId::isMmx(typeId)) {
        uint32_t regId = i < CallConv::kMaxRegArgsPerKind ? cc._group[X86Reg::kKindGp].passed[i] : kInvalidReg;
        if (regId != kInvalidReg) {
          uint32_t regType = (size <= 4 && !TypeId::isMmx(typeId))
            ? X86Reg::kRegGpd
            : X86Reg::kRegGpq;
          arg.assignToReg(regType, regId);
          self->_usedMask[X86Reg::kKindGp] |= Utils::mask(regId);
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += gpSize;
        }
        continue;
      }

      if (TypeId::isFloat(typeId) || TypeId::isVec(typeId)) {
        uint32_t regId = i < CallConv::kMaxRegArgsPerKind ? cc._group[X86Reg::kKindXyz].passed[i] : kInvalidReg;
        if (regId != kInvalidReg && (TypeId::isFloat(typeId) || cc.isVectorCall())) {
          uint32_t regType = x86VecTypeIdToRegType(typeId);
          uint32_t regId = cc._group[X86Reg::kKindXyz].passed[i];
          arg.assignToReg(regType, regId);
          self->_usedMask[X86Reg::kKindXyz] |= Utils::mask(regId);
        }
        else {
          arg.assignToStack(stackOffset);
          stackOffset += 8; // Always 8 bytes (float/double).
        }
        continue;
      }
    }
  }

  self->_argStackSize = stackOffset - stackBase;
  return kErrorOk;
}

Error X86FuncDecl::setSignature(const FuncSignature& p) {
  uint32_t ccId = p.getCallConv();
  uint32_t arch = x86GetArchFromCConv(ccId);

  if (arch != Arch::kTypeX86 && arch != Arch::kTypeX64)
    return DebugUtils::errored(kErrorInvalidArch);

  if (p.getArgCount() > kFuncArgCount)
    return DebugUtils::errored(kErrorInvalidArgument);

  ASMJIT_PROPAGATE(this->_callConv.init(ccId));
  ASMJIT_PROPAGATE(X86FuncDecl_initFunc(this, arch, p.getRet(), p.getArgs(), p.getArgCount()));

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && ASMJIT_BUILD_X86
