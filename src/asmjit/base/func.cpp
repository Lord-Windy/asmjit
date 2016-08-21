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
#include "../x86/x86func_p.h"
#include "../x86/x86operand.h"
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
#include "../arm/armfunc_p.h"
#include "../arm/armoperand.h"
#endif // ASMJIT_BUILD_ARM

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::Func - Helpers]
// ============================================================================

static ASMJIT_INLINE bool CallConv_isX86(uint32_t ccId) noexcept {
  return ccId >= CallConv::_kIdX86Start && ccId <= CallConv::_kIdX64End;
}

static ASMJIT_INLINE bool CallConv_isArm(uint32_t ccId) noexcept {
  return ccId >= CallConv::_kIdArmStart && ccId <= CallConv::_kIdArmEnd;
}

// ============================================================================
// [asmjit::CallConv - Init / Reset]
// ============================================================================

Error CallConv::init(uint32_t ccId) noexcept {
  reset();

#if defined(ASMJIT_BUILD_X86)
  if (CallConv_isX86(ccId))
    return X86FuncUtils::initCallConv(*this, ccId);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv_isArm(ccId))
    return ArmFuncUtils::initCallConv(*this, ccId);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncDecl - Init / Reset]
// ============================================================================

Error FuncDecl::init(const FuncSignature& sign) {
  uint32_t ccId = sign.getCallConv();
  CallConv& cc = _callConv;

  uint32_t argCount = sign.getArgCount();
  if (ASMJIT_UNLIKELY(argCount > kFuncArgCount))
    return DebugUtils::errored(kErrorInvalidArgument);

  ASMJIT_PROPAGATE(cc.init(ccId));

  uint32_t gpSize = (cc.getArchType() == Arch::kTypeX86) ? 4 : 8;
  uint32_t deabstractDelta = TypeId::deabstractDeltaOfSize(gpSize);

  const uint8_t* args = sign.getArgs();
  for (uint32_t i = 0; i < static_cast<int32_t>(argCount); i++) {
    FuncInOut& arg = _args[i];
    arg.initTypeId(TypeId::deabstract(args[i], deabstractDelta));
  }
  _argCount = static_cast<uint8_t>(argCount);

  uint32_t ret = sign.getRet();
  if (ret != TypeId::kVoid) {
    _rets[0].initTypeId(TypeId::deabstract(ret, deabstractDelta));
    _retCount = 1;
  }

#if defined(ASMJIT_BUILD_X86)
  if (CallConv_isX86(ccId))
    return X86FuncUtils::initFuncDecl(*this, sign, gpSize);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv_isArm(ccId))
    return ArmFuncUtils::initFuncDecl(*this, sign, gpSize);
#endif // ASMJIT_BUILD_ARM

  // We should never bubble here as if `cc.init()` succeeded then there has to
  // be an implementation for the current architecture. However, stay safe.
  return DebugUtils::errored(kErrorInvalidArgument);
}

// ============================================================================
// [asmjit::FuncUtils]
// ============================================================================

Error FuncLayout::init(const FuncDecl& decl, const FuncFrameInfo& ffi) noexcept {
  uint32_t ccId = decl.getCallConv().getId();

#if defined(ASMJIT_BUILD_X86)
  if (CallConv_isX86(ccId))
    return X86FuncUtils::initFuncLayout(*this, decl, ffi);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (CallConv_isArm(ccId))
    return ArmFuncUtils::initFuncLayout(*this, decl, ffi);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

Error FuncUtils::insertProlog(CodeEmitter* emitter, const FuncLayout& layout) {
#if defined(ASMJIT_BUILD_X86)
  if (emitter->getArch().isX86Family())
    return X86FuncUtils::insertProlog(reinterpret_cast<X86Emitter*>(emitter), layout);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (emitter->getArch().isArmFamily())
    return ArmFuncUtils::insertProlog(reinterpret_cast<ArmEmitter*>(emitter), layout);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

Error FuncUtils::insertEpilog(CodeEmitter* emitter, const FuncLayout& layout) {
#if defined(ASMJIT_BUILD_X86)
  if (emitter->getArch().isX86Family())
    return X86FuncUtils::insertEpilog(reinterpret_cast<X86Emitter*>(emitter), layout);
#endif // ASMJIT_BUILD_X86

#if defined(ASMJIT_BUILD_ARM)
  if (emitter->getArch().isArmFamily())
    return ArmFuncUtils::insertEpilog(reinterpret_cast<ArmEmitter*>(emitter), layout);
#endif // ASMJIT_BUILD_ARM

  return DebugUtils::errored(kErrorInvalidArgument);
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
