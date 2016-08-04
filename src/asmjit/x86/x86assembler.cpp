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
#include "../base/cpuinfo.h"
#include "../base/runtime.h"
#include "../base/utils.h"
#include "../base/vmem.h"
#include "../x86/x86assembler.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [FastUInt8]
// ============================================================================

#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
typedef unsigned char FastUInt8;
#else
typedef unsigned int FastUInt8;
#endif

// ============================================================================
// [Constants]
// ============================================================================

//! \internal
//!
//! X86/X64 bytes used to encode important prefixes.
enum X86Byte {
  //! 1-byte REX prefix mask.
  kX86ByteRex = 0x40,

  //! 1-byte REX.W component.
  kX86ByteRexW = 0x08,

  //! 2-byte VEX prefix:
  //!   - `[0]` - `0xC5`.
  //!   - `[1]` - `RvvvvLpp`.
  kX86ByteVex2 = 0xC5,

  //! 3-byte VEX prefix.
  //!   - `[0]` - `0xC4`.
  //!   - `[1]` - `RXBmmmmm`.
  //!   - `[2]` - `WvvvvLpp`.
  kX86ByteVex3 = 0xC4,

  //! 3-byte XOP prefix.
  //!   - `[0]` - `0x8F`.
  //!   - `[1]` - `RXBmmmmm`.
  //!   - `[2]` - `WvvvvLpp`.
  kX86ByteXop3 = 0x8F,

  //! 4-byte EVEX prefix.
  //!   - `[0]` - `0x62`.
  //!   - `[1]` - Payload0 or `P[ 7: 0]` - `[R  X  B  R' 0  0  m  m]`.
  //!   - `[2]` - Payload1 or `P[15: 8]` - `[W  v  v  v  v  1  p  p]`.
  //!   - `[3]` - Payload2 or `P[23:16]` - `[z  L' L  b  V' a  a  a]`.
  //!
  //! Groups:
  //!   - `P[ 1: 0]` - OPCODE: EVEX.mmmmm, only lowest 2 bits [1:0] used.
  //!   - `P[ 3: 2]` - ______: Must be 0.
  //!   - `P[    4]` - REG-ID: EVEX.R' - 5th bit of 'RRRRR'.
  //!   - `P[    5]` - REG-ID: EVEX.B  - 4th bit of 'BBBBB'.
  //!   - `P[    6]` - REG-ID: EVEX.X  - 5th bit of 'BBBBB' or 4th bit of 'XXXX' (with SIB).
  //!   - `P[    7]` - REG-ID: EVEX.R  - 4th bit of 'RRRRR'.
  //!   - `P[ 9: 8]` - OPCODE: EVEX.pp.
  //!   - `P[   10]` - ______: Must be 1.
  //!   - `P[14:11]` - REG-ID: 4 bits of 'VVVV'.
  //!   - `P[   15]` - OPCODE: EVEX.W.
  //!   - `P[18:16]` - REG-ID: K register k0...k7 (Merging/Zeroing Vector Ops).
  //!   - `P[   19]` - REG-ID: 5th bit of 'VVVVV'.
  //!   - `P[   20]` - OPCODE: Broadcast/Rounding Control/SAE bit.
  //!   - `P[22.21]` - OPCODE: Vector Length (L' and  L) / Rounding Control.
  //!   - `P[   23]` - OPCODE: Zeroing/Merging.
  kX86ByteEvex = 0x62
};

// AsmJit specific (used to encode VVVVV field in XOP/VEX/EVEX).
enum VexVVVVV {
  kVexVVVVVShift = 7,
  kVexVVVVVMask = 0x1F << kVexVVVVVShift
};

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix definition.
struct X86OpCodeMM {
  uint8_t len;
  uint8_t data[3];
};

//! \internal
//!
//! Mandatory prefixes used to encode legacy [66, F3, F2] or [9B] byte.
static const uint8_t x86OpCodePP[8] = { 0x00, 0x66, 0xF3, 0xF2, 0x00, 0x00, 0x00, 0x9B };

//! \internal
//!
//! Instruction 2-byte/3-byte opcode prefix data.
static const X86OpCodeMM x86OpCodeMM[] = {
  { 0, { 0x00, 0x00, 0 } }, // #00 (0b0000).
  { 1, { 0x0F, 0x00, 0 } }, // #01 (0b0001).
  { 2, { 0x0F, 0x38, 0 } }, // #02 (0b0010).
  { 2, { 0x0F, 0x3A, 0 } }, // #03 (0b0011).
  { 2, { 0x0F, 0x01, 0 } }, // #04 (0b0100).
  { 0, { 0x00, 0x00, 0 } }, // #05 (0b0101).
  { 0, { 0x00, 0x00, 0 } }, // #06 (0b0110).
  { 0, { 0x00, 0x00, 0 } }, // #07 (0b0111).
  { 0, { 0x00, 0x00, 0 } }, // #08 (0b1000).
  { 0, { 0x00, 0x00, 0 } }, // #09 (0b1001).
  { 0, { 0x00, 0x00, 0 } }, // #0A (0b1010).
  { 0, { 0x00, 0x00, 0 } }, // #0B (0b1011).
  { 0, { 0x00, 0x00, 0 } }, // #0C (0b1100).
  { 0, { 0x00, 0x00, 0 } }, // #0D (0b1101).
  { 0, { 0x00, 0x00, 0 } }, // #0E (0b1110).
  { 0, { 0x00, 0x00, 0 } }  // #0F (0b1111).
};

static const uint8_t x86SegmentPrefix[8] = { 0x00, 0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x00 };
static const uint8_t x86OpCodePushSeg[8] = { 0x00, 0x06, 0x0E, 0x16, 0x1E, 0xA0, 0xA8, 0x00 };
static const uint8_t x86OpCodePopSeg[8]  = { 0x00, 0x07, 0x00, 0x17, 0x1F, 0xA1, 0xA9, 0x00 };

// ============================================================================
// [asmjit::X86MemInfo]
// ============================================================================

//! \internal
//!
//! Memory operand's info bits.
enum X86MemInfo_Enum {
  kX86MemInfo_0,

  kX86MemInfo_BaseGp    = 0x01, //!< Has BASE reg, REX.B can be 1, compatible with REX.B byte.
  kX86MemInfo_Index     = 0x02, //!< Has INDEX reg, REX.X can be 1, compatible with REX.X byte.

  kX86MemInfo_BaseLabel = 0x10, //!< Base is Label.
  kX86MemInfo_BaseRip   = 0x20, //!< Base is RIP.

  kX86MemInfo_67H_X86   = 0x40, //!< Address-size override in 32-bit mode.
  kX86MemInfo_67H_X64   = 0x80, //!< Address-size override in 64-bit mode.
  kX86MemInfo_67H_Mask  = 0xC0  //!< Contains all address-size override bits.
};

// A lookup table that contains various information based on the BASE and INDEX
// information of a memory operand. This is much better and safer than playing
// with IFs in the code and can check for errors must faster and better as this
// checks basically everything as a side product.
template<uint32_t B, uint32_t I>
struct X86MemInfo_T {
  enum {
    kHasB  = (B == X86Reg::kRegGpw ||
              B == X86Reg::kRegGpd ||
              B == X86Reg::kRegGpq ) ? kX86MemInfo_BaseGp    : kX86MemInfo_0,
    kRip   = (B == X86Reg::kRegRip ) ? kX86MemInfo_BaseRip   : kX86MemInfo_0,
    kLabel = (B == Label::kLabelTag) ? kX86MemInfo_BaseLabel : kX86MemInfo_0,

    kHasX  = (I == X86Reg::kRegGpw ||
              I == X86Reg::kRegGpd ||
              I == X86Reg::kRegGpq ||
              I == X86Reg::kRegXmm ||
              I == X86Reg::kRegYmm ||
              I == X86Reg::kRegZmm ) ? kX86MemInfo_Index     : kX86MemInfo_0,

    k67H   = (B == X86Reg::kRegGpw  && I == X86Reg::kRegNone) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegGpd  && I == X86Reg::kRegNone) ? kX86MemInfo_67H_X64 :
             (B == X86Reg::kRegNone && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegNone && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64 :
             (B == X86Reg::kRegGpw  && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegGpd  && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64 :
             (B == X86Reg::kRegGpw  && I == X86Reg::kRegXmm ) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegGpd  && I == X86Reg::kRegXmm ) ? kX86MemInfo_67H_X64 :
             (B == X86Reg::kRegGpw  && I == X86Reg::kRegYmm ) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegGpd  && I == X86Reg::kRegYmm ) ? kX86MemInfo_67H_X64 :
             (B == X86Reg::kRegGpw  && I == X86Reg::kRegZmm ) ? kX86MemInfo_67H_X86 :
             (B == X86Reg::kRegGpd  && I == X86Reg::kRegZmm ) ? kX86MemInfo_67H_X64 :
             (B == Label::kLabelTag && I == X86Reg::kRegGpw ) ? kX86MemInfo_67H_X86 :
             (B == Label::kLabelTag && I == X86Reg::kRegGpd ) ? kX86MemInfo_67H_X64 : kX86MemInfo_0,

    // The result stored in the LUT is a combination of
    //   - k67H - Address override prefix - depends on BASE+INDEX register types and
    //            the target architecture.
    //   - kREX - A possible combination of REX.[B|X|R|W] bits in REX prefix where
    //            REX.B and REX.X are possibly masked out, but REX.R and REX.W are
    //            kept as is.
    kValue = kHasB | kHasX | 0x04 | 0x08 | kRip | kLabel | k67H
  };
};
#define R(INDEX) X86MemInfo_T< 0, INDEX>::kValue, X86MemInfo_T< 1, INDEX>::kValue, \
                 X86MemInfo_T< 2, INDEX>::kValue, X86MemInfo_T< 3, INDEX>::kValue, \
                 X86MemInfo_T< 4, INDEX>::kValue, X86MemInfo_T< 5, INDEX>::kValue, \
                 X86MemInfo_T< 6, INDEX>::kValue, X86MemInfo_T< 7, INDEX>::kValue, \
                 X86MemInfo_T< 8, INDEX>::kValue, X86MemInfo_T< 9, INDEX>::kValue, \
                 X86MemInfo_T<10, INDEX>::kValue, X86MemInfo_T<11, INDEX>::kValue, \
                 X86MemInfo_T<12, INDEX>::kValue, X86MemInfo_T<13, INDEX>::kValue, \
                 X86MemInfo_T<14, INDEX>::kValue, X86MemInfo_T<15, INDEX>::kValue
static const uint8_t x86MemInfo[256] = {
  R(0), R(1), R( 2), R( 3), R( 4), R( 5), R( 6), R( 7),
  R(8), R(9), R(10), R(11), R(12), R(13), R(14), R(15)
};
#undef R

// ============================================================================
// [asmjit::X86VEXPrefix | X86LLByRegType | X86CDisp8Table]
// ============================================================================

#define R(TMPL, I) TMPL<I +  0>::kValue, TMPL<I +  1>::kValue, \
                   TMPL<I +  2>::kValue, TMPL<I +  3>::kValue, \
                   TMPL<I +  4>::kValue, TMPL<I +  5>::kValue, \
                   TMPL<I +  6>::kValue, TMPL<I +  7>::kValue, \
                   TMPL<I +  8>::kValue, TMPL<I +  9>::kValue, \
                   TMPL<I + 10>::kValue, TMPL<I + 11>::kValue, \
                   TMPL<I + 12>::kValue, TMPL<I + 13>::kValue, \
                   TMPL<I + 14>::kValue, TMPL<I + 15>::kValue

// VEX3 or XOP xor bits applied to the opcode before emitted. The index to this
// table is 'mmmmm' value, which contains all we need. This is only used by a
// 3 BYTE VEX and XOP prefixes, 2 BYTE VEX prefix is handled differently. The
// idea is to minimize the difference between VEX3 vs XOP when encoding VEX
// or XOP instruction. This should minimize the code required to emit such
// instructions and should also make it faster as we don't need any branch to
// decide between VEX3 vs XOP.
template<uint32_t MM>
struct X86VEXPrefix_T {
  enum {
    //            ____    ___
    // [_OPCODE_|WvvvvLpp|RXBmmmmm|VEX3_XOP]
    kValue = ((MM & 0x08) ? kX86ByteXop3 : kX86ByteVex3) | (0xF << 19) | (0x7 << 13)
  };
};
static const uint32_t x86VEXPrefix[16] = { R(X86VEXPrefix_T, 0) };

// Table that contains LL opcode field addressed by a register size / 16. It's
// used to propagate L.256 or L.512 when YMM or ZMM registers are used,
// respectively.
template<uint32_t SIZE>
struct X86LLBySizeDiv16_T {
  enum {
    kValue = (SIZE & (64 >> 4)) ? X86Inst::kOpCode_L_512 :
             (SIZE & (32 >> 4)) ? X86Inst::kOpCode_L_256 : 0
  };
};
static const uint32_t x86LLBySizeDiv16[16] = { R(X86LLBySizeDiv16_T, 0) };

// Table that contains LL opcode field addressed by a register size / 16. It's
// used to propagate L.256 or L.512 when YMM or ZMM registers are used,
// respectively.
template<uint32_t REG_TYPE>
struct X86LLByRegType_T {
  enum {
    kValue = REG_TYPE == X86Reg::kRegZmm ? X86Inst::kOpCode_L_512 :
             REG_TYPE == X86Reg::kRegYmm ? X86Inst::kOpCode_L_256 : 0
  };
};
static const uint32_t x86LLByRegType[16] = { R(X86LLByRegType_T, 0) };

// Table that contains a scale (shift left) based on 'TTWLL' field and
// the instruction's tuple-type (TT) field. The scale is then applied to
// the BASE-N stored in each opcode to calculate the final compressed
// displacement used by all EVEX encoded instructions.
template<uint32_t INDEX>
struct X86CDisp8SHL_T {
  enum {
    kLL   = (INDEX >> 0) & 0x3,
    kL128 = (kLL == 0),
    kL256 = (kLL == 1),
    kL512 = (kLL == 2),

    kW    = (INDEX >> 2) & 0x1,
    kTT   = (INDEX >> 3) << X86Inst::kOpCode_CDTT_Shift,

    kSHL  = (kTT == X86Inst::kOpCode_CDTT_None) ? (kL128 ? 0    : kL256 ? 0    : 0   ) :
            (kTT == X86Inst::kOpCode_CDTT_ByLL) ? (kL128 ? 0    : kL256 ? 1    : 2   ) :
            (kTT == X86Inst::kOpCode_CDTT_T1W ) ? (kL128 ? 0+kW : kL256 ? 1+kW : 2+kW) :
            (kTT == X86Inst::kOpCode_CDTT_DUP ) ? (kL128 ? 0    : kL256 ? 2    : 3   ) : 0,

    // Scale in a way we can just add it to the opcode.
    kValue = kSHL << X86Inst::kOpCode_CDSHL_Shift
  };
};
static const uint32_t x86CDisp8SHL[32] = { R(X86CDisp8SHL_T, 0), R(X86CDisp8SHL_T, 8) };

#undef R

// ============================================================================
// [asmjit::X86Assembler - Helpers]
// ============================================================================

//! Cast `op` to `X86Reg` and return it.
static ASMJIT_INLINE const X86Reg* x86OpReg(const Operand_& op) noexcept {
  return static_cast<const X86Reg*>(&op);
}

//! Cast `op` to `X86Mem` and return it.
static ASMJIT_INLINE const X86Mem* x86OpMem(const Operand_& op) noexcept {
  return static_cast<const X86Mem*>(&op);
}

//! Get if the given pointers `a` and `b` can be encoded by using relative
//! displacement, which fits into a signed 32-bit integer.
static ASMJIT_INLINE bool x64IsRelative(uint64_t a, uint64_t b) noexcept {
  int64_t diff = static_cast<int64_t>(a) - static_cast<int64_t>(b);
  return Utils::isInt32(diff);
}

static ASMJIT_INLINE uint32_t x86OpCodeLByVMem(const Operand_& op) noexcept {
  return x86LLByRegType[static_cast<const X86Mem&>(op).getIndexType()];
}

static ASMJIT_INLINE uint32_t x86OpCodeLBySize(uint32_t size) noexcept {
  return x86LLBySizeDiv16[size / 16];
}

//! Combine `regId` and `vvvvvId` into a single value (used by AVX and AVX-512).
static ASMJIT_INLINE uint32_t x86PackRegAndVvvvv(uint32_t regId, uint32_t vvvvvId) noexcept {
  return regId + (vvvvvId << kVexVVVVVShift);
}

//! Get `O` field of `opCode`.
static ASMJIT_INLINE uint32_t x86ExtractO(uint32_t opCode) noexcept {
  return (opCode >> X86Inst::kOpCode_O_Shift) & 0x07;
}

static ASMJIT_INLINE uint32_t x86ExtractREX(uint32_t opCode, uint32_t options) noexcept {
  // kOpCode_REX was designed in a way that when shifted there will be no bytes
  // set except REX.[B|X|R|W]. The returned value forms a real REX prefix byte.
  // This case is tested by `X86Inst.cpp`.
  return (opCode | options) >> X86Inst::kOpCode_REX_Shift;
}

static ASMJIT_INLINE uint32_t x86ExtractLLMM(uint32_t opCode, uint32_t options) noexcept {
  uint32_t x = (opCode & (X86Inst::kOpCode_L_Mask | X86Inst::kOpCode_MM_Mask)) >> X86Inst::kOpCode_MM_Shift;
  uint32_t y = (options & X86Inst::kOptionVex3);
  return x | y;
}

//! Encode MOD byte.
static ASMJIT_INLINE uint32_t x86EncodeMod(uint32_t m, uint32_t o, uint32_t rm) noexcept {
  ASMJIT_ASSERT(m <= 3);
  ASMJIT_ASSERT(o <= 7);
  ASMJIT_ASSERT(rm <= 7);
  return (m << 6) + (o << 3) + rm;
}

//! Encode SIB byte.
static ASMJIT_INLINE uint32_t x86EncodeSib(uint32_t s, uint32_t i, uint32_t b) noexcept {
  ASMJIT_ASSERT(s <= 3);
  ASMJIT_ASSERT(i <= 7);
  ASMJIT_ASSERT(b <= 7);
  return (s << 6) + (i << 3) + b;
}

// ============================================================================
// [asmjit::X86Assembler - Macros]
// ============================================================================

#define EMIT_BYTE(VAL)                              \
  do {                                              \
    cursor[0] = static_cast<uint8_t>((VAL) & 0xFF); \
    cursor += 1;                                    \
  } while (0)

#define EMIT_DWORD(VAL)                             \
  do {                                              \
    Utils::writeU32uLE(cursor,                      \
      static_cast<uint32_t>(VAL & 0xFFFFFFFF));     \
    cursor += 4;                                    \
  } while (0)

// ============================================================================
// [asmjit::X86Assembler - Construction / Destruction]
// ============================================================================

X86Assembler::X86Assembler(CodeHolder* code) noexcept : Assembler() {
  if (code)
    code->attach(this);
}
X86Assembler::~X86Assembler() noexcept {}

// ============================================================================
// [asmjit::X86Assembler - Events]
// ============================================================================

Error X86Assembler::onAttach(CodeHolder* code) noexcept {
  if (code->getArchType() == Arch::kTypeX86) {
    ASMJIT_PROPAGATE(Base::onAttach(code));

    _setAddressOverrideMask(kX86MemInfo_67H_X86);
    _globalOptions |= X86Inst::_kOptionInvalidRex;

    _nativeGpArray = x86OpData.gpd;
    _nativeGpReg = _nativeGpArray[0];
    return kErrorOk;
  }

  if (code->getArchType() == Arch::kTypeX64) {
    ASMJIT_PROPAGATE(Base::onAttach(code));

    _setAddressOverrideMask(kX86MemInfo_67H_X64);

    _nativeGpArray = x86OpData.gpq;
    _nativeGpReg = _nativeGpArray[0];
    return kErrorOk;
  }

  return DebugUtils::errored(kErrorInvalidArch);
}

Error X86Assembler::onDetach(CodeHolder* code) noexcept {
  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::X86Assembler - Align]
// ============================================================================

Error X86Assembler::align(uint32_t mode, uint32_t alignment) {
#if !defined(ASMJIT_DISABLE_LOGGING)
  if (_globalOptions & kOptionLoggingEnabled)
    _code->_logger->logf("%s.align %u\n", _code->_logger->getIndentation(), alignment);
#endif // !ASMJIT_DISABLE_LOGGING

  if (mode > kAlignZero)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment <= 1)
    return kErrorOk;

  if (!Utils::isPowerOf2(alignment) || alignment > 64)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  uint32_t i = static_cast<uint32_t>(Utils::alignDiff<size_t>(getOffset(), alignment));
  if (i == 0)
    return kErrorOk;

  if (getRemainingSpace() < i) {
    Error err = _code->growBuffer(&_section->buffer, i);
    if (ASMJIT_UNLIKELY(err)) return setLastError(err);
  }

  uint8_t* cursor = _bufferPtr;
  uint8_t pattern = 0x00;

  switch (mode) {
    case kAlignCode: {
      if (_globalHints & kHintOptimizedAlign) {
        // Intel 64 and IA-32 Architectures Software Developer's Manual - Volume 2B (NOP).
        enum { kMaxNopSize = 9 };

        static const uint8_t nopData[kMaxNopSize][kMaxNopSize] = {
          { 0x90 },
          { 0x66, 0x90 },
          { 0x0F, 0x1F, 0x00 },
          { 0x0F, 0x1F, 0x40, 0x00 },
          { 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00 },
          { 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 },
          { 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00 }
        };

        do {
          uint32_t n = Utils::iMin<uint32_t>(i, kMaxNopSize);
          const uint8_t* src = nopData[n - 1];

          i -= n;
          do {
            EMIT_BYTE(*src++);
          } while (--n);
        } while (i);
      }

      pattern = 0x90;
      break;
    }

    case kAlignData: pattern = 0xCC; break;
    case kAlignZero: break; // Pattern already set to zero.
  }

  while (i) {
    EMIT_BYTE(pattern);
    i--;
  }

  _bufferPtr = cursor;
  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Assembler - Emit Helpers]
// ============================================================================

#if !defined(ASMJIT_DISABLE_LOGGING)
static void X86Assembler_logInstruction(X86Assembler* self,
  uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3,
  uint32_t dispSize, uint32_t imLen, uint8_t* afterCursor) {

  Logger* logger = self->_code->getLogger();
  ASMJIT_ASSERT(logger != nullptr);
  ASMJIT_ASSERT(options & CodeEmitter::kOptionLoggingEnabled);

  StringBuilderTmp<256> sb;
  uint32_t logOptions = logger->getOptions();

  uint8_t* beforeCursor = self->_bufferPtr;
  intptr_t emittedSize = (intptr_t)(afterCursor - beforeCursor);

  sb.appendString(logger->getIndentation());

  Operand_ opArray[6];
  opArray[0].copyFrom(o0);
  opArray[1].copyFrom(o1);
  opArray[2].copyFrom(o2);
  opArray[3].copyFrom(o3);
  opArray[4].copyFrom(self->_op4);
  opArray[5].copyFrom(self->_op5);
  if (!(options & CodeEmitter::kOptionHasOp4)) opArray[4].reset();
  if (!(options & CodeEmitter::kOptionHasOp5)) opArray[5].reset();

  self->_formatter.formatInstruction(sb, logOptions, instId, options, self->_opMask, opArray, 6);

  if ((logOptions & Logger::kOptionBinaryForm) != 0)
    LogUtil::formatLine(sb, self->_bufferPtr, emittedSize, dispSize, imLen, self->getInlineComment());
  else
    LogUtil::formatLine(sb, nullptr, kInvalidIndex, 0, 0, self->getInlineComment());

  logger->log(sb.getData(), sb.getLength());
}

static Error X86Assembler_failedInstruction(
  X86Assembler* self,
  uint32_t err,
  uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {

  StringBuilderTmp<256> sb;
  sb.appendString(DebugUtils::errorAsString(err));
  sb.appendString(": ");

  Operand_ opArray[6];
  opArray[0].copyFrom(o0);
  opArray[1].copyFrom(o1);
  opArray[2].copyFrom(o2);
  opArray[3].copyFrom(o3);
  opArray[4].copyFrom(self->_op4);
  opArray[5].copyFrom(self->_op5);
  if (!(options & CodeEmitter::kOptionHasOp4)) opArray[4].reset();
  if (!(options & CodeEmitter::kOptionHasOp5)) opArray[5].reset();

  self->_formatter.formatInstruction(sb, 0, instId, options, self->_opMask, opArray, 6);

  self->resetOptions();
  self->resetInlineComment();

  return self->setLastError(err, sb.getData());
}
#else
static ASMJIT_INLINE Error X86Assembler_failedInstruction(
  X86Assembler* self,
  uint32_t err,
  uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {

  self->resetOptions();
  self->resetInlineComment();

  return self->setLastError(err);
}
#endif

#if !defined(ASMJIT_DISABLE_VALIDATION)
static Error X86Assembler_validateInstruction(
  X86Assembler* self,
  uint32_t instId, uint32_t options, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {

  Operand_ opArray[6];
  opArray[0].copyFrom(o0);
  opArray[1].copyFrom(o1);
  opArray[2].copyFrom(o2);
  opArray[3].copyFrom(o3);
  opArray[4].copyFrom(self->_op4);
  opArray[5].copyFrom(self->_op5);
  if (!(options & CodeEmitter::kOptionHasOp4)) opArray[4].reset();
  if (!(options & CodeEmitter::kOptionHasOp5)) opArray[5].reset();

  Error err = X86Inst::validate(self->getArchType(), instId, options, self->getOpMask(), opArray, 6);
  if (err) return X86Assembler_failedInstruction(self, err, instId, options, o0, o1, o2, o3);

  return kErrorOk;
}
#endif // !ASMJIT_DISABLE_VALIDATION

// ============================================================================
// [asmjit::X86Assembler - Emit]
// ============================================================================

#define ADD_66H_P(EXP)                                                   \
  do {                                                                   \
    opCode |= (static_cast<uint32_t>(EXP) << X86Inst::kOpCode_PP_Shift); \
  } while (0)

#define ADD_66H_P_BY_SIZE(SIZE)                                          \
  do {                                                                   \
    opCode |= (static_cast<uint32_t>((SIZE) & 0x02))                     \
           << (X86Inst::kOpCode_PP_Shift - 1);                           \
  } while (0)

#define ADD_REX_W(EXP)                                                   \
  do {                                                                   \
    if (EXP)                                                             \
      opCode |= X86Inst::kOpCode_W;                                      \
  } while (0)

#define ADD_REX_W_BY_SIZE(SIZE)                                          \
  do {                                                                   \
    if ((SIZE) == 8)                                                     \
      opCode |= X86Inst::kOpCode_W;                                      \
  } while (0)

#define ADD_PREFIX_BY_SIZE(SIZE)                                         \
  do {                                                                   \
    ADD_66H_P_BY_SIZE(SIZE);                                             \
    ADD_REX_W_BY_SIZE(SIZE);                                             \
  } while (0)

#define ADD_VEX_W(EXP)                                                   \
  do {                                                                   \
    opCode |= static_cast<uint32_t>(EXP) << X86Inst::kOpCode_W_Shift;    \
  } while (0)

#define EMIT_PP(OPCODE)                                                  \
  do {                                                                   \
    uint32_t ppIndex =                                                   \
      ((OPCODE                   ) >> X86Inst::kOpCode_PP_Shift) &       \
      (X86Inst::kOpCode_PP_FPUMask >> X86Inst::kOpCode_PP_Shift) ;       \
    uint8_t ppCode = x86OpCodePP[ppIndex];                               \
                                                                         \
    cursor[0] = ppCode;                                                  \
    cursor   += ppIndex != 0;                                            \
  } while (0)

#define EMIT_MM_OP(OPCODE)                                               \
  do {                                                                   \
    uint32_t op = OPCODE & (0x00FF | X86Inst::kOpCode_MM_Mask);          \
                                                                         \
    uint32_t mmIndex = op >> X86Inst::kOpCode_MM_Shift;                  \
    const X86OpCodeMM& mmCode = x86OpCodeMM[mmIndex];                    \
                                                                         \
    if (mmIndex) {                                                       \
      cursor[0] = mmCode.data[0];                                        \
      cursor[1] = mmCode.data[1];                                        \
      cursor   += mmCode.len;                                            \
    }                                                                    \
                                                                         \
    EMIT_BYTE(op);                                                       \
  } while (0)

// If the operand is BPL|SPL|SIL|DIL|R8B-15B
//   - Force REX prefix
// If the operand is AH|BH|CH|DH
//   - patch its index from 0..3 to 4..7 as encoded by X86.
//   - Disallow REX prefix.
#define FIXUP_GPB(REG_OP, REG_ID, ...)                                   \
  do {                                                                   \
    if (static_cast<const X86Gp&>(REG_OP).isGpbLo()) {                   \
      options |= (REG_ID >= 4) ? X86Inst::kOptionRex : 0;                \
    }                                                                    \
    else {                                                               \
      ASMJIT_ASSERT(x86::isGpbHi(REG_OP));                               \
      options |= X86Inst::_kOptionInvalidRex;                            \
      REG_ID += 4;                                                       \
    }                                                                    \
  } while (0)

#define ENC_OPS1(OP0)                     ((Operand::kOp##OP0))
#define ENC_OPS2(OP0, OP1)                ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3))
#define ENC_OPS3(OP0, OP1, OP2)           ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3) + ((Operand::kOp##OP2) << 6))
#define ENC_OPS4(OP0, OP1, OP2, OP3)      ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3) + ((Operand::kOp##OP2) << 6) + ((Operand::kOp##OP3) << 9))
#define ENC_OPS5(OP0, OP1, OP2, OP3, OP4) ((Operand::kOp##OP0) + ((Operand::kOp##OP1) << 3) + ((Operand::kOp##OP2) << 6) + ((Operand::kOp##OP3) << 9) + ((Operand::kOp##OP4) << 12))

Error X86Assembler::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {
  const X86Mem* rmMem;           // Memory operand.
  uint32_t rmInfo;               // Memory operand's info based on x86MemInfo.
  uint32_t rbReg;                // Memory base or modRM register.
  uint32_t rxReg;                // Memory index register.
  uint32_t opReg;                // ModR/M opcode or register id.
  uint32_t opCode;               // Instruction opcode.

  CodeHolder::LabelEntry* label; // Label entry.
  int32_t dispOffset;            // Displacement offset
  intptr_t relocId;              // Displacement relocation id.
  FastUInt8 dispSize = 0;        // Displacement size.

  int64_t imVal;                 // Immediate value (must be 64-bit).
  FastUInt8 imLen = 0;           // Immediate length.

  const uint32_t kSHR_W_PP = X86Inst::kOpCode_PP_Shift - 16;
  const uint32_t kSHR_W_EW = X86Inst::kOpCode_EW_Shift - 23;

  uint8_t* cursor = _bufferPtr;
  uint32_t options = static_cast<uint32_t>(instId >= X86Inst::_kIdCount)       |
                     static_cast<uint32_t>((size_t)(_bufferEnd - cursor) < 16) |
                     getGlobalOptions() | getOptions();


  const X86Inst* iData = _x86InstData + instId;
  const X86Inst::ExtendedData* iExtData;

  // Handle failure and rare cases first.
  const uint32_t kErrorsAndSpecialCases =
    CodeEmitter::kOptionMaybeFailureCase | // Error / Buffer check.
    CodeEmitter::kOptionStrictValidation | // Strict validation.
    X86Inst::kOptionLock;              // LOCK prefix check.

  // Signature of the first 3 operands. Instructions that use more operands
  // create `isign4` and `isign5` on-the-fly.
  uint32_t isign3 = o0.getOp() + (o1.getOp() << 3) + (o2.getOp() << 6);

  if (ASMJIT_UNLIKELY(options & kErrorsAndSpecialCases)) {
    // Don't do anything if we are in error state.
    if (_lastError) return _lastError;

    if (options & CodeEmitter::kOptionMaybeFailureCase) {
      // Unknown instruction.
      if (instId >= X86Inst::_kIdCount)
        goto UnknownInstruction;

      // Grow request, happens rarely.
      if ((size_t)(_bufferEnd - cursor) < 16) {
        Error err = _code->growBuffer(&_section->buffer, 16);
        if (ASMJIT_UNLIKELY(err))
          return X86Assembler_failedInstruction(this, err, instId, options, o0, o1, o2, o3);
        cursor = _bufferPtr;
      }
    }

    // Strict validation.
#if !defined(ASMJIT_DISABLE_VALIDATION)
    // It calls `X86Assembler_failedInstruction` on error, no need to call it.
    if (options & CodeEmitter::kOptionStrictValidation)
      ASMJIT_PROPAGATE(X86Assembler_validateInstruction(this, instId, options, o0, o1, o2, o3));
#endif // !ASMJIT_DISABLE_VALIDATION

    // Now it's safe to get extended-data.
    iExtData = iData->getExtendedDataPtr();

    // Handle LOCK prefix.
    if (options & X86Inst::kOptionLock) {
      if (!iExtData->isLockable())
        goto IllegalInstruction;
      EMIT_BYTE(0xF0);
    }
  }
  else {
    iExtData = iData->getExtendedDataPtr();
  }

  // --------------------------------------------------------------------------
  // [Encoding Scope]
  // --------------------------------------------------------------------------

  opCode = iData->getPrimaryOpCode();
  opReg = x86ExtractO(opCode);

  switch (iExtData->getEncoding()) {
    case X86Inst::kEncodingNone:
      goto EmitDone;

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingX86Op:
      goto EmitX86Op;

    case X86Inst::kEncodingX86Op_O:
      rbReg = 0;
      goto EmitX86R;

    case X86Inst::kEncodingX86OpAx:
      if (isign3 == 0)
        goto EmitX86Op;

      if (isign3 == ENC_OPS1(Reg) && o0.getId() == X86Gp::kIdAx)
        goto EmitX86Op;
      break;

    case X86Inst::kEncodingX86OpDxAx:
      if (isign3 == 0)
        goto EmitX86Op;

      if (isign3 == ENC_OPS2(Reg, Reg) && o0.getId() == X86Gp::kIdDx &&
                                          o1.getId() == X86Gp::kIdAx)
        goto EmitX86Op;
      break;

    case X86Inst::kEncodingX86M:
      rbReg = o0.getId();
      ADD_PREFIX_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS1(Reg))
        goto EmitX86R;

      rmMem = x86OpMem(o0);
      if (isign3 == ENC_OPS1(Mem))
        goto EmitX86M;
      break;

    case X86Inst::kEncodingX86M_OptB_MulDiv:
CaseX86M_OptB_MulDiv:
      // Explicit form?
      if (isign3 > 0x7) {
        // [AX] <- [AX] div|mul r8.
        if (isign3 == ENC_OPS2(Reg, Reg)) {
          if (ASMJIT_UNLIKELY(!x86::isGpw(o0, X86Gp::kIdAx) || !x86::isGpb(o1)))
            goto IllegalInstruction;

          rbReg = o1.getId();
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }

        // [AX] <- [AX] div|mul m8.
        if (isign3 == ENC_OPS2(Reg, Mem)) {
          if (ASMJIT_UNLIKELY(!x86::isGpw(o0, X86Gp::kIdAx)))
            goto IllegalInstruction;

          rmMem = x86OpMem(o1);
          goto EmitX86M;
        }

        // [?DX:?AX] <- [?DX:?AX] div|mul r16|r32|r64
        if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
          if (ASMJIT_UNLIKELY(o0.getSize() != o1.getSize()))
            goto IllegalInstruction;
          rbReg = o2.getId();

          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }

        // [?DX:?AX] <- [?DX:?AX] div|mul m16|m32|m64
        if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
          if (ASMJIT_UNLIKELY(o0.getSize() != o1.getSize()))
            goto IllegalInstruction;
          rmMem = x86OpMem(o2);

          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }

        goto IllegalInstruction;
      }

      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86M_OptB:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto IllegalInstruction;
        rmMem = x86OpMem(o0);

        opCode += o0.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86M_Only:
      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Rm:
      ADD_PREFIX_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Arith:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        if (o0.getSize() != o1.getSize())
          goto IllegalInstruction;

        if (o0.getSize() == 1) {
          opCode += 2;
          FIXUP_GPB(o0, opReg);
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }
        else {
          opCode += 3;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          opCode += 2;
          goto EmitX86M;
        }
        else {
          opCode += 3;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
      }

      // The remaining instructions use 0x80 opcode.
      opCode = 0x80;

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        uint32_t regSize = o0.getSize();

        rbReg = o0.getId();
        imVal = static_cast<const Imm&>(o1).getInt64();

        if (regSize == 1) {
          FIXUP_GPB(o0, rbReg);
          imLen = 1;
        }
        else {
          if (regSize == 2) {
            ADD_66H_P(1);
          }
          else if (regSize == 8) {
            // In 64-bit mode it's not possible to use 64-bit immediate.
            if (Utils::isUInt32(imVal)) {
              // Zero-extend `and` by using a 32-bit GPD destination instead of a 64-bit GPQ.
              if (instId == X86Inst::kIdAnd)
                regSize = 4;
              else if (!Utils::isInt32(imVal))
                goto IllegalInstruction;
            }
            ADD_REX_W_BY_SIZE(regSize);
          }

          imLen = Utils::iMin<uint32_t>(regSize, 4);
          if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm)) imLen = 1;
        }

        // Alternate Form - AL, AX, EAX, RAX.
        if (rbReg == 0 && (regSize == 1 || imLen != 1) && !(options & X86Inst::kOptionLongForm)) {
          opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
          opCode |= ((opReg << 3) | (0x04 + (regSize != 1)));
          imLen = Utils::iMin<uint32_t>(regSize, 4);
          goto EmitX86Op;
        }

        opCode += regSize != 1 ? (imLen != 1 ? 1 : 3) : 0;
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        uint32_t memSize = o0.getSize();

        if (ASMJIT_UNLIKELY(memSize == 0))
          goto IllegalInstruction;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = Utils::iMin<uint32_t>(memSize, 4);
        if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm)) imLen = 1;

        opCode += memSize != 1 ? (imLen != 1 ? 1 : 3) : 0;
        ADD_PREFIX_BY_SIZE(memSize);

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Bswap:
      if (isign3 == ENC_OPS1(Reg)) {
        if (ASMJIT_UNLIKELY(o0.getSize() < 4))
          goto IllegalInstruction;

        opReg = o0.getId();
        ADD_REX_W_BY_SIZE(o0.getSize());
        goto EmitX86OpWithOpReg;
      }
      break;

    case X86Inst::kEncodingX86Bt:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }

      // The remaining instructions use the secondary opcode/r.
      imVal = static_cast<const Imm&>(o1).getInt64();
      imLen = 1;

      opCode = iExtData->getSecondaryOpCode();
      opReg = x86ExtractO(opCode);
      ADD_PREFIX_BY_SIZE(o0.getSize());

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto IllegalInstruction;

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Call:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        goto EmitJmpOrCallAbs;
      }

      if (isign3 == ENC_OPS1(Label)) {
        label = _code->getLabelEntry(static_cast<const Label&>(o0));
        if (!label) goto InvalidLabel;

        if (label->offset != -1) {
          // Bound label.
          static const intptr_t kRel32Size = 5;
          intptr_t offs = label->offset - (intptr_t)(cursor - _bufferData);

          ASMJIT_ASSERT(offs <= 0);
          EMIT_BYTE(opCode);
          EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));
        }
        else {
          // Non-bound label.
          EMIT_BYTE(opCode);
          dispOffset = -4;
          dispSize = 4;
          relocId = -1;
          goto EmitDisplacement;
        }
        goto EmitDone;
      }
      break;

    case X86Inst::kEncodingX86Cmpxchg: {
      // Convert explicit to implicit.
      if (isign3 & (0x7 << 6)) {
        if (!x86::isGp(o2) || o2.getId() != X86Gp::kIdAx)
          goto IllegalInstruction;
        isign3 &= 0x3F;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (o0.getSize() != o1.getSize())
          goto IllegalInstruction;

        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          ADD_PREFIX_BY_SIZE(o0.getSize());
          opCode++;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        if (o1.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          goto EmitX86M;
        }
        else {
          ADD_PREFIX_BY_SIZE(o1.getSize());
          opCode++;
          goto EmitX86M;
        }
      }
      break;
    }

    case X86Inst::kEncodingX86Crc:
      opReg = o0.getId();

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o1.getId();
        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }
        else {
          // This seems to be the only exception of encoding 66F2 PP prefix.
          if (o1.getSize() == 2) EMIT_BYTE(0x66);

          opCode++;
          ADD_REX_W_BY_SIZE(o1.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmMem = x86OpMem(o1);
        if (o1.getSize() == 0)
          goto IllegalInstruction;

        // This seems to be the only exception of encoding 66F2 PP prefix.
        if (o1.getSize() == 2) EMIT_BYTE(0x66);

        opCode += o1.getSize() != 1;
        ADD_REX_W_BY_SIZE(o1.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Enter:
      if (isign3 == ENC_OPS2(Imm, Imm)) {
        imVal = (static_cast<uint32_t>(static_cast<const Imm&>(o1).getUInt16()) << 0) |
                (static_cast<uint32_t>(static_cast<const Imm&>(o0).getUInt8()) << 16) ;
        imLen = 3;
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Imul:
      // First process all forms distinct of `kEncodingX86M_OptB_MulDiv`.
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = o0.getId();
        rbReg = o1.getId();

        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        goto EmitX86M;
      }

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        // Must be explicit 'ax, r8' form.
        if (o1.getSize() == 1)
          goto CaseX86M_OptB_MulDiv;

        if (o0.getSize() != o1.getSize())
          goto IllegalInstruction;

        opReg = o0.getId();
        rbReg = o1.getId();

        opCode = X86Inst::kOpCode_MM_0F | 0xAF;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        // Must be explicit 'ax, m8' form.
        if (o1.getSize() == 1)
          goto CaseX86M_OptB_MulDiv;

        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        opCode = X86Inst::kOpCode_MM_0F | 0xAF;
        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }

      // Shorthand to imul 'reg, reg, imm'.
      if (isign3 == ENC_OPS2(Reg, Imm)) {
        opCode = 0x6B;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        if (!Utils::isInt8(imVal) || (options & X86Inst::kOptionLongForm)) {
          opCode -= 2;
          imLen = o0.getSize() == 2 ? 2 : 4;
        }

        opReg = rbReg = o0.getId();
        goto EmitX86R;
      }

      // Try implicit form.
      goto CaseX86M_OptB_MulDiv;

    case X86Inst::kEncodingX86IncDec:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          goto EmitX86R;
        }

        if (getArchType() == Arch::kTypeX86) {
          // INC r16|r32 is only encodable in 32-bit mode (collides with REX).
          opCode = iExtData->getSecondaryOpCode() + (rbReg & 0x07);
          ADD_66H_P_BY_SIZE(o0.getSize());
          goto EmitX86Op;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        opCode += o0.getSize() != 1;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Int:
      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        imLen = 1;
        goto EmitX86Op;
      }
      break;

    case X86Inst::kEncodingX86Jcc:
      if (isign3 == ENC_OPS1(Label)) {
        label = _code->getLabelEntry(static_cast<const Label&>(o0));
        if (!label) goto InvalidLabel;

        if (_globalHints & CodeEmitter::kHintPredictedJumps) {
          if (options & X86Inst::kOptionTaken)
            EMIT_BYTE(0x3E);
          if (options & X86Inst::kOptionNotTaken)
            EMIT_BYTE(0x2E);
        }

        if (label->offset != -1) {
          // Bound label.
          static const intptr_t kRel8Size = 2;
          static const intptr_t kRel32Size = 6;

          intptr_t offs = label->offset - (intptr_t)(cursor - _bufferData);
          ASMJIT_ASSERT(offs <= 0);

          if ((options & X86Inst::kOptionLongForm) == 0 && Utils::isInt8(offs - kRel8Size)) {
            EMIT_BYTE(opCode);
            EMIT_BYTE(offs - kRel8Size);

            options |= X86Inst::kOptionShortForm;
            goto EmitDone;
          }
          else {
            EMIT_BYTE(0x0F);
            EMIT_BYTE(opCode + 0x10);
            EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));

            options &= ~X86Inst::kOptionShortForm;
            goto EmitDone;
          }
        }
        else {
          // Non-bound label.
          if (options & X86Inst::kOptionShortForm) {
            EMIT_BYTE(opCode);
            dispOffset = -1;
            dispSize = 1;
            relocId = -1;
            goto EmitDisplacement;
          }
          else {
            EMIT_BYTE(0x0F);
            EMIT_BYTE(opCode + 0x10);
            dispOffset = -4;
            dispSize = 4;
            relocId = -1;
            goto EmitDisplacement;
          }
        }
      }
      break;

    case X86Inst::kEncodingX86Jecxz:
      if (isign3 == ENC_OPS2(Reg, Label)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdCx))
          goto IllegalInstruction;

        label = _code->getLabelEntry(static_cast<const Label&>(o1));
        if (!label) goto InvalidLabel;

        if ((getArchType() == Arch::kTypeX86 && o0.getSize() == 2) ||
            (getArchType() != Arch::kTypeX86 && o0.getSize() == 4)) {
          EMIT_BYTE(0x67);
        }
        EMIT_BYTE(0xE3);

        if (label->offset != -1) {
          // Bound label.
          intptr_t offs = label->offset - (intptr_t)(cursor - _bufferData) - 1;
          if (ASMJIT_UNLIKELY(!Utils::isInt8(offs)))
            goto IllegalInstruction;

          EMIT_BYTE(offs);
          goto EmitDone;
        }
        else {
          // Non-bound label.
          dispOffset = -1;
          dispSize = 1;
          relocId = -1;
          goto EmitDisplacement;
        }
      }
      break;

    case X86Inst::kEncodingX86Jmp:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }

      // The following instructions use the secondary opcode (0xE9).
      opCode = 0xE9;

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        goto EmitJmpOrCallAbs;
      }

      if (isign3 == ENC_OPS1(Label)) {
        label = _code->getLabelEntry(static_cast<const Label&>(o0));
        if (!label) goto InvalidLabel;

        if (label->offset != -1) {
          // Bound label.
          const intptr_t kRel8Size = 2;
          const intptr_t kRel32Size = 5;

          intptr_t offs = label->offset - (intptr_t)(cursor - _bufferData);

          if (Utils::isInt8(offs - kRel8Size) && !(options & X86Inst::kOptionLongForm)) {
            options |= X86Inst::kOptionShortForm;

            EMIT_BYTE(0xEB);
            EMIT_BYTE(offs - kRel8Size);
            goto EmitDone;
          }
          else {
            options &= ~X86Inst::kOptionShortForm;

            EMIT_BYTE(0xE9);
            EMIT_DWORD(static_cast<int32_t>(offs - kRel32Size));
            goto EmitDone;
          }
        }
        else {
          // Non-bound label.
          if ((options & X86Inst::kOptionShortForm) != 0) {
            EMIT_BYTE(0xEB);
            dispOffset = -1;
            dispSize = 1;
            relocId = -1;
            goto EmitDisplacement;
          }
          else {
            EMIT_BYTE(0xE9);
            dispOffset = -4;
            dispSize = 4;
            relocId = -1;
            goto EmitDisplacement;
          }
        }
      }
      break;

    case X86Inst::kEncodingX86Lea:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Mov:
      // Reg <- Reg
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // Asmjit uses segment registers indexed from 1 to 6, leaving zero as
        // "no segment register used". We have to fix this (decrement the index
        // of the register) when emitting MOV instructions which move to/from
        // a segment register. The segment register is always `opReg`, because
        // the MOV instruction uses RM or MR encoding.

        // GP <- ??
        if (x86::isGp(o0)) {
          // GP <- GP
          if (x86::isGp(o1)) {
            uint32_t size0 = o0.getSize();
            uint32_t size1 = o1.getSize();

            if (size0 != size1) {
              // We allow 'mov r64, r32' as it's basically zero-extend.
              if (size0 == 8 && size1 == 4)
                size0 = 4; // Zero extend, don't promote to 64-bit.
              else
                goto IllegalInstruction;
            }

            if (size0 == 1) {
              FIXUP_GPB(o0, opReg);
              FIXUP_GPB(o1, rbReg);
              opCode = 0x8A;
              goto EmitX86R;
            }
            else {
              opCode = 0x8B;
              ADD_PREFIX_BY_SIZE(size0);
              goto EmitX86R;
            }
          }

          opReg = rbReg;
          rbReg = o0.getId();

          // GP <- SEG
          if (x86::isSeg(o1)) {
            opCode = 0x8C;
            opReg--;
            ADD_PREFIX_BY_SIZE(o0.getSize());
            goto EmitX86R;
          }

          // GP <- CR
          if (x86::isCr(o1)) {
            opCode = 0x20 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }

          // GP <- DR
          if (x86::isDr(o1)) {
            opCode = 0x21 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }
        }
        else {
          // ?? <- GP
          if (!x86::isGp(o1))
            goto IllegalInstruction;

          // SEG <- GP
          if (x86::isSeg(o0)) {
            opCode = 0x8E;
            opReg--;
            ADD_PREFIX_BY_SIZE(o1.getSize());
            goto EmitX86R;
          }

          // CR <- GP
          if (x86::isCr(o0)) {
            opCode = 0x22 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }

          // DR <- GP
          if (x86::isDr(o0)) {
            opCode = 0x23 | X86Inst::kOpCode_MM_0F;
            goto EmitX86R;
          }
        }

        goto IllegalInstruction;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        // SEG <- Mem
        if (x86::isSeg(o0)) {
          opCode = 0x8E;
          opReg--;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
        // Reg <- Mem
        else {
          if (o0.getSize() == 1) {
            FIXUP_GPB(o0, opReg);
          }
          else {
            opCode++;
            ADD_PREFIX_BY_SIZE(o0.getSize());
          }

          // Handle a special form 'mov al|ax|eax|rax, [ptr64]' that doesn't use MOD.
          if (!rmMem->hasBaseOrIndex() && o0.getId() == X86Gp::kIdAx) {
            opCode += 0xA0;
            imVal = rmMem->getOffset();
            imLen = getGpSize();
            goto EmitX86Op;
          }
          else {
            opCode += 0x8A;
            goto EmitX86M;
          }
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        // Mem <- SEG
        if (x86::isSeg(o1)) {
          opCode = 0x8C;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
        // Mem <- Reg
        else {
          if (o1.getSize() == 1) {
            FIXUP_GPB(o1, opReg);
          }
          else {
            opCode++;
            ADD_PREFIX_BY_SIZE(o1.getSize());
          }

          // Handle a special form 'mov [ptr64], al|ax|eax|rax' that doesn't use MOD.
          if (!rmMem->hasBaseOrIndex() && o1.getId() == X86Gp::kIdAx) {
            opCode += 0xA2;
            imVal = rmMem->getOffset();
            imLen = getGpSize();
            goto EmitX86Op;
          }
          else {
            opCode += 0x88;
            goto EmitX86M;
          }
        }
      }

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        opReg = o0.getId();
        imLen = o0.getSize();

        if (imLen == 1) {
          FIXUP_GPB(o0, opReg);

          imVal = static_cast<const Imm&>(o1).getUInt8();
          opCode = 0xB0;
          goto EmitX86OpWithOpReg;
        }
        else {
          // 64-bit immediate in 64-bit mode is allowed.
          imVal = static_cast<const Imm&>(o1).getInt64();

          // Optimize the instruction size by using a 32-bit immediate if possible.
          if (imLen == 8 && !(options & X86Inst::kOptionLongForm)) {
            if (Utils::isUInt32(imVal)) {
              // Zero-extend by using a 32-bit GPD destination instead of a 64-bit GPQ.
              imLen = 4;
            }
            else if (Utils::isInt32(imVal)) {
              // Sign-extend, uses 'C7 /0' opcode.
              rbReg = opReg;

              opCode = 0xC7 | X86Inst::kOpCode_W;
              opReg = 0;

              imLen = 4;
              goto EmitX86R;
            }
          }

          opCode = 0xB8;
          ADD_PREFIX_BY_SIZE(imLen);
          goto EmitX86OpWithOpReg;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        uint32_t memSize = o0.getSize();

        if (ASMJIT_UNLIKELY(memSize == 0))
          goto IllegalInstruction;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = Utils::iMin<uint32_t>(memSize, 4);

        opCode = 0xC6 + (memSize != 1);
        opReg = 0;
        ADD_PREFIX_BY_SIZE(memSize);

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86MovsxMovzx:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        ADD_PREFIX_BY_SIZE(o0.getSize());

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, rbReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode += o1.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Push:
      if (isign3 == ENC_OPS1(Reg)) {
        if (x86::isSeg(o0)) {
          uint32_t segment = o0.getId();
          if (ASMJIT_UNLIKELY(segment >= X86Seg::kIdCount))
            goto IllegalInstruction;

          if (segment >= X86Seg::kIdFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePushSeg[segment]);
          goto EmitDone;
        }
        else {
          goto CaseX86Pop_Gp;
        }
      }

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        imLen = 4;
        if (Utils::isInt8(imVal) && !(options & X86Inst::kOptionLongForm)) imLen = 1;

        opCode = imLen == 1 ? 0x6A : 0x68;
        goto EmitX86Op;
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Pop:
      if (isign3 == ENC_OPS1(Reg)) {
        if (x86::isSeg(o0)) {
          uint32_t segment = o0.getId();
          if (ASMJIT_UNLIKELY(segment == X86Seg::kIdCs || segment >= X86Seg::kIdCount))
            goto IllegalInstruction;

          if (segment >= X86Seg::kIdFs)
            EMIT_BYTE(0x0F);

          EMIT_BYTE(x86OpCodePopSeg[segment]);
          goto EmitDone;
        }
        else {
CaseX86Pop_Gp:
          // We allow 2 byte, 4 byte, and 8 byte register sizes, although PUSH
          // and POP only allow 2 bytes or native size. On 64-bit we simply
          // PUSH/POP 64-bit register even if 32-bit register was given.
          if (ASMJIT_UNLIKELY(o0.getSize() < 2))
            goto IllegalInstruction;

          opCode = iExtData->getSecondaryOpCode();
          opReg = o0.getId();

          ADD_66H_P_BY_SIZE(o0.getSize());
          goto EmitX86OpWithOpReg;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
        if (ASMJIT_UNLIKELY(o0.getSize() != 2 && o0.getSize() != getGpSize()))
          goto IllegalInstruction;

        ADD_66H_P_BY_SIZE(o0.getSize());
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Rep:
      // Emit REP 0xF2 or 0xF3 prefix first.
      EMIT_BYTE(0xF2 + opReg);
      goto EmitX86Op;

    case X86Inst::kEncodingX86Ret:
      if (isign3 == 0) {
        // 'ret' without immediate, change C2 to C3.
        opCode++;
        goto EmitX86Op;
      }

      if (isign3 == ENC_OPS1(Imm)) {
        imVal = static_cast<const Imm&>(o0).getInt64();
        if (imVal == 0 && !(options & X86Inst::kOptionLongForm)) {
          // 'ret' without immediate, change C2 to C3.
          opCode++;
          goto EmitX86Op;
        }
        else {
          imLen = 2;
          goto EmitX86Op;
        }
      }
      break;

    case X86Inst::kEncodingX86Rot:
      if (o0.isReg()) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
        }

        if (isign3 == ENC_OPS2(Reg, Reg)) {
          if (ASMJIT_UNLIKELY(o1.getId() != X86Gp::kIdCx))
            goto IllegalInstruction;

          opCode += 2;
          goto EmitX86R;
        }

        if (isign3 == ENC_OPS2(Reg, Imm)) {
          imVal = static_cast<const Imm&>(o1).getInt64() & 0xFF;
          imLen = imVal != 1;
          if (imLen)
            opCode -= 0x10;
          goto EmitX86R;
        }
      }
      else {
        opCode += o0.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o0.getSize());

        if (isign3 == ENC_OPS2(Mem, Reg)) {
          if (ASMJIT_UNLIKELY(o1.getId() != X86Gp::kIdCx))
            goto IllegalInstruction;

          opCode += 2;
          rmMem = x86OpMem(o0);
          goto EmitX86M;
        }

        if (isign3 == ENC_OPS2(Mem, Imm)) {
          if (ASMJIT_UNLIKELY(o0.getSize() == 0))
            goto IllegalInstruction;

          imVal = static_cast<const Imm&>(o1).getInt64() & 0xFF;
          imLen = imVal != 1;
          if (imLen)
            opCode -= 0x10;
          rmMem = x86OpMem(o0);
          goto EmitX86M;
        }
      }
      break;

    case X86Inst::kEncodingX86Set:
      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86ShldShrd:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_PREFIX_BY_SIZE(o0.getSize());
        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        ADD_PREFIX_BY_SIZE(o1.getSize());
        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }

      // The following instructions use opCode + 1.
      opCode++;

      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o2.getId() != X86Gp::kIdCx))
          goto IllegalInstruction;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Reg)) {
        if (ASMJIT_UNLIKELY(o2.getId() != X86Gp::kIdCx))
          goto IllegalInstruction;

        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Test:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (o0.getSize() != o1.getSize())
          goto IllegalInstruction;

        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        if (o1.getSize() == 1) {
          FIXUP_GPB(o1, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o1.getSize());
          goto EmitX86M;
        }
      }

      // The following instructions use the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();
      opReg = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        rbReg = o0.getId();

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);

          imVal = static_cast<const Imm&>(o1).getUInt8();
          imLen = 1;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());

          imVal = static_cast<const Imm&>(o1).getInt64();
          imLen = Utils::iMin<uint32_t>(o0.getSize(), 4);
        }

        // Alternate Form - AL, AX, EAX, RAX.
        if (o0.getId() == 0 && !(options & X86Inst::kOptionLongForm)) {
          opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
          opCode |= 0xA8 + (o0.getSize() != 1);
          goto EmitX86Op;
        }

        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Mem, Imm)) {
        if (ASMJIT_UNLIKELY(o0.getSize() == 0))
          goto IllegalInstruction;

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = Utils::iMin<uint32_t>(o0.getSize(), 4);

        opCode += (o0.getSize() != 1);
        ADD_PREFIX_BY_SIZE(o0.getSize());

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Xchg:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, opReg);
          goto EmitX86M;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());
          goto EmitX86M;
        }
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingX86Xadd:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o0.getId();
        opReg = o1.getId();

        if (o0.getSize() != o1.getSize())
          goto IllegalInstruction;

        if (o0.getSize() == 1) {
          FIXUP_GPB(o0, rbReg);
          FIXUP_GPB(o1, opReg);
          goto EmitX86R;
        }
        else {
          opCode++;
          ADD_PREFIX_BY_SIZE(o0.getSize());

          // Special opcode for 'xchg ?ax, reg'.
          if (instId == X86Inst::kIdXchg && (opReg == 0 || rbReg == 0)) {
            opCode &= X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_W;
            opCode |= 0x90;
            // One of `xchg a, b` or `xchg b, a` is AX/EAX/RAX.
            opReg += rbReg;
            goto EmitX86OpWithOpReg;
          }
          else {
            goto EmitX86R;
          }
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode += o1.getSize() != 1;
        ADD_PREFIX_BY_SIZE(o1.getSize());

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Prefetch:
      if (isign3 == ENC_OPS2(Mem, Imm)) {
        opReg = static_cast<const Imm&>(o1).getUInt32() & 0x3;
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingX86Fence:
      rbReg = 0;
      goto EmitX86R;

    // ------------------------------------------------------------------------
    // [FPU]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingFpuOp:
      goto EmitFpuOp;

    case X86Inst::kEncodingFpuArith:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // We switch to the alternative opcode if the first operand is zero.
        if (opReg == 0) {
CaseFpuArith_Reg:
          opCode = ((0xD8   << X86Inst::kOpCode_FPU_2B_Shift)       ) +
                   ((opCode >> X86Inst::kOpCode_FPU_2B_Shift) & 0xFF) + rbReg;
          goto EmitFpuOp;
        }
        else if (rbReg == 0) {
          rbReg = opReg;
          opCode = ((0xDC   << X86Inst::kOpCode_FPU_2B_Shift)       ) +
                   ((opCode                                 ) & 0xFF) + rbReg;
          goto EmitFpuOp;
        }
        else {
          goto IllegalInstruction;
        }
      }

      if (isign3 == ENC_OPS1(Mem)) {
CaseFpuArith_Mem:
        // 0xD8/0xDC, depends on the size of the memory operand; opReg is valid.
        opCode = (o0.getSize() == 4) ? 0xD8 : 0xDC;

        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingFpuCom:
      if (isign3 == 0) {
        rbReg = 1;
        goto CaseFpuArith_Reg;
      }

      if (isign3 == ENC_OPS1(Reg)) {
        rbReg = o0.getId();
        goto CaseFpuArith_Reg;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        goto CaseFpuArith_Mem;
      }
      break;

    case X86Inst::kEncodingFpuFldFst:
      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);

        if (o0.getSize() == 4 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M4)) {
          goto EmitX86M;
        }

        if (o0.getSize() == 8 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M8)) {
          opCode += 4;
          goto EmitX86M;
        }

        if (o0.getSize() == 10 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M10)) {
          opCode = iExtData->getSecondaryOpCode();
          opReg  = x86ExtractO(opCode);
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS1(Reg)) {
        if (instId == X86Inst::kIdFld ) { opCode = (0xD9 << X86Inst::kOpCode_FPU_2B_Shift) + 0xC0 + o0.getId(); goto EmitFpuOp; }
        if (instId == X86Inst::kIdFst ) { opCode = (0xDD << X86Inst::kOpCode_FPU_2B_Shift) + 0xD0 + o0.getId(); goto EmitFpuOp; }
        if (instId == X86Inst::kIdFstp) { opCode = (0xDD << X86Inst::kOpCode_FPU_2B_Shift) + 0xD8 + o0.getId(); goto EmitFpuOp; }
      }
      break;

    case X86Inst::kEncodingFpuM:
      if (isign3 == ENC_OPS1(Mem)) {
        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmMem = x86OpMem(o0);
        if (o0.getSize() == 2 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M2)) {
          opCode += 4;
          goto EmitX86M;
        }

        if (o0.getSize() == 4 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M4)) {
          goto EmitX86M;
        }

        if (o0.getSize() == 8 && iExtData->hasFlag(X86Inst::kInstFlagFPU_M8)) {
          opCode = iExtData->getSecondaryOpCode() & ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);
          opReg  = x86ExtractO(opCode);
          goto EmitX86M;
        }
      }
      break;

    case X86Inst::kEncodingFpuRDef:
      if (isign3 == 0) {
        opCode += 1;
        goto EmitFpuOp;
      }
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingFpuR:
      if (isign3 == ENC_OPS1(Reg)) {
        opCode += o0.getId();
        goto EmitFpuOp;
      }
      break;

    case X86Inst::kEncodingFpuStsw:
      if (isign3 == ENC_OPS1(Reg)) {
        if (ASMJIT_UNLIKELY(o0.getId() != X86Gp::kIdAx))
          goto IllegalInstruction;

        opCode = iExtData->getSecondaryOpCode();
        goto EmitFpuOp;
      }

      if (isign3 == ENC_OPS1(Mem)) {
        // Clear compressed displacement before going to EmitX86M.
        opCode &= ~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);

        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Ext]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExtPextrw:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(x86::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        // Secondary opcode of 'pextrw' instruction (SSE4.1).
        opCode = iExtData->getSecondaryOpCode();
        ADD_66H_P(x86::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtExtract:
      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(x86::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        ADD_66H_P(x86::isXmm(o1));

        imVal = static_cast<const Imm&>(o2).getInt64();
        imLen = 1;

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMov:
      // GP|MMX|XMM <- GP|MMX|XMM
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      // GP|MMX|XMM <- Mem
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }

      // The following instruction uses opCode[1].
      opCode = iExtData->getSecondaryOpCode();

      // Mem <- GP|MMX|XMM
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovnti:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        ADD_REX_W(x86::isGpq(o1));

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovbe:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        if (o0.getSize() == 1)
          goto IllegalInstruction;

        ADD_PREFIX_BY_SIZE(o0.getSize());
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        if (o1.getSize() == 1)
          goto IllegalInstruction;

        ADD_PREFIX_BY_SIZE(o1.getSize());
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovd:
CaseExtMovd:
      opReg = o0.getId();
      ADD_66H_P(x86::isXmm(o0));

      // MMX/XMM <- Gp
      if (isign3 == ENC_OPS2(Reg, Reg) && x86::isGp(o1)) {
        rbReg = o1.getId();
        goto EmitX86R;
      }

      // MMX/XMM <- Mem
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }

      // The following instructions use the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();
      opReg = o1.getId();
      ADD_66H_P(x86::isXmm(o1));

      // GP <- MMX/XMM
      if (isign3 == ENC_OPS2(Reg, Reg) && x86::isGp(o0)) {
        rbReg = o0.getId();
        goto EmitX86R;
      }

      // Mem <- MMX/XMM
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        rmMem = x86OpMem(o0);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtMovq:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // MMX <- MMX
        if (x86::isMm(o0) && x86::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x6F;
          goto EmitX86R;
        }

        // XMM <- XMM
        if (x86::isXmm(o0) && x86::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0x7E;
          goto EmitX86R;
        }

        // MMX <- XMM (MOVDQ2Q)
        if (x86::isMm(o0) && x86::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_F2 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86R;
        }

        // XMM <- MMX (MOVQ2DQ)
        if (x86::isXmm(o0) && x86::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86R;
        }
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        // MMX <- Mem
        if (x86::isMm(o0)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x6F;
          goto EmitX86M;
        }

        // XMM <- Mem
        if (x86::isXmm(o0)) {
          opCode = X86Inst::kOpCode_PP_F3 | X86Inst::kOpCode_MM_0F | 0x7E;
          goto EmitX86M;
        }
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        // Mem <- MMX
        if (x86::isMm(o1)) {
          opCode = X86Inst::kOpCode_PP_00 | X86Inst::kOpCode_MM_0F | 0x7F;
          goto EmitX86M;
        }

        // Mem <- XMM
        if (x86::isXmm(o1)) {
          opCode = X86Inst::kOpCode_PP_66 | X86Inst::kOpCode_MM_0F | 0xD6;
          goto EmitX86M;
        }
      }

      // MOVQ in other case is simply a MOVD instruction promoted to 64-bit.
      opCode |= X86Inst::kOpCode_W;
      goto CaseExtMovd;

    case X86Inst::kEncodingExtRmXMM0:
      if (ASMJIT_UNLIKELY(!o2.isNone() && !x86::isXmm(o2, 0)))
        goto IllegalInstruction;

      isign3 &= 0x3F;
      goto CaseExtRm;

    case X86Inst::kEncodingExtRmZDI:
      if (ASMJIT_UNLIKELY(!o2.isNone() && !x86::isGp(o2, X86Gp::kIdDi)))
        goto IllegalInstruction;

      isign3 &= 0x3F;
      goto CaseExtRm;

    case X86Inst::kEncodingExtRm_Wx:
      ADD_REX_W(x86::isGpq(o0) || o1.getSize() == 8);
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingExtRm:
CaseExtRm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRm_P:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_66H_P(x86::isXmm(o0) | x86::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_66H_P(x86::isXmm(o0));

        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRmRi:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();
      opReg  = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        rbReg = o0.getId();
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtRmRi_P:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        ADD_66H_P(x86::isXmm(o0) | x86::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        ADD_66H_P(x86::isXmm(o0));

        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }

      // The following instruction uses the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();
      opReg  = x86ExtractO(opCode);

      if (isign3 == ENC_OPS2(Reg, Imm)) {
        ADD_66H_P(x86::isXmm(o0));

        imVal = static_cast<const Imm&>(o1).getInt64();
        imLen = 1;

        rbReg = o0.getId();
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtRmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    case X86Inst::kEncodingExtRmi_P:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        ADD_66H_P(x86::isXmm(o0) | x86::isXmm(o1));

        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        ADD_66H_P(x86::isXmm(o0));

        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [Extrq / Insertq (SSE4A)]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExtExtrq:
      opReg = o0.getId();
      rbReg = o1.getId();

      if (isign3 == ENC_OPS2(Reg, Reg))
        goto EmitX86R;

      // The following instruction uses the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS3(Reg, Imm, Imm)) {
        imVal = (static_cast<const Imm&>(o1).getUInt32()     ) +
                (static_cast<const Imm&>(o2).getUInt32() << 8) ;
        imLen = 2;

        rbReg = x86ExtractO(opCode);
        goto EmitX86R;
      }
      break;

    case X86Inst::kEncodingExtInsertq: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      opReg = o0.getId();
      rbReg = o1.getId();

      if (isign4 == ENC_OPS2(Reg, Reg))
        goto EmitX86R;

      // The following instruction uses the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      if (isign4 == ENC_OPS4(Reg, Reg, Imm, Imm)) {
        imVal = (static_cast<const Imm&>(o2).getUInt32()     ) +
                (static_cast<const Imm&>(o3).getUInt32() << 8) ;
        imLen = 2;
        goto EmitX86R;
      }
      break;
    }

    // ------------------------------------------------------------------------
    // [3dNow]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingExt3dNow:
      // Every 3dNow instruction starts with 0x0F0F and the actual opcode is
      // stored as 8-bit immediate.
      imVal = opCode & 0xFF;
      imLen = 1;

      opCode = X86Inst::kOpCode_MM_0F | 0x0F;
      opReg = o0.getId();

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        rbReg = o1.getId();
        goto EmitX86R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        rmMem = x86OpMem(o1);
        goto EmitX86M;
      }
      break;

    // ------------------------------------------------------------------------
    // [VEX/EVEX]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingVexOp:
      goto EmitVexEvexOp;

    case X86Inst::kEncodingVexKmov:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();

        // Form 'k, reg'.
        if (x86::isGp(o1)) {
          opCode = iExtData->getSecondaryOpCode();
          goto EmitVexEvexR;
        }

        // Form 'reg, k'.
        if (x86::isGp(o0)) {
          opCode = iExtData->getSecondaryOpCode() + 1;
          goto EmitVexEvexR;
        }

        // Form 'k, k'.
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);

        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);

        opCode++;
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexM:
      if (isign3 == ENC_OPS1(Mem)) {
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexM_VM:
      if (isign3 == ENC_OPS1(Mem)) {
        opCode |= x86OpCodeLByVMem(o0);
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMr_VM:
      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode |= Utils::iMax(x86OpCodeLByVMem(o0), x86OpCodeLBySize(o1.getSize()));

        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexMri_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexMri:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Mem, Reg, Imm)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRmZDI:
      if (o2.isNone())
        goto CaseVexRm;

      if (ASMJIT_UNLIKELY(!x86::isGp(o2, X86Gp::kIdDi)))
        goto IllegalInstruction;

      isign3 &= 0x3F;
      goto CaseVexRm;

    case X86Inst::kEncodingVexRm_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRm:
CaseVexRm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRm_VM:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode |= Utils::iMax(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize()));
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRmi_Wx:
      ADD_REX_W(x86::isGpq(o0) | x86::isGpq(o1));
      goto CaseVexRmi;

    case X86Inst::kEncodingVexRmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmi:
CaseVexRmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvm:
CaseVexRvm:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
CaseVexRvm_R:
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmZDX_Wx:
      if (ASMJIT_UNLIKELY(!o3.isNone() && !x86::isGp(o3, X86Gp::kIdDx)))
        goto IllegalInstruction;
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvm_Wx:
      ADD_REX_W(x86::isGpq(o0) | x86::isGpq(o1));
      goto CaseVexRvm;

    case X86Inst::kEncodingVexRvm_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      goto CaseVexRvm;

    case X86Inst::kEncodingVexRvmr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmr: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = o3.getId() << 4;
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRvmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmi: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = static_cast<const Imm&>(o3).getInt64();
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRmv_Wx:
      ADD_REX_W(x86::isGpq(o0) | x86::isGpq(o2));
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmv:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRmvRm_VM:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opCode  = iExtData->getSecondaryOpCode();
        opCode |= Utils::iMax(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize()));

        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }

      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmv_VM:
      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opCode |= Utils::iMax(x86OpCodeLByVMem(o1), x86OpCodeLBySize(o0.getSize() | o2.getSize()));

        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;


    case X86Inst::kEncodingVexRmvi: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      imVal = static_cast<const Imm&>(o3).getInt64();
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Mem, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexMovDQ:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        if (x86::isGp(o0)) {
          opCode = iExtData->getSecondaryOpCode();
          opReg = o1.getId();
          rbReg = o0.getId();
          goto EmitVexEvexR;
        }

        if (x86::isGp(o1)) {
          opReg = o0.getId();
          rbReg = o1.getId();
          goto EmitVexEvexR;
        }
      }

      // If this is a 'W' version (movq) then allow also vmovq 'xmm|xmm' form.
      if (opCode & X86Inst::kOpCode_EW)
        goto CaseVexRmMr;
      else
        goto CaseVexRmMr_AfterRegReg;

    case X86Inst::kEncodingVexRmMr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRmMr:
CaseVexRmMr:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

CaseVexRmMr_AfterRegReg:
      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_L_Mask;
      opCode |= iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmRmv:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();

        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmMem = x86OpMem(o1);

        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      break;

    case X86Inst::kEncodingVexRvmRmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmRmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode &= X86Inst::kOpCode_L_Mask;
      opCode |= iExtData->getSecondaryOpCode();

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmRmvRmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o2.getId());
        rmMem = x86OpMem(o1);

        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = o0.getId();
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmMr:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }

      // The following instructions use the secondary opcode.
      opCode = iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = o1.getId();
        rbReg = o0.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmMvr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmMvr:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_L_Mask;
      opCode |= iExtData->getSecondaryOpCode();

      if (isign3 == ENC_OPS3(Mem, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o2.getId(), o1.getId());
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvmVmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvmVmi:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);
        goto EmitVexEvexM;
      }

      // The following instruction uses the secondary opcode.
      opCode &= X86Inst::kOpCode_L_Mask;
      opCode |= iExtData->getSecondaryOpCode();
      opReg = x86ExtractO(opCode);

      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexVm_Wx:
      ADD_REX_W(x86::isGpq(o0) | x86::isGpq(o1));
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVm:
      if (isign3 == ENC_OPS2(Reg, Reg)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexVmi_VexEvex_Lx:
      if (isign3 == ENC_OPS3(Reg, Mem, Imm))
        opCode |= X86Inst::kOpCode_MM_ForceEvex;
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVmi_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexVmi:
      imVal = static_cast<const Imm&>(o2).getInt64();
      imLen = 1;

      if (isign3 == ENC_OPS3(Reg, Reg, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rbReg = o1.getId();
        goto EmitVexEvexR;
      }

      if (isign3 == ENC_OPS3(Reg, Mem, Imm)) {
        opReg = x86PackRegAndVvvvv(opReg, o0.getId());
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }
      break;

    case X86Inst::kEncodingVexRvrmRvmr_Lx:
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingVexRvrmRvmr: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal = o2.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o3);

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);

        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexRvrmiRvmri_Lx: {
      if (!(options & CodeEmitter::kOptionHasOp4) || !_op4.isImm())
        goto IllegalInstruction;

      const uint32_t isign4 = isign3 + (o3.getOp() << 9);
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize() | o2.getSize() | o3.getSize());

      imVal = static_cast<const Imm&>(_op4).getUInt8() & 0x0F;
      imLen = 1;

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal |= o3.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal |= o2.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o3);

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal |= o3.getId() << 4;
        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);

        goto EmitVexEvexM;
      }
      break;
    }

    case X86Inst::kEncodingVexMovSsSd:
      if (isign3 == ENC_OPS3(Reg, Reg, Reg)) {
        goto CaseVexRvm_R;
      }

      if (isign3 == ENC_OPS2(Reg, Mem)) {
        opReg = o0.getId();
        rmMem = x86OpMem(o1);
        goto EmitVexEvexM;
      }

      if (isign3 == ENC_OPS2(Mem, Reg)) {
        opCode = iExtData->getSecondaryOpCode();
        opReg = o1.getId();
        rmMem = x86OpMem(o0);
        goto EmitVexEvexM;
      }
      break;

    // ------------------------------------------------------------------------
    // [FMA4]
    // ------------------------------------------------------------------------

    case X86Inst::kEncodingFma4_Lx:
      // It's fine to just check the first operand, second is just for sanity.
      opCode |= x86OpCodeLBySize(o0.getSize() | o1.getSize());
      ASMJIT_FALLTHROUGH;

    case X86Inst::kEncodingFma4: {
      const uint32_t isign4 = isign3 + (o3.getOp() << 9);

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rbReg = o2.getId();

        goto EmitVexEvexR;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Reg, Mem)) {
        imVal = o2.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o3);

        ADD_VEX_W(true);
        goto EmitVexEvexM;
      }

      if (isign4 == ENC_OPS4(Reg, Reg, Mem, Reg)) {
        imVal = o3.getId() << 4;
        imLen = 1;

        opReg = x86PackRegAndVvvvv(o0.getId(), o1.getId());
        rmMem = x86OpMem(o2);

        goto EmitVexEvexM;
      }
      break;
    }
  }
  goto IllegalInstruction;

  // --------------------------------------------------------------------------
  // [Emit - X86]
  // --------------------------------------------------------------------------

EmitX86Op:
  // Emit mandatory instruction prefix.
  EMIT_PP(opCode);

  // Emit REX prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options);
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto IllegalInstruction;
      EMIT_BYTE(rex | kX86ByteRex);
    }
  }

  // Emit instruction opcodes.
  EMIT_MM_OP(opCode);

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86OpWithOpReg:
  // Emit mandatory instruction prefix.
  EMIT_PP(opCode);

  // Emit REX prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options) |
                   (opReg >> 3); // Rex.B (0x01).
    if (rex) {
      EMIT_BYTE(rex | kX86ByteRex);
      if (options & X86Inst::_kOptionInvalidRex)
        goto IllegalInstruction;
      opReg &= 0x7;
    }
  }

  // Emit instruction opcodes.
  opCode += opReg;
  EMIT_MM_OP(opCode);

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86R:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // Rex prefix (64-bit only).
  {
    uint32_t rex = x86ExtractREX(opCode, options) |
                   ((opReg & 0x08) >> 1) | // REX.R (0x04).
                   ((rbReg       ) >> 3) ; // REX.B (0x01).
    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto IllegalInstruction;
      EMIT_BYTE(rex | kX86ByteRex);
      opReg &= 0x07;
      rbReg &= 0x07;
    }
  }

  // Instruction opcodes.
  EMIT_MM_OP(opCode);
  // ModR.
  EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

EmitX86M:
  ASMJIT_ASSERT(rmMem != nullptr);
  ASMJIT_ASSERT(rmMem->getOp() == Operand::kOpMem);
  rmInfo = x86MemInfo[rmMem->getBaseIndexType()];

  // GP instructions have never compressed displacement specified.
  ASMJIT_ASSERT((opCode & X86Inst::kOpCode_CDSHL_Mask) == 0);

  // Address-override prefix.
  if (ASMJIT_UNLIKELY(rmInfo & _getAddressOverrideMask()))
    EMIT_BYTE(0x67);

  // Segment override prefix.
  if (rmMem->hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmMem->getSegmentId()]);

  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  rbReg = rmMem->getBaseId();
  rxReg = rmMem->getIndexId();

  // REX prefix (64-bit only).
  {
    uint32_t rex;

    rex  = (rbReg >> 3) & 0x01; // REX.B (0x01).
    rex |= (rxReg >> 2) & 0x02; // REX.X (0x02).
    rex |= (opReg >> 1) & 0x04; // REX.R (0x04).

    rex &= rmInfo;
    rex |= x86ExtractREX(opCode, options);

    if (rex) {
      if (options & X86Inst::_kOptionInvalidRex)
        goto IllegalInstruction;
      EMIT_BYTE(rex | kX86ByteRex);
      opReg &= 0x07;
    }
  }

  // Instruction opcodes.
  EMIT_MM_OP(opCode);
  // ... Fall through ...

  // --------------------------------------------------------------------------
  // [Emit - MOD/SIB]
  // --------------------------------------------------------------------------

EmitModSib:
  if (!(rmInfo & kX86MemInfo_Index)) {
    // ==========|> [BASE + DISP8|DISP32].
    if (rmInfo & kX86MemInfo_BaseGp) {
      rbReg &= 0x7;
      dispOffset = rmMem->getOffsetLo32();
      uint32_t mod = x86EncodeMod(0, opReg, rbReg);

      if (rbReg == X86Gp::kIdSp) {
        // [XSP|R12].
        if (dispOffset == 0) {
          EMIT_BYTE(mod);
          EMIT_BYTE(x86EncodeSib(0, 4, 4));
        }
        // [XSP|R12 + DISP8|DISP32].
        else {
          uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
          int32_t cdOffset = dispOffset >> cdShift;

          if (Utils::isInt8(cdOffset) && dispOffset == (cdOffset << cdShift)) {
            EMIT_BYTE(mod + 0x40); // <- MOD(1, opReg, rbReg).
            EMIT_BYTE(x86EncodeSib(0, 4, 4));
            EMIT_BYTE(cdOffset & 0xFF);
          }
          else {
            EMIT_BYTE(mod + 0x80); // <- MOD(2, opReg, rbReg).
            EMIT_BYTE(x86EncodeSib(0, 4, 4));
            EMIT_DWORD(static_cast<int32_t>(dispOffset));
          }
        }
      }
      else if (rbReg != X86Gp::kIdBp && dispOffset == 0) {
        // [BASE].
        EMIT_BYTE(mod);
      }
      else {
        // [BASE + DISP8|DISP32].
        uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
        int32_t cdOffset = dispOffset >> cdShift;

        if (Utils::isInt8(cdOffset) && dispOffset == (cdOffset << cdShift)) {
          EMIT_BYTE(mod + 0x40);
          EMIT_BYTE(cdOffset & 0xFF);
        }
        else {
          EMIT_BYTE(mod + 0x80);
          EMIT_DWORD(static_cast<int32_t>(dispOffset));
        }
      }
    }
    // ==========|> [DISP32].
    else if (!(rmInfo & (kX86MemInfo_BaseLabel | kX86MemInfo_BaseRip))) {
      dispOffset = rmMem->getOffsetLo32();
      EMIT_BYTE(x86EncodeMod(0, opReg, 5));
      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
    // ==========|> [LABEL|RIP + DISP32]
    else {
      EMIT_BYTE(x86EncodeMod(0, opReg, 5));

      if (getArchType() == Arch::kTypeX86) {
EmitModSib_LabelRip_X86:
        dispOffset = rmMem->getOffsetLo32();
        if (rmInfo & kX86MemInfo_BaseLabel) {
          // [LABEL->ABS].
          label = _code->getLabelEntry(rmMem->getBaseId());
          if (!label) goto InvalidLabel;

          relocId = _code->_relocations.getLength();
          CodeHolder::RelocEntry re;
          re.type = kRelocRelToAbs;
          re.size = 4;
          re.from = static_cast<uint64_t>((uintptr_t)(cursor - _bufferData));
          re.data = static_cast<int64_t>(dispOffset);

          if (_code->_relocations.append(re) != kErrorOk)
            return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

          if (label->offset != -1) {
            // Bound label.
            _code->_relocations[relocId].data += static_cast<int64_t>(label->offset);
            EMIT_DWORD(0);
          }
          else {
            // Non-bound label.
            dispOffset = -4 - imLen;
            dispSize = 4;
            goto EmitDisplacement;
          }
        }
        else {
          // [RIP->ABS].
          relocId = _code->_relocations.getLength();

          CodeHolder::RelocEntry re;
          re.type = kRelocRelToAbs;
          re.size = 4;
          re.from = static_cast<uint64_t>((uintptr_t)(cursor - _bufferData));
          re.data = re.from + static_cast<int64_t>(dispOffset);

          if (_code->_relocations.append(re) != kErrorOk)
            return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

          EMIT_DWORD(0);
        }
      }
      else {
EmitModSib_LabelRip_X64:
        dispOffset = rmMem->getOffsetLo32();
        if (rmInfo & kX86MemInfo_BaseLabel) {
          // [RIP].
          label = _code->getLabelEntry(rmMem->getBaseId());
          if (!label) goto InvalidLabel;

          dispOffset -= (4 + imLen);
          if (label->offset != -1) {
            // Bound label.
            dispOffset += label->offset - static_cast<int32_t>((intptr_t)(cursor - _bufferData));
            EMIT_DWORD(static_cast<int32_t>(dispOffset));
          }
          else {
            // Non-bound label.
            dispSize = 4;
            relocId = -1;
            goto EmitDisplacement;
          }
        }
        else {
          // [RIP].
          EMIT_DWORD(static_cast<int32_t>(dispOffset));
        }
      }
    }
  }
  else {
    // ESP|RSP can't be used as INDEX in pure SIB mode, however, VSIB mode
    // allows XMM4|YMM4|ZMM4 (that's why the check is before the label).
    if (rxReg == X86Gp::kIdSp) goto IllegalAddressing;

EmitModVSib:
    rxReg &= 0x7;

    // ==========|> [BASE + INDEX + DISP8|DISP32].
    if (rmInfo & kX86MemInfo_BaseGp) {
      rbReg &= 0x7;
      dispOffset = rmMem->getOffsetLo32();

      uint32_t mod = x86EncodeMod(0, opReg, 4);
      uint32_t sib = x86EncodeSib(rmMem->getShift(), rxReg, rbReg);

      if (dispOffset == 0 && rbReg != X86Gp::kIdBp) {
        // [BASE + INDEX << SHIFT].
        EMIT_BYTE(mod);
        EMIT_BYTE(sib);
      }
      else {
        uint32_t cdShift = (opCode & X86Inst::kOpCode_CDSHL_Mask) >> X86Inst::kOpCode_CDSHL_Shift;
        int32_t cdOffset = dispOffset >> cdShift;

        if (Utils::isInt8(cdOffset) && dispOffset == (cdOffset << cdShift)) {
          // [BASE + INDEX << SHIFT + DISP8].
          EMIT_BYTE(mod + 0x40); // <- MOD(1, opReg, 4).
          EMIT_BYTE(sib);
          EMIT_BYTE(cdOffset & 0xFF);
        }
        else {
          // [BASE + INDEX << SHIFT + DISP32].
          EMIT_BYTE(mod + 0x80); // <- MOD(2, opReg, 4).
          EMIT_BYTE(sib);
          EMIT_DWORD(static_cast<int32_t>(dispOffset));
        }
      }
    }
    // ==========|> [INDEX + DISP32].
    else if (!(rmInfo & (kX86MemInfo_BaseLabel | kX86MemInfo_BaseRip))) {
      // [INDEX << SHIFT + DISP32].
      EMIT_BYTE(x86EncodeMod(0, opReg, 4));
      EMIT_BYTE(x86EncodeSib(rmMem->getShift(), rxReg, 5));

      // [DISP32 - from rmMem].
      dispOffset = rmMem->getOffsetLo32();
      EMIT_DWORD(static_cast<int32_t>(dispOffset));
    }
    // ==========|> [LABEL|RIP + INDEX + DISP32].
    else {
      if (getArchType() == Arch::kTypeX86) {
        EMIT_BYTE(x86EncodeMod(0, opReg, 4));
        EMIT_BYTE(x86EncodeSib(rmMem->getShift(), rxReg, 5));
        goto EmitModSib_LabelRip_X86;
      }
      else {
        goto IllegalAddressing;
      }
    }
  }

  if (imLen != 0)
    goto EmitImm;
  else
    goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - FPU]
  // --------------------------------------------------------------------------

EmitFpuOp:
  // Mandatory instruction prefix.
  EMIT_PP(opCode);

  // FPU instructions consist of two opcodes.
  EMIT_BYTE(opCode >> X86Inst::kOpCode_FPU_2B_Shift);
  EMIT_BYTE(opCode);
  goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - VEX / EVEX]
  // --------------------------------------------------------------------------

EmitVexEvexOp:
  {
    // These don't use immediate.
    ASMJIT_ASSERT(imLen == 0);

    uint32_t x = ((opCode & X86Inst::kOpCode_MM_Mask   ) >> (X86Inst::kOpCode_MM_Shift    )) |
                 ((opCode & X86Inst::kOpCode_L_Mask    ) >> (X86Inst::kOpCode_L_Shift - 10)) |
                 ((opCode & X86Inst::kOpCode_PP_VEXMask) >> (X86Inst::kOpCode_PP_Shift - 8)) |
                 (options & X86Inst::kOptionVex3);       // [........|........|.....Lpp|...mmmmm].

    // Only 'vzeroall' and 'vzeroupper' instructions use this encoding, they
    // don't define 'W' to be '1' so we can just check the 'mmmmm' field. Both
    // functions can encode by using VEV2 prefix so VEV3 is basically only used
    // when forced from outside.
    ASMJIT_ASSERT((opCode & X86Inst::kOpCode_W) == 0);

    if (x & 0x04U) {
      x  = (x & (0x4 ^ 0xFFFF)) << 8;                    // [00000000|00000Lpp|0000m0mm|00000000].
      x ^= (kX86ByteVex3) |                              // [........|00000Lpp|0000m0mm|__VEX3__].
           (0x07U  << 13) |                              // [........|00000Lpp|1110m0mm|__VEX3__].
           (0x0FU  << 19) |                              // [........|01111Lpp|1110m0mm|__VEX3__].
           (opCode << 24) ;                              // [_OPCODE_|01111Lpp|1110m0mm|__VEX3__].

      EMIT_DWORD(x);
      goto EmitDone;
    }
    else {
      x = ((x >> 8) ^ x) ^ 0xF9;
      EMIT_BYTE(kX86ByteVex2);
      EMIT_BYTE(x);
      EMIT_BYTE(opCode);
      goto EmitDone;
    }
  }

EmitVexEvexR:
  {
    // VEX instructions use only 0-1 BYTE immediate.
    ASMJIT_ASSERT(imLen <= 1);

    // Construct `x` - a complete EVEX|VEX prefix.
    uint32_t x = ((opReg << 4) & 0xF980U) |              // [........|........|Vvvvv..R|R.......].
                 ((rbReg << 2) & 0x0060U) |              // [........|........|........|.BB.....].
                 (x86ExtractLLMM(opCode, options));      // [........|.LL.....|Vvvvv..R|RBBmmmmm].
    opReg &= 0x7;

    // Handle {k} and {kz} by a single branch.
    if (options & (CodeEmitter::kOptionHasOpMask | X86Inst::kOptionKZ)) {
      // NOTE: We consider a valid construct internally even when {kz} was
      // specified without specifying the register. In that case it would be
      // `k0` and basically everything should be zeroed. It's valid EVEX.
      if (options & CodeEmitter::kOptionHasOpMask) x |= _opMask.getId() << 16;
      x |= options & X86Inst::kOptionKZ;                 // [........|zLL..aaa|Vvvvv..R|RBBmmmmm].
    }

    // Check if EVEX is required by checking bits in `x` :  [........|xx...xxx|x......x|.x.x....].
    if (x & 0x00C78150U) {
      uint32_t y = ((x << 4) & 0x00080000U) |            // [........|....V...|........|........].
                   ((x >> 4) & 0x00000010U) ;            // [........|....V...|........|...R....].
      x  = (x & 0x00FF78E3U) | y;                        // [........|zLL.Vaaa|0vvvv000|RBBR00mm].
      x  = (x << 8) |                                    // [zLL.Vaaa|0vvvv000|RBBR00mm|00000000].
           ((opCode >> kSHR_W_PP) & 0x00830000U) |       // [zLL.Vaaa|Wvvvv0pp|RBBR00mm|00000000].
           ((opCode >> kSHR_W_EW) & 0x00800000U) ;       // [zLL.Vaaa|Wvvvv0pp|RBBR00mm|00000000] (added EVEX.W).
                                                         //      _     ____    ____
      x ^= 0x087CF000U | kX86ByteEvex;                   // [zLL.Vaaa|Wvvvv1pp|RBBR00mm|01100010].

      EMIT_DWORD(x);
      EMIT_BYTE(opCode);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }

    // Not EVEX, prepare `x` for VEX2 or VEX3:          x = [........|00L00000|0vvvv000|R0B0mmmm].
    x |= ((opCode >> (kSHR_W_PP + 8)) & 0x8300U) |       // [00000000|00L00000|Wvvvv0pp|R0B0mmmm].
         ((x      >> 11             ) & 0x0400U) ;       // [00000000|00L00000|WvvvvLpp|R0B0mmmm].

    // Check if VEX3 is required / forced:                  [........|........|x.......|..x..x..].
    if (x & 0x0008024U) {
      uint32_t xorMsk = x86VEXPrefix[x & 0xF] | (opCode << 24);

      // Clear 'FORCE-VEX3' bit and all high bits.
      x  = (x & (0x4 ^ 0xFFFF)) << 8;                    // [00000000|WvvvvLpp|R0B0m0mm|00000000].
                                                         //            ____    _ _
      x ^= xorMsk;                                       // [_OPCODE_|WvvvvLpp|R1Bmmmmm|VEX3_XOP].
      EMIT_DWORD(x);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }
    else {
      // 'mmmmm' must be '00001'.
      ASMJIT_ASSERT((x & 0x1F) == 0x01);

      x = ((x >> 8) ^ x) ^ 0xF9;
      EMIT_BYTE(kX86ByteVex2);
      EMIT_BYTE(x);
      EMIT_BYTE(opCode);

      rbReg &= 0x7;
      EMIT_BYTE(x86EncodeMod(3, opReg, rbReg));

      if (imLen == 0) goto EmitDone;
      EMIT_BYTE(imVal & 0xFF);
      goto EmitDone;
    }
  }

EmitVexEvexM:
  ASMJIT_ASSERT(rmMem != nullptr);
  ASMJIT_ASSERT(rmMem->getOp() == Operand::kOpMem);
  rmInfo = x86MemInfo[rmMem->getBaseIndexType()];

  // Address-override prefix.
  if (ASMJIT_UNLIKELY(rmInfo & _getAddressOverrideMask()))
    EMIT_BYTE(0x67);

  // Segment override prefix.
  if (rmMem->hasSegment())
    EMIT_BYTE(x86SegmentPrefix[rmMem->getSegmentId()]);

  rbReg = rmMem->hasBaseReg()  ? rmMem->getBaseId()  : uint32_t(0);
  rxReg = rmMem->hasIndexReg() ? rmMem->getIndexId() : uint32_t(0);

  {
    // VEX instructions use only 0-1 BYTE immediate.
    ASMJIT_ASSERT(imLen <= 1);

    // Construct `x` - a complete EVEX|VEX prefix.
    uint32_t x = ((opReg << 4 ) & 0x0000F980U) |         // [........|........|Vvvvv..R|R.......].
                 ((rxReg << 3 ) & 0x00000040U) |         // [........|........|........|.X......].
                 ((rxReg << 15) & 0x00080000U) |         // [........|....X...|........|........].
                 ((rbReg << 2 ) & 0x00000020U) |         // [........|........|........|..B.....].
                 (x86ExtractLLMM(opCode, options));      // [........|.LL.X...|Vvvvv..R|RXBmmmmm].
    opReg &= 0x07U;

    // Handle {k}, {kz}, {1tox} by a single branch.
    if (options & (CodeEmitter::kOptionHasOpMask | X86Inst::kOption1ToX | X86Inst::kOptionKZ)) {
      // NOTE: We consider a valid construct internally even when {kz} was
      // specified without specifying the register. In that case it would be
      // `k0` and basically everything would be zeroed. It's a valid EVEX.
      if (options & CodeEmitter::kOptionHasOpMask) x |= _opMask.getId() << 16;

      x |= options & (X86Inst::kOption1ToX |             // [........|.LLbXaaa|Vvvvv..R|RXBmmmmm].
                      X86Inst::kOptionKZ   );            // [........|zLLbXaaa|Vvvvv..R|RXBmmmmm].
    }

    // Check if EVEX is required by checking bits in `x` :  [........|xx.xxxxx|x......x|...x....].
    if (x & 0x00DF8110U) {
      uint32_t y = ((x << 4) & 0x00080000U) |            // [........|....V...|........|........].
                   ((x >> 4) & 0x00000010U) ;            // [........|....V...|........|...R....].
      x  = (x & 0xFFFF78E3U) | y;                        // [........|zLLbVaaa|0vvvv000|RXBR00mm].
      x  = (x << 8) |                                    // [zLLbVaaa|0vvvv000|RBBR00mm|00000000].
           ((opCode >> kSHR_W_PP) & 0x00830000U) |       // [zLLbVaaa|Wvvvv0pp|RBBR00mm|00000000].
           ((opCode >> kSHR_W_EW) & 0x00800000U) ;       // [zLLbVaaa|Wvvvv0pp|RBBR00mm|00000000] (added EVEX.W).
                                                         //      _     ____    ____
      x ^= 0x087CF000U | kX86ByteEvex;                   // [zLLbVaaa|Wvvvv1pp|RBBR00mm|01100010].

      EMIT_DWORD(x);
      EMIT_BYTE(opCode);

      if (opCode & 0x10000000U) {
        // Broadcast, change the compressed displacement scale to either x4 (SHL 2) or x8 (SHL 3)
        // depending on instruction's W. If 'W' is 1 'SHL' must be 3, otherwise it must be 2.
        opCode &=~static_cast<uint32_t>(X86Inst::kOpCode_CDSHL_Mask);
        opCode |= ((x & 0x00800000U) ? 3 : 2) << X86Inst::kOpCode_CDSHL_Shift;
      }
      else {
        // Add the compressed displacement 'SHF' to the opcode based on 'TTWLL'.
        uint32_t TTWLL = ((opCode >> (X86Inst::kOpCode_CDTT_Shift - 3)) & 0x18) +
                         ((opCode >> (X86Inst::kOpCode_W_Shift    - 2)) & 0x04) +
                         ((x >> 29) & 0x3);
        opCode += x86CDisp8SHL[TTWLL];
      }
    }
    else {
      // Not EVEX, prepare `x` for VEX2 or VEX3:        x = [........|00L00000|0vvvv000|RXB0mmmm].
      x |= ((opCode >> (kSHR_W_PP + 8)) & 0x8300U) |     // [00000000|00L00000|Wvvvv0pp|RXB0mmmm].
           ((x      >> 11             ) & 0x0400U) ;     // [00000000|00L00000|WvvvvLpp|RXB0mmmm].

      // Clear a possible CDisp specified by EVEX.
      opCode &= ~X86Inst::kOpCode_CDSHL_Mask;

      // Check if VEX3 is required / forced:                [........|........|x.......|.xx..x..].
      if (x & 0x0008064U) {
        uint32_t xorMsk = x86VEXPrefix[x & 0xF] | (opCode << 24);

        // Clear 'FORCE-VEX3' bit and all high bits.
        x  = (x & (0x4 ^ 0xFFFF)) << 8;                  // [00000000|WvvvvLpp|RXB0m0mm|00000000].
                                                         //            ____    ___
        x ^= xorMsk;                                     // [_OPCODE_|WvvvvLpp|RXBmmmmm|VEX3_XOP].
        EMIT_DWORD(x);
      }
      else {
        // 'mmmmm' must be '00001'.
        ASMJIT_ASSERT((x & 0x1F) == 0x01);

        x = ((x >> 8) ^ x) ^ 0xF9;
        EMIT_BYTE(kX86ByteVex2);
        EMIT_BYTE(x);
        EMIT_BYTE(opCode);
      }
    }
  }

  // MOD|SIB address.
  if (!iExtData->hasFlag(X86Inst::kInstFlagVM))
    goto EmitModSib;

  // MOD|VSIB address without INDEX is invalid.
  if (rmInfo & kX86MemInfo_Index)
    goto EmitModVSib;
  goto IllegalInstruction;

  // --------------------------------------------------------------------------
  // [Emit - Jump/Call to an Immediate]
  // --------------------------------------------------------------------------

  // 64-bit mode requires a trampoline if a relative displacement doesn't fit
  // into a 32-bit address. Old version of AsmJit used to emit jump to a section
  // which contained another jump followed by an address (it worked well for
  // both `jmp` and `call`), but it required to reserve 14-bytes for a possible
  // trampoline.
  //
  // Instead of using 5-byte `jmp/call` and reserving 14 bytes required by the
  // trampoline, it's better to use 6-byte `jmp/call` (prefixing it with REX
  // prefix) and to patch the `jmp/call` instruction to read the address from
  // a memory in case the trampoline is needed.
EmitJmpOrCallAbs:
  {
    CodeHolder::RelocEntry re;
    re.type = kRelocAbsToRel;
    re.size = 4;
    re.from = (intptr_t)(cursor - _bufferData) + 1;
    re.data = static_cast<int64_t>(imVal);

    uint32_t trampolineSize = 0;

    if (getArchType() == Arch::kTypeX64) {
      uint64_t baseAddress = _code->getBaseAddress();

      // If the base address of the output is known, it's possible to determine
      // the need for a trampoline here. This saves possible REX prefix in
      // 64-bit mode and prevents reserving space needed for an absolute address.
      if (baseAddress == kNoBaseAddress || !x64IsRelative(re.data, baseAddress + re.from + 4)) {
        // Emit REX prefix so the instruction can be patched later on. The REX
        // prefix does nothing if not patched after, but allows to patch the
        // instruction in case in which the trampoline is required.
        re.type = kRelocTrampoline;
        re.from++;

        EMIT_BYTE(kX86ByteRex);
        trampolineSize = 8;
      }
    }

    // Both `jmp` and `call` instructions have a single-byte opcode and are
    // followed by a 32-bit displacement.
    EMIT_BYTE(opCode);
    EMIT_DWORD(0);

    if (_code->_relocations.append(re) != kErrorOk)
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    // Reserve space for a possible trampoline.
    _code->_trampolinesSize += trampolineSize;
  }
  goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Displacement]
  // --------------------------------------------------------------------------

EmitDisplacement:
  {
    ASMJIT_ASSERT(label->offset == -1);
    ASMJIT_ASSERT(dispSize == 1 || dispSize == 4);

    // Chain with label.
    CodeHolder::LabelLink* link = _code->newLabelLink();
    // TODO: nullcheck.
    link->prev = label->links;
    link->offset = (intptr_t)(cursor - _bufferData);
    link->displacement = dispOffset;
    link->relocId = relocId;
    label->links = link;

    // Emit label size as dummy data.
    if (dispSize == 1)
      EMIT_BYTE(0x01);
    else // if (dispSize == 4)
      EMIT_DWORD(0x04040404);
  }

  if (imLen == 0)
    goto EmitDone;

  // --------------------------------------------------------------------------
  // [Emit - Immediate]
  // --------------------------------------------------------------------------

EmitImm:
  {
#if ASMJIT_ARCH_64BIT
    uint32_t i = imLen;
    uint64_t imm = static_cast<uint64_t>(imVal);
#else
    uint32_t i = imLen;
    uint32_t imm = static_cast<uint32_t>(imVal & 0xFFFFFFFFU);
#endif

    // Many instructions just use a single byte immediate, so make it fast.
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;
    imm >>= 8;
    EMIT_BYTE(imm & 0xFFU); if (--i == 0) goto EmitDone;

    // Can be 1-4 or 8 bytes, this handles the remaining high DWORD of an 8-byte immediate.
    ASMJIT_ASSERT(i == 4);

#if ASMJIT_ARCH_64BIT
    EMIT_DWORD(imm >> 8);
#else
    EMIT_DWORD(static_cast<uint32_t>((static_cast<uint64_t>(imVal) >> 32) & 0xFFFFFFFFU));
#endif
  }

  // --------------------------------------------------------------------------
  // [Done]
  // --------------------------------------------------------------------------

EmitDone:
#if !defined(ASMJIT_DISABLE_LOGGING)
  // Logging is a performance hit anyway, so make it the unlikely case.
  if (ASMJIT_UNLIKELY(options & CodeEmitter::kOptionLoggingEnabled))
    X86Assembler_logInstruction(this, instId, options, o0, o1, o2, o3, dispSize, imLen, cursor);
#endif // !ASMJIT_DISABLE_LOGGING

  resetOptions();
  resetInlineComment();

  _bufferPtr = cursor;
  return kErrorOk;

  // --------------------------------------------------------------------------
  // [Error Cases]
  // --------------------------------------------------------------------------

UnknownInstruction:
  return X86Assembler_failedInstruction(this,
    DebugUtils::errored(kErrorUnknownInstruction), instId, options, o0, o1, o2, o3);

IllegalInstruction:
  return X86Assembler_failedInstruction(this,
    DebugUtils::errored(kErrorIllegalInstruction), instId, options, o0, o1, o2, o3);

IllegalAddressing:
  return X86Assembler_failedInstruction(this,
    DebugUtils::errored(kErrorIllegalAddressing), instId, options, o0, o1, o2, o3);

IllegalDisplacement:
  return X86Assembler_failedInstruction(this,
    DebugUtils::errored(kErrorIllegalDisplacement), instId, options, o0, o1, o2, o3);

InvalidLabel:
  return X86Assembler_failedInstruction(this,
    DebugUtils::errored(kErrorInvalidLabel), instId, options, o0, o1, o2, o3);
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // ASMJIT_BUILD_X86
