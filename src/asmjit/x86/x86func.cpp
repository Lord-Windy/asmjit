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

static ASMJIT_INLINE uint32_t x86ArgTypeToXmmType(uint32_t aType) {
  if (aType == VirtType::kIdF32) return VirtType::kIdX86XmmSs;
  if (aType == VirtType::kIdF64) return VirtType::kIdX86XmmSd;
  return aType;
}

//! Get an architecture depending on the calling convention `callConv`.
//!
//! Returns `ArchId`.
static ASMJIT_INLINE uint32_t x86GetArchFromCConv(uint32_t callConv) {
  if (Utils::inInterval<uint32_t>(callConv, _kCallConvX86Start, _kCallConvX86End)) return Arch::kTypeX86;
  if (Utils::inInterval<uint32_t>(callConv, _kCallConvX64Start, _kCallConvX64End)) return Arch::kTypeX64;

  return Arch::kTypeNone;
}

// ============================================================================
// [asmjit::X86FuncDecl - SetPrototype]
// ============================================================================

static uint32_t X86FuncDecl_initConv(X86FuncDecl* self, uint32_t arch, uint32_t callConv) {
  // Setup defaults.
  self->_argStackSize = 0;
  self->_redZoneSize = 0;
  self->_spillZoneSize = 0;

  self->_callConv = static_cast<uint8_t>(callConv);
  self->_calleePopsStack = false;
  self->_argsDirection = kFuncDirRTL;

  self->_passed.reset();
  self->_preserved.reset();

  ::memset(self->_passedOrderGp, kInvalidReg, ASMJIT_ARRAY_SIZE(self->_passedOrderGp));
  ::memset(self->_passedOrderXyz, kInvalidReg, ASMJIT_ARRAY_SIZE(self->_passedOrderXyz));

  const int32_t kAx = X86Gp::kIdAx;
  const int32_t kBx = X86Gp::kIdBx;
  const int32_t kCx = X86Gp::kIdCx;
  const int32_t kDx = X86Gp::kIdDx;
  const int32_t kSp = X86Gp::kIdSp;
  const int32_t kBp = X86Gp::kIdBp;
  const int32_t kSi = X86Gp::kIdSi;
  const int32_t kDi = X86Gp::kIdDi;

  // --------------------------------------------------------------------------
  // [X86 Support]
  // --------------------------------------------------------------------------

  if (arch == Arch::kTypeX86) {
    self->_preserved.set(X86Reg::kClassGp, Utils::mask(kBx, kSp, kBp, kSi, kDi));

    switch (callConv) {
      case kCallConvX86CDecl:
        return kErrorOk;

      case kCallConvX86StdCall:
        self->_calleePopsStack = true;
        return kErrorOk;

      case kCallConvX86MsThisCall:
        self->_calleePopsStack = true;
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kCx));
        self->_passedOrderGp[0] = kCx;
        return kErrorOk;

      case kCallConvX86MsFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kCx, kCx));
        self->_passedOrderGp[0] = kCx;
        self->_passedOrderGp[1] = kDx;
        return kErrorOk;

      case kCallConvX86BorlandFastCall:
        self->_calleePopsStack = true;
        self->_argsDirection = kFuncDirLTR;
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kAx, kDx, kCx));
        self->_passedOrderGp[0] = kAx;
        self->_passedOrderGp[1] = kDx;
        self->_passedOrderGp[2] = kCx;
        return kErrorOk;

      case kCallConvX86GccFastCall:
        self->_calleePopsStack = true;
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kCx, kDx));
        self->_passedOrderGp[0] = kCx;
        self->_passedOrderGp[1] = kDx;
        return kErrorOk;

      case kCallConvX86GccRegParm1:
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kAx));
        self->_passedOrderGp[0] = kAx;
        return kErrorOk;

      case kCallConvX86GccRegParm2:
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kAx, kDx));
        self->_passedOrderGp[0] = kAx;
        self->_passedOrderGp[1] = kDx;
        return kErrorOk;

      case kCallConvX86GccRegParm3:
        self->_passed.set(X86Reg::kClassGp, Utils::mask(kAx, kDx, kCx));
        self->_passedOrderGp[0] = kAx;
        self->_passedOrderGp[1] = kDx;
        self->_passedOrderGp[2] = kCx;
        return kErrorOk;
    }
  }

  // --------------------------------------------------------------------------
  // [X64 Support]
  // --------------------------------------------------------------------------

  if (arch == Arch::kTypeX64) {
    switch (callConv) {
      case kCallConvX64Win:
        self->_spillZoneSize = 32;

        self->_passed.set(X86Reg::kClassGp, Utils::mask(kCx, kDx, 8, 9));
        self->_passedOrderGp[0] = kCx;
        self->_passedOrderGp[1] = kDx;
        self->_passedOrderGp[2] = 8;
        self->_passedOrderGp[3] = 9;

        self->_passed.set(X86Reg::kClassXyz, Utils::mask(0, 1, 2, 3));
        self->_passedOrderXyz[0] = 0;
        self->_passedOrderXyz[1] = 1;
        self->_passedOrderXyz[2] = 2;
        self->_passedOrderXyz[3] = 3;

        self->_preserved.set(X86Reg::kClassGp , Utils::mask(kBx, kSp, kBp, kSi, kDi, 12, 13, 14, 15));
        self->_preserved.set(X86Reg::kClassXyz, Utils::mask(6  , 7  , 8  , 9  , 10 , 11, 12, 13, 14, 15));
        return kErrorOk;

      case kCallConvX64Unix:
        self->_redZoneSize = 128;

        self->_passed.set(X86Reg::kClassGp, Utils::mask(kDi, kSi, kDx, kCx, 8, 9));
        self->_passedOrderGp[0] = kDi;
        self->_passedOrderGp[1] = kSi;
        self->_passedOrderGp[2] = kDx;
        self->_passedOrderGp[3] = kCx;
        self->_passedOrderGp[4] = 8;
        self->_passedOrderGp[5] = 9;

        self->_passed.set(X86Reg::kClassXyz, Utils::mask(0, 1, 2, 3, 4, 5, 6, 7));
        self->_passedOrderXyz[0] = 0;
        self->_passedOrderXyz[1] = 1;
        self->_passedOrderXyz[2] = 2;
        self->_passedOrderXyz[3] = 3;
        self->_passedOrderXyz[4] = 4;
        self->_passedOrderXyz[5] = 5;
        self->_passedOrderXyz[6] = 6;
        self->_passedOrderXyz[7] = 7;

        self->_preserved.set(X86Reg::kClassGp, Utils::mask(kBx, kSp, kBp, 12, 13, 14, 15));
        return kErrorOk;
    }
  }

  return DebugUtils::errored(kErrorInvalidArch);
}

static Error X86FuncDecl_initFunc(X86FuncDecl* self, uint32_t arch,
  uint32_t ret, const uint32_t* args, uint32_t numArgs) {

  ASMJIT_ASSERT(numArgs <= kFuncArgCount);

  uint32_t callConv = self->_callConv;
  uint32_t regSize = (arch == Arch::kTypeX86) ? 4 : 8;

  int32_t i = 0;
  int32_t gpPos = 0;
  int32_t xmmPos = 0;
  int32_t stackOffset = 0;
  const uint32_t* idMap = nullptr;

  if (arch == Arch::kTypeX86) idMap = _x86TypeData.idMapX86;
  if (arch == Arch::kTypeX64) idMap = _x86TypeData.idMapX64;

  ASMJIT_ASSERT(idMap != nullptr);
  self->_numArgs = static_cast<uint8_t>(numArgs);
  self->_retCount = 0;

  for (i = 0; i < static_cast<int32_t>(numArgs); i++) {
    FuncInOut& arg = self->getArg(i);
    arg._typeId = static_cast<uint8_t>(idMap[args[i]]);
    arg._regId = kInvalidReg;
    arg._stackOffset = kFuncStackInvalid;
  }

  for (; i < kFuncArgCount; i++) {
    self->_args[i].reset();
  }

  self->_rets[0].reset();
  self->_rets[1].reset();
  self->_argStackSize = 0;
  self->_used.reset();

  if (VirtType::isValidTypeId(ret)) {
    ret = idMap[ret];
    switch (ret) {
      case VirtType::kIdI64:
      case VirtType::kIdU64:
        // 64-bit value is returned in EDX:EAX on x86.
        if (arch == Arch::kTypeX86) {
          self->_retCount = 2;
          self->_rets[0]._typeId = VirtType::kIdU32;
          self->_rets[0]._regId = X86Gp::kIdAx;
          self->_rets[1]._typeId = static_cast<uint8_t>(ret - 2);
          self->_rets[1]._regId = X86Gp::kIdDx;
        }
        ASMJIT_FALLTHROUGH;

      case VirtType::kIdI8:
      case VirtType::kIdU8:
      case VirtType::kIdI16:
      case VirtType::kIdU16:
      case VirtType::kIdI32:
      case VirtType::kIdU32:
        self->_retCount = 1;
        self->_rets[0]._typeId = static_cast<uint8_t>(ret);
        self->_rets[0]._regId = X86Gp::kIdAx;
        break;

      case VirtType::kIdF32:
        self->_retCount = 1;
        if (arch == Arch::kTypeX86) {
          self->_rets[0]._typeId = VirtType::kIdF32;
          self->_rets[0]._regId = 0;
        }
        else {
          self->_rets[0]._typeId = VirtType::kIdX86XmmSs;
          self->_rets[0]._regId = 0;
        }
        break;

      case VirtType::kIdF64:
        self->_retCount = 1;
        if (arch == Arch::kTypeX86) {
          self->_rets[0]._typeId = VirtType::kIdF64;
          self->_rets[0]._regId = 0;
        }
        else {
          self->_rets[0]._typeId = VirtType::kIdX86XmmSd;
          self->_rets[0]._regId = 0;
          break;
        }
        break;

      case VirtType::kIdX86Mm:
        self->_retCount = 1;
        self->_rets[0]._typeId = static_cast<uint8_t>(ret);
        self->_rets[0]._regId = 0;
        break;

      case VirtType::kIdX86Xmm:
      case VirtType::kIdX86XmmSs:
      case VirtType::kIdX86XmmSd:
      case VirtType::kIdX86XmmPs:
      case VirtType::kIdX86XmmPd:
        self->_retCount = 1;
        self->_rets[0]._typeId = static_cast<uint8_t>(ret);
        self->_rets[0]._regId = 0;
        break;
    }
  }

  if (self->_numArgs == 0)
    return kErrorOk;

  if (arch == Arch::kTypeX86) {
    // Register arguments (Integer), always left-to-right.
    for (i = 0; i != static_cast<int32_t>(numArgs); i++) {
      FuncInOut& arg = self->getArg(i);
      uint32_t varType = idMap[arg.getTypeId()];

      if (!VirtType::isIntTypeId(varType) || gpPos >= ASMJIT_ARRAY_SIZE(self->_passedOrderGp))
        continue;

      if (self->_passedOrderGp[gpPos] == kInvalidReg)
        continue;

      arg._regId = self->_passedOrderGp[gpPos++];
      self->_used.or_(X86Reg::kClassGp, Utils::mask(arg.getRegId()));
    }

    // Stack arguments.
    int32_t iStart = static_cast<int32_t>(numArgs - 1);
    int32_t iEnd   = -1;
    int32_t iStep  = -1;

    if (self->_argsDirection == kFuncDirLTR) {
      iStart = 0;
      iEnd   = static_cast<int32_t>(numArgs);
      iStep  = 1;
    }

    for (i = iStart; i != iEnd; i += iStep) {
      FuncInOut& arg = self->getArg(i);
      if (arg.hasRegId()) continue;

      uint32_t varType = idMap[arg.getTypeId()];
      if (VirtType::isIntTypeId(varType)) {
        stackOffset -= 4;
        arg._stackOffset = static_cast<int16_t>(stackOffset);
      }
      else if (VirtType::isFloatTypeId(varType)) {
        int32_t size = static_cast<int32_t>(_x86TypeData.typeInfo[varType].getTypeSize());
        stackOffset -= size;
        arg._stackOffset = static_cast<int16_t>(stackOffset);
      }
    }
  }

  if (arch == Arch::kTypeX64) {
    if (callConv == kCallConvX64Win) {
      int32_t argMax = Utils::iMin<int32_t>(numArgs, 4);

      // Register arguments (GP/XMM), always left-to-right.
      for (i = 0; i != argMax; i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = idMap[arg.getTypeId()];

        if (VirtType::isIntTypeId(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderGp)) {
          arg._regId = self->_passedOrderGp[i];
          self->_used.or_(X86Reg::kClassGp, Utils::mask(arg.getRegId()));
          continue;
        }

        if (VirtType::isFloatTypeId(varType) && i < ASMJIT_ARRAY_SIZE(self->_passedOrderXyz)) {
          arg._typeId = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regId = self->_passedOrderXyz[i];
          self->_used.or_(X86Reg::kClassXyz, Utils::mask(arg.getRegId()));
        }
      }

      // Stack arguments (always right-to-left).
      for (i = numArgs - 1; i != -1; i--) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = idMap[arg.getTypeId()];

        if (arg.hasRegId())
          continue;

        if (VirtType::isIntTypeId(varType)) {
          stackOffset -= 8; // Always 8 bytes.
          arg._stackOffset = stackOffset;
        }
        else if (VirtType::isFloatTypeId(varType)) {
          stackOffset -= 8; // Always 8 bytes (float/double).
          arg._stackOffset = stackOffset;
        }
      }

      // 32 bytes shadow space (X64W calling convention specific).
      stackOffset -= 4 * 8;
    }
    else {
      // Register arguments (Gp), always left-to-right.
      for (i = 0; i != static_cast<int32_t>(numArgs); i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = idMap[arg.getTypeId()];

        if (!VirtType::isIntTypeId(varType) || gpPos >= ASMJIT_ARRAY_SIZE(self->_passedOrderGp))
          continue;

        if (self->_passedOrderGp[gpPos] == kInvalidReg)
          continue;

        arg._regId = self->_passedOrderGp[gpPos++];
        self->_used.or_(X86Reg::kClassGp, Utils::mask(arg.getRegId()));
      }

      // Register arguments (XMM), always left-to-right.
      for (i = 0; i != static_cast<int32_t>(numArgs); i++) {
        FuncInOut& arg = self->getArg(i);
        uint32_t varType = idMap[arg.getTypeId()];

        if (VirtType::isFloatTypeId(varType)) {
          arg._typeId = static_cast<uint8_t>(x86ArgTypeToXmmType(varType));
          arg._regId = self->_passedOrderXyz[xmmPos++];
          self->_used.or_(X86Reg::kClassXyz, Utils::mask(arg.getRegId()));
        }
      }

      // Stack arguments.
      for (i = numArgs - 1; i != -1; i--) {
        FuncInOut& arg = self->getArg(i);
        if (arg.hasRegId()) continue;

        uint32_t varType = idMap[arg.getTypeId()];
        if (VirtType::isIntTypeId(varType)) {
          stackOffset -= 8;
          arg._stackOffset = static_cast<int16_t>(stackOffset);
        }
        else if (VirtType::isFloatTypeId(varType)) {
          int32_t size = static_cast<int32_t>(_x86TypeData.typeInfo[varType].getTypeSize());

          stackOffset -= size;
          arg._stackOffset = static_cast<int16_t>(stackOffset);
        }
      }
    }
  }

  // Modify the stack offset, thus in result all parameters would have positive
  // non-zero stack offset.
  for (i = 0; i < static_cast<int32_t>(numArgs); i++) {
    FuncInOut& arg = self->getArg(i);
    if (!arg.hasRegId()) {
      arg._stackOffset += static_cast<uint16_t>(static_cast<int32_t>(regSize) - stackOffset);
    }
  }

  self->_argStackSize = static_cast<uint32_t>(-stackOffset);
  return kErrorOk;
}

Error X86FuncDecl::setPrototype(const FuncPrototype& p) {
  uint32_t callConv = p.getCallConv();
  uint32_t arch = x86GetArchFromCConv(callConv);

  if (arch != Arch::kTypeX86 && arch != Arch::kTypeX64)
    return DebugUtils::errored(kErrorInvalidArch);

  if (p.getNumArgs() > kFuncArgCount)
    return DebugUtils::errored(kErrorInvalidArgument);

  ASMJIT_PROPAGATE(X86FuncDecl_initConv(this, arch, callConv));
  ASMJIT_PROPAGATE(X86FuncDecl_initFunc(this, arch, p.getRet(), p.getArgs(), p.getNumArgs()));

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86FuncDecl - Reset]
// ============================================================================

void X86FuncDecl::reset() {
  uint32_t i;

  _callConv = kCallConvNone;
  _calleePopsStack = false;
  _argsDirection = kFuncDirRTL;
  _reserved0 = 0;

  _numArgs = 0;
  _retCount = 0;

  _argStackSize = 0;
  _redZoneSize = 0;
  _spillZoneSize = 0;

  for (i = 0; i < ASMJIT_ARRAY_SIZE(_args); i++)
    _args[i].reset();

  _rets[0].reset();
  _rets[1].reset();

  _used.reset();
  _passed.reset();
  _preserved.reset();

  ::memset(_passedOrderGp, kInvalidReg, ASMJIT_ARRAY_SIZE(_passedOrderGp));
  ::memset(_passedOrderXyz, kInvalidReg, ASMJIT_ARRAY_SIZE(_passedOrderXyz));
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && ASMJIT_BUILD_X86
