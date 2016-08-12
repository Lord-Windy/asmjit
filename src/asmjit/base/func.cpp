// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/func.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

Error CallConv::init(uint32_t ccId) noexcept {
  reset();

#if defined(ASMJIT_BUILD_X86)
  if (ccId >= _kIdX86Start && ccId <= _kIdX64End) {
    // Don't depend on `x86operand.h` here.
    static const uint32_t kKindGp  = 0;
    static const uint32_t kKindMm  = 1;
    static const uint32_t kKindK   = 2;
    static const uint32_t kKindXyz = 3;

    static const uint32_t kAx = 0;
    static const uint32_t kCx = 1;
    static const uint32_t kDx = 2;
    static const uint32_t kBx = 3;
    static const uint32_t kSp = 4;
    static const uint32_t kBp = 5;
    static const uint32_t kSi = 6;
    static const uint32_t kDi = 7;

    switch (ccId) {
      case kIdX86StdCall:
        _calleePopsStack = true;
        goto X86CallConv;

      case kIdX86MsThisCall:
        _calleePopsStack = true;
        _group[kKindGp].initPassed(kCx);
        goto X86CallConv;

      case kIdX86MsFastCall:
      case kIdX86GccFastCall:
        _calleePopsStack = true;
        _group[kKindGp].initPassed(kCx, kDx);
        goto X86CallConv;

      case kIdX86GccRegParm1:
        _group[kKindGp].initPassed(kAx);
        goto X86CallConv;

      case kIdX86GccRegParm2:
        _group[kKindGp].initPassed(kAx, kDx);
        goto X86CallConv;

      case kIdX86GccRegParm3:
        _group[kKindGp].initPassed(kAx, kDx, kCx);
        goto X86CallConv;

      case kIdX86CDecl:
X86CallConv:
        _group[kKindGp].initPreserved(Utils::mask(kBx, kSp, kBp, kSi, kDi));
        break;

      case kIdX86Win64:
        _spillZoneSize = 32;
        _algorithm = kAlgorithmWin64;
        _floatByVec = true;
        _indirectVecArgs = true;
        _group[kKindGp].initPassed(kCx, kDx, 8, 9);
        _group[kKindGp].initPreserved(Utils::mask(kBx, kSp, kBp, kSi, kDi, 12, 13, 14, 15));
        _group[kKindXyz].initPassed(0, 1, 2, 3);
        _group[kKindXyz].initPreserved(Utils::mask(6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
        break;

      case kIdX86Unix64:
        _redZoneSize = 128;
        _floatByVec = true;
        _group[kKindGp].initPassed(kDi, kSi, kDx, kCx, 8, 9);
        _group[kKindGp].initPreserved(Utils::mask(kBx, kSp, kBp, 12, 13, 14, 15));
        _group[kKindXyz].initPassed(0, 1, 2, 3, 4, 5, 6, 7);
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

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
