// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_FUNC_H
#define _ASMJIT_BASE_FUNC_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::VirtType]
// ============================================================================

struct VirtType {
  enum Id {
    kIdI8         = 0,
    kIdU8         = 1,
    kIdI16        = 2,
    kIdU16        = 3,
    kIdI32        = 4,
    kIdU32        = 5,
    kIdI64        = 6,
    kIdU64        = 7,
    kIdIntPtr     = 8,
    kIdUIntPtr    = 9,
    kIdF32        = 10,
    kIdF64        = 11,

    kIdMaskReg    = 12,
    kIdSimd64     = 13,
    kIdSimd128    = 14,
    kIdSimd128Ss  = 15,
    kIdSimd128Sd  = 16,
    kIdSimd128Ps  = 17,
    kIdSimd128Pd  = 18,
    kIdSimd256    = 19,
    kIdSimd256Ps  = 20,
    kIdSimd256Pd  = 21,
    kIdSimd512    = 22,
    kIdSimd512Ps  = 23,
    kIdSimd512Pd  = 24,
    kIdCount      = 25,

    kIdX86K       = kIdMaskReg,
    kIdX86Mm      = kIdSimd64,
    kIdX86Xmm     = kIdSimd128,
    kIdX86XmmSs   = kIdSimd128Ss,
    kIdX86XmmSd   = kIdSimd128Sd,
    kIdX86XmmPs   = kIdSimd128Ps,
    kIdX86XmmPd   = kIdSimd128Pd,
    kIdX86Ymm     = kIdSimd256,
    kIdX86YmmPs   = kIdSimd256Ps,
    kIdX86YmmPd   = kIdSimd256Pd,
    kIdX86Zmm     = kIdSimd512,
    kIdX86ZmmPs   = kIdSimd512Ps,
    kIdX86ZmmPd   = kIdSimd512Pd
  };

  enum Flags {
    kFlagF32      = 0x01,
    kFlagF64      = 0x02,
    kFlagVector   = 0x04
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE bool isValidTypeId(uint32_t id) noexcept { return id < kIdCount; }
  static ASMJIT_INLINE bool isIntTypeId(uint32_t id) noexcept { return id >= kIdI8 && id <= kIdUIntPtr; }
  static ASMJIT_INLINE bool isFloatTypeId(uint32_t id) noexcept { return id >= kIdF32 && id <= kIdF64; }

  ASMJIT_INLINE uint32_t getSignature() const noexcept { return _regInfo.signature; }
  ASMJIT_INLINE uint32_t getRegType() const noexcept { return _regInfo.regType; }
  ASMJIT_INLINE uint32_t getRegClass() const noexcept { return _regInfo.regClass; }
  ASMJIT_INLINE uint32_t getRegSize() const noexcept { return _regInfo.size; }

  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _typeId; }
  ASMJIT_INLINE uint32_t getTypeSize() const noexcept { return _typeSize; }
  ASMJIT_INLINE uint32_t getTypeFlags() const noexcept { return _typeFlags; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Operand::RegInfo _regInfo;             //!< Target register information.
  uint32_t _typeId;                      //!< Type id.
  uint16_t _typeSize;                    //!< Type size (doesn't have to match the register size).
  uint16_t _typeFlags;                   //!< Type flags
  char _typeName[12];                    //!< Type name.
};

// ============================================================================
// [asmjit::FuncHint]
// ============================================================================

//! Function hints.
//!
//! For a platform specific calling conventions, see:
//!   - `X86FuncHint` - X86/X64 function hints.
ASMJIT_ENUM(FuncHint) {
  //! Generate a naked function by omitting its prolog and epilog (default true).
  //!
  //! Naked functions should always result in less code required for function's
  //! prolog and epilog. In addition, on X86/64 naked functions save one register
  //! (ebp or rbp), which can be used by the function instead.
  kFuncHintNaked = 0,

  //! Generate a compact function prolog/epilog if possible (default true).
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Use shorter, but possible slower prolog/epilog sequence to save/restore
  //! registers. At the moment this only enables emitting `leave` in function's
  //! epilog to make the code shorter, however, the counterpart `enter` is not
  //! used in function's prolog for performance reasons.
  kFuncHintCompact = 1,

  //! Emit `emms` instruction in the function's epilog.
  kFuncHintX86Emms = 17,
  //! Emit `sfence` instruction in the function's epilog.
  kFuncHintX86SFence = 18,
  //! Emit `lfence` instruction in the function's epilog.
  kFuncHintX86LFence = 19
};

// ============================================================================
// [asmjit::FuncFlags]
// ============================================================================

//! Function flags.
ASMJIT_ENUM(FuncFlags) {
  //! Whether the function is using naked (minimal) prolog / epilog.
  kFuncFlagIsNaked = 0x00000001,

  //! Whether an another function is called from this function.
  kFuncFlagIsCaller = 0x00000002,

  //! Whether the stack is not aligned to the required stack alignment,
  //! thus it has to be aligned manually.
  kFuncFlagIsStackMisaligned = 0x00000004,

  //! Whether the stack pointer is adjusted by the stack size needed
  //! to save registers and function variables.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Stack pointer (ESP/RSP) is adjusted by 'sub' instruction in prolog and by
  //! 'add' instruction in epilog (only if function is not naked). If function
  //! needs to perform manual stack alignment more instructions are used to
  //! adjust the stack (like "and zsp, -Alignment").
  kFuncFlagIsStackAdjusted = 0x00000008,

  //! Whether the function is finished using `Compiler::endFunc()`.
  kFuncFlagIsFinished = 0x80000000,

  //! Whether to emit `leave` instead of two instructions in case that the
  //! function saves and restores the frame pointer.
  kFuncFlagX86Leave = 0x00010000,

  //! Whether it's required to move arguments to a new stack location,
  //! because of manual aligning.
  kFuncFlagX86MoveArgs = 0x00040000,

  //! Whether to emit `emms` instruction in epilog (auto-detected).
  kFuncFlagX86Emms = 0x01000000,

  //! Whether to emit `sfence` instruction in epilog (auto-detected).
  //!
  //! `kFuncFlagX86SFence` with `kFuncFlagX86LFence` results in emitting `mfence`.
  kFuncFlagX86SFence = 0x02000000,

  //! Whether to emit `lfence` instruction in epilog (auto-detected).
  //!
  //! `kFuncFlagX86SFence` with `kFuncFlagX86LFence` results in emitting `mfence`.
  kFuncFlagX86LFence = 0x04000000
};

// ============================================================================
// [asmjit::FuncDir]
// ============================================================================

//! Function arguments direction.
ASMJIT_ENUM(FuncDir) {
  //! Arguments are passed left to right.
  //!
  //! This arguments direction is unusual in C, however it's used in Pascal.
  kFuncDirLTR = 0,

  //! Arguments are passed right ro left
  //!
  //! This is the default argument direction in C.
  kFuncDirRTL = 1
};

// ============================================================================
// [asmjit::FuncMisc]
// ============================================================================

enum {
  //! Function doesn't have variable number of arguments (`...`) (default).
  kFuncNoVarArgs = 0xFF,
  //! Invalid stack offset in function or function parameter.
  kFuncStackInvalid = -1
};

// ============================================================================
// [asmjit::FuncArgIndex]
// ============================================================================

//! Function argument index (lo/hi).
ASMJIT_ENUM(FuncArgIndex) {
  //! Maxumum number of function arguments supported by AsmJit.
  kFuncArgCount = 16,
  //! Extended maximum number of arguments (used internally).
  kFuncArgCountLoHi = kFuncArgCount * 2,

  //! Index to the LO part of function argument (default).
  //!
  //! This value is typically omitted and added only if there is HI argument
  //! accessed.
  kFuncArgLo = 0,

  //! Index to the HI part of function argument.
  //!
  //! HI part of function argument depends on target architecture. On x86 it's
  //! typically used to transfer 64-bit integers (they form a pair of 32-bit
  //! integers).
  kFuncArgHi = kFuncArgCount
};

// ============================================================================
// [asmjit::FuncRet]
// ============================================================================

//! Function return value (lo/hi) specification.
ASMJIT_ENUM(FuncRet) {
  //! Index to the LO part of function return value.
  kFuncRetLo = 0,
  //! Index to the HI part of function return value.
  kFuncRetHi = 1
};

// ============================================================================
// [asmjit::TypeId]
// ============================================================================

//! Function builder's `void` type.
struct Void {};

//! Function builder's `int8_t` type.
struct Int8Type {};
//! Function builder's `uint8_t` type.
struct UInt8Type {};

//! Function builder's `int16_t` type.
struct Int16Type {};
//! Function builder's `uint16_t` type.
struct UInt16Type {};

//! Function builder's `int32_t` type.
struct Int32Type {};
//! Function builder's `uint32_t` type.
struct UInt32Type {};

//! Function builder's `int64_t` type.
struct Int64Type {};
//! Function builder's `uint64_t` type.
struct UInt64Type {};

//! Function builder's `intptr_t` type.
struct IntPtrType {};
//! Function builder's `uintptr_t` type.
struct UIntPtrType {};

//! Function builder's `float` type.
struct FloatType {};
//! Function builder's `double` type.
struct DoubleType {};

#if !defined(ASMJIT_DOCGEN)
template<typename T>
struct TypeId {}; // Let it fail here if `T` was not specialized.

template<typename T>
struct TypeId<T*> { enum { kId = VirtType::kIdUIntPtr }; };

template<typename T>
struct VirtTypeIdOfInt {
  enum { kId = (sizeof(T) == 1) ? (int)(IntTraits<T>::kIsSigned ? VirtType::kIdI8  : VirtType::kIdU8 ) :
               (sizeof(T) == 2) ? (int)(IntTraits<T>::kIsSigned ? VirtType::kIdI16 : VirtType::kIdU16) :
               (sizeof(T) == 4) ? (int)(IntTraits<T>::kIsSigned ? VirtType::kIdI32 : VirtType::kIdU32) :
               (sizeof(T) == 8) ? (int)(IntTraits<T>::kIsSigned ? VirtType::kIdI64 : VirtType::kIdU64) : (int)kInvalidValue
  };
};

#define ASMJIT_TYPE_ID(T, ID) \
  template<> struct TypeId<T> { enum { kId = ID }; }

ASMJIT_TYPE_ID(void              , kInvalidValue);
ASMJIT_TYPE_ID(signed char       , VirtTypeIdOfInt<signed char>::kId);
ASMJIT_TYPE_ID(unsigned char     , VirtTypeIdOfInt<unsigned char>::kId);
ASMJIT_TYPE_ID(short             , VirtTypeIdOfInt<short>::kId);
ASMJIT_TYPE_ID(unsigned short    , VirtTypeIdOfInt<unsigned short>::kId);
ASMJIT_TYPE_ID(int               , VirtTypeIdOfInt<int>::kId);
ASMJIT_TYPE_ID(unsigned int      , VirtTypeIdOfInt<unsigned int>::kId);
ASMJIT_TYPE_ID(long              , VirtTypeIdOfInt<long>::kId);
ASMJIT_TYPE_ID(unsigned long     , VirtTypeIdOfInt<unsigned long>::kId);
ASMJIT_TYPE_ID(float             , VirtType::kIdF32);
ASMJIT_TYPE_ID(double            , VirtType::kIdF64);

#if ASMJIT_CC_HAS_NATIVE_CHAR
ASMJIT_TYPE_ID(char              , VirtTypeIdOfInt<char>::kId);
#endif
#if ASMJIT_CC_HAS_NATIVE_WCHAR_T
ASMJIT_TYPE_ID(wchar_t           , VirtTypeIdOfInt<wchar_t>::kId);
#endif
#if ASMJIT_CC_HAS_NATIVE_CHAR16_T
ASMJIT_TYPE_ID(char16_t          , VirtTypeIdOfInt<char16_t>::kId);
#endif
#if ASMJIT_CC_HAS_NATIVE_CHAR32_T
ASMJIT_TYPE_ID(char32_t          , VirtTypeIdOfInt<char32_t>::kId);
#endif

#if ASMJIT_CC_MSC && !ASMJIT_CC_MSC_GE(16, 0, 0)
ASMJIT_TYPE_ID(__int64           , VirtTypeIdOfInt<__int64>::kId);
ASMJIT_TYPE_ID(unsigned __int64  , VirtTypeIdOfInt<unsigned __int64>::kId);
#else
ASMJIT_TYPE_ID(long long         , VirtTypeIdOfInt<long long>::kId);
ASMJIT_TYPE_ID(unsigned long long, VirtTypeIdOfInt<unsigned long long>::kId);
#endif

ASMJIT_TYPE_ID(Void              , kInvalidValue);
ASMJIT_TYPE_ID(Int8Type          , VirtType::kIdI8);
ASMJIT_TYPE_ID(UInt8Type         , VirtType::kIdU8);
ASMJIT_TYPE_ID(Int16Type         , VirtType::kIdI16);
ASMJIT_TYPE_ID(UInt16Type        , VirtType::kIdU16);
ASMJIT_TYPE_ID(Int32Type         , VirtType::kIdI32);
ASMJIT_TYPE_ID(UInt32Type        , VirtType::kIdU32);
ASMJIT_TYPE_ID(Int64Type         , VirtType::kIdI64);
ASMJIT_TYPE_ID(UInt64Type        , VirtType::kIdU64);
ASMJIT_TYPE_ID(IntPtrType        , VirtType::kIdIntPtr);
ASMJIT_TYPE_ID(UIntPtrType       , VirtType::kIdUIntPtr);
ASMJIT_TYPE_ID(FloatType         , VirtType::kIdF32);
ASMJIT_TYPE_ID(DoubleType        , VirtType::kIdF64);
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::FuncInOut]
// ============================================================================

//! Function in/out - argument or return value translated from `FuncPrototype`.
struct FuncInOut {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _typeId; }

  ASMJIT_INLINE bool hasRegId() const noexcept { return _regId != kInvalidReg; }
  ASMJIT_INLINE uint32_t getRegId() const noexcept { return _regId; }

  ASMJIT_INLINE bool hasStackOffset() const noexcept { return _stackOffset != kFuncStackInvalid; }
  ASMJIT_INLINE int32_t getStackOffset() const noexcept { return static_cast<int32_t>(_stackOffset); }

  //! Get whether the argument / return value is assigned.
  ASMJIT_INLINE bool isSet() const noexcept {
    return (_regId != kInvalidReg) | (_stackOffset != kFuncStackInvalid);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the function argument to "unassigned state".
  ASMJIT_INLINE void reset() noexcept { _packed = ASMJIT_UINT64_C(0xFFFFFFFFFFFFFFFF); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint32_t _typeId;                  //!< Type-id, see \ref VirtType::Id.
      uint8_t _regId;                    //!< Register index if arg|reg is a register.
      uint8_t _reserved;                 //!< Reserved.
      int16_t _stackOffset;              //!< Stack offset if arg|reg is on the stack.
    };

    uint64_t _packed;
  };
};

// ============================================================================
// [asmjit::FuncPrototype]
// ============================================================================

//! Function prototype.
//!
//! Function prototype contains information about function return type, count
//! of arguments and their types. Function prototype is a low level structure
//! which doesn't contain platform specific or calling convention specific
//! information. Function prototype is used to create a `FuncDecl`.
struct FuncPrototype {
  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  //! Setup the prototype.
  ASMJIT_INLINE void setup(
    uint32_t callConv,
    uint32_t ret,
    const uint32_t* args, uint32_t numArgs) noexcept {

    ASMJIT_ASSERT(callConv <= 0xFF);
    ASMJIT_ASSERT(numArgs <= 0xFF);

    _callConv = static_cast<uint8_t>(callConv);
    _varArgs = kFuncNoVarArgs;
    _numArgs = static_cast<uint8_t>(numArgs);
    _reserved = 0;

    _ret = ret;
    _args = args;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention.
  ASMJIT_INLINE uint32_t getCallConv() const noexcept { return _callConv; }
  //! Get the variable arguments `...` index, `kFuncNoVarArgs` if none.
  ASMJIT_INLINE uint32_t getVarArgs() const noexcept { return _varArgs; }
  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getNumArgs() const noexcept { return _numArgs; }

  //! Get the return value type.
  ASMJIT_INLINE uint32_t getRet() const noexcept { return _ret; }
  //! Get the type of the argument at index `i`.
  ASMJIT_INLINE uint32_t getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _numArgs);
    return _args[i];
  }
  //! Get the array of function arguments' types.
  ASMJIT_INLINE const uint32_t* getArgs() const noexcept { return _args; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _callConv;
  uint8_t _varArgs;
  uint8_t _numArgs;
  uint8_t _reserved;

  uint32_t _ret;
  const uint32_t* _args;
};

// ============================================================================
// [asmjit::FuncSignatureX]
// ============================================================================

// TODO: Rename to `DynamicFuncSignature`
//! Custom function builder for up to 32 function arguments.
struct FuncSignatureX : public FuncPrototype {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncSignatureX(uint32_t callConv = kCallConvHost) noexcept {
    setup(callConv, kInvalidValue, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setCallConv(uint32_t callConv) noexcept {
    ASMJIT_ASSERT(callConv <= 0xFF);
    _callConv = static_cast<uint8_t>(callConv);
  }

  //! Set the return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) noexcept {
    _ret = retType;
  }
  //! Set the return type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setRetT() noexcept { setRet(TypeId<T>::kId); }

  //! Set the argument at index `i` to the `type`
  ASMJIT_INLINE void setArg(uint32_t i, uint32_t type) noexcept {
    ASMJIT_ASSERT(i < _numArgs);
    _builderArgList[i] = type;
  }
  //! Set the argument at index `i` to the type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t i) noexcept { setArg(i, TypeId<T>::kId); }

  //! Append an argument of `type` to the function prototype.
  ASMJIT_INLINE void addArg(uint32_t type) noexcept {
    ASMJIT_ASSERT(_numArgs < kFuncArgCount);
    _builderArgList[_numArgs++] = type;
  }
  //! Append an argument of type based on `T` to the function prototype.
  template<typename T>
  ASMJIT_INLINE void addArgT() noexcept { addArg(TypeId<T>::kId); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _builderArgList[kFuncArgCount];
};

//! \internal
#define T(_Type_) TypeId<_Type_>::kId

//! Function prototype (no args).
template<typename RET>
struct FuncSignature0 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature0(uint32_t callConv = kCallConvHost) noexcept {
    setup(callConv, T(RET), nullptr, 0);
  }
};

//! Function prototype (1 argument).
template<typename RET, typename P0>
struct FuncSignature1 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature1(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (2 arguments).
template<typename RET, typename P0, typename P1>
struct FuncSignature2 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature2(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (3 arguments).
template<typename RET, typename P0, typename P1, typename P2>
struct FuncSignature3 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature3(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (4 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3>
struct FuncSignature4 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature4(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (5 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4>
struct FuncSignature5 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature5(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (6 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
struct FuncSignature6 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature6(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (7 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
struct FuncSignature7 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature7(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (8 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
struct FuncSignature8 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature8(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (9 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
struct FuncSignature9 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature9(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7), T(P8) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function prototype (10 arguments).
template<typename RET, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
struct FuncSignature10 : public FuncPrototype {
  ASMJIT_INLINE FuncSignature10(uint32_t callConv = kCallConvHost) noexcept {
    static const uint32_t args[] = { T(P0), T(P1), T(P2), T(P3), T(P4), T(P5), T(P6), T(P7), T(P8), T(P9) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};
#undef T

// ============================================================================
// [asmjit::FuncDecl]
// ============================================================================

//! Function declaration.
struct FuncDecl {
  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention, see `CallConv`.
  ASMJIT_INLINE uint32_t getCallConv() const noexcept { return _callConv; }

  //! Get whether the callee pops the stack.
  ASMJIT_INLINE uint32_t getCalleePopsStack() const noexcept { return _calleePopsStack; }

  //! Get direction of arguments passed on the stack.
  //!
  //! Direction should be always `kFuncDirRTL`.
  //!
  //! NOTE: This is related to used calling convention, it's not affected by
  //! number of function arguments or their types.
  ASMJIT_INLINE uint32_t getArgsDirection() const noexcept { return _argsDirection; }

  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const noexcept { return _argStackSize; }
  //! Get size of "Red Zone".
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! Get size of "Spill Zone".
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }

  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const noexcept { return _retCount != 0; }
  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const noexcept { return _retCount; }

  //! Get function return value.
  ASMJIT_INLINE FuncInOut& getRet(uint32_t index = kFuncRetLo) noexcept { return _rets[index]; }
  //! Get function return value.
  ASMJIT_INLINE const FuncInOut& getRet(uint32_t index = kFuncRetLo) const noexcept { return _rets[index]; }

  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getNumArgs() const noexcept { return _numArgs; }

  //! Get function arguments array.
  ASMJIT_INLINE FuncInOut* getArgs() noexcept { return _args; }
  //! Get function arguments array (const).
  ASMJIT_INLINE const FuncInOut* getArgs() const noexcept { return _args; }

  //! Get function argument at index `index`.
  ASMJIT_INLINE FuncInOut& getArg(size_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE const FuncInOut& getArg(size_t index) const noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  ASMJIT_INLINE void resetArg(size_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    _args[index].reset();
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _callConv;                     //!< Function calling convention.
  uint8_t _calleePopsStack : 1;          //!< Whether a callee pops stack.
  uint8_t _argsDirection : 1;            //!< Stack arguments' direction, see \ref FuncDir.
  uint8_t _reserved0 : 6;                //!< Reserved #0 (alignment).

  uint8_t _numArgs;                      //!< Number of function arguments.
  uint8_t _retCount;                     //!< Number of function return values.

  uint32_t _argStackSize;                //!< Stack arguments' size (aligned).

  uint16_t _redZoneSize;                 //!< Red Zone size (AMD64-ABI == 128 bytes).
  uint16_t _spillZoneSize;               //!< Spill Zone size (WIN64-ABI == 32 bytes).

  //! Function arguments (LO & HI) mapped to physical registers and stack.
  FuncInOut _args[kFuncArgCountLoHi];

  //! Function return value(s).
  FuncInOut _rets[2];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_FUNC_H
