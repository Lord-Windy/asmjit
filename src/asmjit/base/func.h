// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_FUNC_H
#define _ASMJIT_BASE_FUNC_H

#include "../build.h"

// [Dependencies]
#include "../base/operand.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [Forward Declarations]
// ============================================================================

class CodeEmitter;

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

//! Function calling convention.
//!
//! Function calling convention is a scheme that defines how function parameters
//! are passed and how function returns its result. AsmJit defines a variety of
//! architecture and OS specific calling conventions and also provides a compile
//! time detection to make JIT code-generation easier.
struct CallConv {
  //! Calling convention id.
  ASMJIT_ENUM(Id) {
    //! None or invalid (can't be used).
    kIdNone = 0,

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    //! X86 `__cdecl` calling convention (used by C runtime and libraries).
    kIdX86CDecl = 1,
    //! X86 `__stdcall` calling convention (used mostly by WinAPI).
    kIdX86StdCall = 2,
    //! X86 `__thiscall` calling convention (MSVC/Intel).
    kIdX86MsThisCall = 3,
    //! X86 `__fastcall` convention (MSVC/Intel).
    kIdX86MsFastCall = 4,
    //! X86 `__fastcall` convention (GCC and Clang).
    kIdX86GccFastCall = 5,
    //! X86 `regparm(1)` convention (GCC and Clang).
    kIdX86GccRegParm1 = 6,
    //! X86 `regparm(2)` convention (GCC and Clang).
    kIdX86GccRegParm2 = 7,
    //! X86 `regparm(3)` convention (GCC and Clang).
    kIdX86GccRegParm3 = 8,

    //! X64 calling convention defined by WIN64-ABI.
    //!
    //! Links:
    //!   * <http://msdn.microsoft.com/en-us/library/9b372w95.aspx>.
    kIdX86Win64 = 16,
    //! X64 calling convention used by Unix platforms (AMD64-ABI).
    kIdX86Unix64 = 17,

    // ------------------------------------------------------------------------
    // [ARM]
    // ------------------------------------------------------------------------

    //! Legacy calling convention, floating point arguments are passed via GP registers.
    kIdArm32SoftFP = 32,
    //! Modern calling convention, uses VFP registers to pass floating point arguments.
    kIdArm32HardFP = 33,

    // ------------------------------------------------------------------------
    // [Internal]
    // ------------------------------------------------------------------------

    _kIdX86Start = 1,   //!< \internal
    _kIdX86End = 8,     //!< \internal

    _kIdX64Start = 16,  //!< \internal
    _kIdX64End = 17,    //!< \internal

    _kIdArmStart = 32,  //!< \internal
    _kIdArmEnd = 33,    //!< \internal

    // ------------------------------------------------------------------------
    // [Host]
    // ------------------------------------------------------------------------

#if defined(ASMJIT_DOCGEN)
    //! Default calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: This should be always the same as `kIdHostCDecl`, but some
    //! compilers allow to override the default calling convention. Overriding
    //! is not detected at the moment.
    kIdHost         = DETECTED_AT_COMPILE_TIME,

    //! Default CDECL calling convention based on the current C++ compiler's settings.
    kIdHostCDecl    = DETECTED_AT_COMPILE_TIME,

    //! Default STDCALL calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostStdCall  = DETECTED_AT_COMPILE_TIME,

    //! Compatibility for `__fastcall` calling convention.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostFastCall = DETECTED_AT_COMPILE_TIME
#elif ASMJIT_ARCH_X86
    kIdHost         = kIdX86CDecl,
    kIdHostCDecl    = kIdX86CDecl,
    kIdHostStdCall  = kIdX86StdCall,
    kIdHostFastCall = ASMJIT_CC_MSC   ? kIdX86MsFastCall  :
                            ASMJIT_CC_GCC   ? kIdX86GccFastCall :
                            ASMJIT_CC_CLANG ? kIdX86GccFastCall : kIdNone
#elif ASMJIT_ARCH_X64
    kIdHost         = ASMJIT_OS_WINDOWS ? kIdX86Win64 : kIdX86Unix64,
    kIdHostCDecl    = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall  = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall = kIdHost  // Doesn't exist, redirected to host.
#elif ASMJIT_ARCH_ARM32
# if defined(__SOFTFP__)
    kIdHost         = kIdArm32SoftFP,
# else
    kIdHost         = kIdArm32HardFP,
# endif
    // These don't exist on ARM.
    kIdHostCDecl    = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall  = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall = kIdHost  // Doesn't exist, redirected to host.
#else
# error "[asmjit] Couldn't determine the target's calling convention."
#endif
  };

  //! Calling convention algorithm.
  //!
  //! This is AsmJit specific. It basically describes how should AsmJit convert
  //! the function arguments defined by `FuncSignature` into register ids or
  //! stack offsets. The default algorithm is a standard algorithm that assigns
  //! registers first, and then assigns stack. The Win64 algorithm does register
  //! shadowing as defined by `WIN64` calling convention - it applies to 64-bit
  //! calling conventions only.
  ASMJIT_ENUM(Algorithm) {
    kAlgorithmDefault    = 0,            //!< Default algorithm (cross-platform).
    kAlgorithmWin64      = 1             //!< WIN64 specific algorithm.
  };

  //! Calling convention flags.
  ASMJIT_ENUM(Flags) {
    kFlagCalleePopsStack = 0x01,         //!< Callee is responsible for cleaning up the stack.
    kFlagPassFloatsByVec = 0x02,         //!< Pass F32 and F64 arguments by VEC128 register.
    kFlagVectorCall      = 0x04,         //!< This is a '__vectorcall' calling convention.
    kFlagIndirectVecArgs = 0x08          //!< Pass vector arguments indirectly (as a pointer).
  };

  //! Internal limits of CallConv.
  ASMJIT_ENUM(Limits) {
    kNumRegKinds         = 4,            //!< Number of RegKinds handled by CallConv.
    kNumRegArgsPerKind   = 8             //!< Number of maximum arguments passed in register per RegKind.
  };

  //! Passed registers' order.
  struct RegOrder {
    uint8_t id[kNumRegArgsPerKind];      //!< Passed registers, order matters.
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_API Error init(uint32_t ccId) noexcept;

  ASMJIT_INLINE void reset() noexcept {
    ::memset(this, 0, sizeof(*this));
    ::memset(_passedOrder, 0xFF, sizeof(_passedOrder));
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get calling convention id, see \ref Id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Set calling convention id, see \ref Id.
  ASMJIT_INLINE void setId(uint32_t id) noexcept { _id = static_cast<uint8_t>(id); }

  //! Get architecture type.
  ASMJIT_INLINE uint32_t getArchType() const noexcept { return _archType; }
  //! Set architecture type.
  ASMJIT_INLINE void setArchType(uint32_t archType) noexcept { _archType = static_cast<uint8_t>(archType); }

  //! Get calling convention algorithm, see \ref Algorithm.
  ASMJIT_INLINE uint32_t getAlgorithm() const noexcept { return _algorithm; }
  //! Set calling convention algorithm, see \ref Algorithm.
  ASMJIT_INLINE void setAlgorithm(uint32_t algorithm) noexcept { _algorithm = static_cast<uint8_t>(algorithm); }

  //! Get calling convention flags, see \ref Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void setFlags(uint32_t flag) noexcept { _flags = flag; };
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void addFlags(uint32_t flag) noexcept { _flags |= flag; };
  //! Get if the calling convention has the given `flag` set.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }

  //! Get if this calling convention specifies 'RedZone'.
  ASMJIT_INLINE bool hasRedZone() const noexcept { return _redZoneSize != 0; }
  //! Get size of 'RedZone'.
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! Set size of 'RedZone'.
  ASMJIT_INLINE void setRedZoneSize(uint32_t size) noexcept { _redZoneSize = static_cast<uint16_t>(size); }

  //! Get if this calling convention specifies 'SpillZone'.
  ASMJIT_INLINE bool hasSpillZone() const noexcept { return _spillZoneSize != 0; }
  //! Get size of 'SpillZone'.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }
  //! Set size of 'SpillZone'.
  ASMJIT_INLINE void setSpillZoneSize(uint32_t size) noexcept { _spillZoneSize = static_cast<uint16_t>(size); }

  ASMJIT_INLINE const uint8_t* getPassedOrder(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    return _passedOrder[kind].id;
  }

  ASMJIT_INLINE uint32_t getPassedRegs(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    return _passedRegs[kind];
  }

  ASMJIT_INLINE void _setPassedPacked(uint32_t kind, uint32_t p0, uint32_t p1) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);

    reinterpret_cast<uint32_t*>(_passedOrder[kind].id)[0] = p0;
    reinterpret_cast<uint32_t*>(_passedOrder[kind].id)[1] = p1;
  }

  ASMJIT_INLINE void setPassedToNone(uint32_t kind) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);

    _setPassedPacked(kind, ASMJIT_PACK32_4x8(0xFF, 0xFF, 0xFF, 0xFF),
                           ASMJIT_PACK32_4x8(0xFF, 0xFF, 0xFF, 0xFF));
    _passedRegs[kind] = 0;
  }

  ASMJIT_INLINE void setPassedOrder(uint32_t kind, uint32_t a0, uint32_t a1 = 0xFF, uint32_t a2 = 0xFF, uint32_t a3 = 0xFF, uint32_t a4 = 0xFF, uint32_t a5 = 0xFF, uint32_t a6 = 0xFF, uint32_t a7 = 0xFF) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);

    _setPassedPacked(kind, ASMJIT_PACK32_4x8(a0, a1, a2, a3),
                              ASMJIT_PACK32_4x8(a4, a5, a6, a7));

    // NOTE: This should always be called with all arguments known at compile
    // time, so even if it looks scary it should be translated to a single
    // instruction.
    _passedRegs[kind] = (a0 != 0xFF ? 1U << a0 : 0U) |
                        (a1 != 0xFF ? 1U << a1 : 0U) |
                        (a2 != 0xFF ? 1U << a2 : 0U) |
                        (a3 != 0xFF ? 1U << a3 : 0U) |
                        (a4 != 0xFF ? 1U << a4 : 0U) |
                        (a5 != 0xFF ? 1U << a5 : 0U) |
                        (a6 != 0xFF ? 1U << a6 : 0U) |
                        (a7 != 0xFF ? 1U << a7 : 0U) ;
  }

  ASMJIT_INLINE uint32_t getPreservedRegs(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    return _preservedRegs[kind];
  }


  ASMJIT_INLINE void setPreservedRegs(uint32_t kind, uint32_t regs) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    _preservedRegs[kind] = regs;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _id;                           //!< Calling convention id, see \ref Id.
  uint8_t _archType;                     //!< Architecture type (see \ref Arch::Type).
  uint8_t _algorithm;                    //!< Algorithm to create FuncFrame.
  uint8_t _flags;                        //!< Calling convention flags.

  uint16_t _redZoneSize;                 //!< Red zone size (AMD64 == 128 bytes).
  uint16_t _spillZoneSize;               //!< Spill zone size (WIN64 == 32 bytes).

  RegOrder _passedOrder[kNumRegKinds];   //!< Passed registers' order, per kind.
  uint32_t _passedRegs[kNumRegKinds];    //!< Mask of all passed registers, per kind.
  uint32_t _preservedRegs[kNumRegKinds]; //!< Mask of all preserved registers, per kind.
};

// ============================================================================
// [asmjit::FuncFrame]
// ============================================================================

//! Function frame (definition).
//!
//! This structure can be used to create a function frame in a cross-platform
//! way. It contains information about the function's stack to be used and
//! registers to be saved and restored. Based on this information in can
//! calculate the optimal layout of a function as \ref FuncLayout.
struct FuncFrame {
  //! FuncFrame flags.
  //!
  //! These flags are designed in a way that all are initially false, and user
  //! or finalizer sets them when necessary. Architecture-specific flags are
  //! prefixed with the architecture name.
  ASMJIT_ENUM(Flags) {
    kFlagPreserveFP       = 0x00000002U, //!< Preserve frame pointer (EBP|RBP).
    kFlagHasCalls         = 0x00000001U, //!< Function calls other functions (is not leaf).
    kFlagCompact          = 0x00008000U, //!< Use smaller, but possibly slower prolog/epilog.

    kX86FlagAlignedVecSR  = 0x00010000U, //!< Use aligned save/restore of VEC regs.
    kX86FlagMmxCleanup    = 0x00020000U, //!< Emit EMMS instruction in epilog (X86).
    kX86FlagAvxCleanup    = 0x00040000U  //!< Emit VZEROUPPER instruction in epilog (X86).
  };

  //! FuncFrame is bound to the same limits as \ref CallConv.
  ASMJIT_ENUM(Limits) {
    kNumRegKinds = CallConv::kNumRegKinds
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() noexcept {
    ::memset(this, 0, sizeof(*this));
    _stackArgsRegId = kInvalidReg;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get FuncFrame flags, see \ref Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }
  //! Check if a FuncFrame `flag` is set, see \ref Flags.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }
  //! Add `flags` to FuncFrame, see \ref Flags.
  ASMJIT_INLINE void addFlags(uint32_t flags) noexcept { _flags |= flags; }
  //! Clear FuncFrame `flags`, see \ref Flags.
  ASMJIT_INLINE void clearFlags(uint32_t flags) noexcept { _flags &= ~flags; }

  //! Get if the function is naked - it doesn't preserve frame pointer (EBP|ESP).
  ASMJIT_INLINE bool hasPreservedFP() const noexcept { return (_flags & kFlagPreserveFP) != 0; }
  //! Get if the function is naked - it doesn't preserve frame pointer (EBP|ESP).
  ASMJIT_INLINE bool isNaked() const noexcept { return (_flags & kFlagPreserveFP) == 0; }
  //! Get if the function prolog and epilog should be compacted (as small as possible).
  ASMJIT_INLINE bool isCompact() const noexcept { return (_flags & kFlagCompact) != 0; }

  //! Get which registers (by `kind`) are saved/restored in prolog/epilog, respectively.
  ASMJIT_INLINE uint32_t getDirtyRegs(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    return _dirtyRegs[kind];
  }

  //! Set which registers (by `kind`) are saved/restored in prolog/epilog, respectively.
  ASMJIT_INLINE void setDirtyRegs(uint32_t kind, uint32_t regs) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    _dirtyRegs[kind] = regs;
  }

  //! Add registers (by `kind`) to saved/restored registers.
  ASMJIT_INLINE void addDirtyRegs(uint32_t kind, uint32_t regs) noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    _dirtyRegs[kind] |= regs;
  }

  //! Get stack-frame size used by the function.
  ASMJIT_INLINE uint32_t getStackFrameSize() const noexcept { return _stackFrameSize; }
  //! Get call-frame size used by the function.
  ASMJIT_INLINE uint32_t getCallFrameSize() const noexcept { return _callFrameSize; }

  //! Get minimum stack-frame alignment required by the function.
  ASMJIT_INLINE uint32_t getStackFrameAlignment() const noexcept { return _stackFrameAlignment; }
  //! Get minimum call-frame alignment required by the function.
  ASMJIT_INLINE uint32_t getCallFrameAlignment() const noexcept { return _callFrameAlignment; }
  //! Get a natural alignment as defined by CallConv or ABI.
  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _naturalStackAlignment; }

  ASMJIT_INLINE void setStackFrameSize(uint32_t size) noexcept { _stackFrameSize = size; }
  ASMJIT_INLINE void setCallFrameSize(uint32_t size) noexcept { _callFrameSize = size; }

  ASMJIT_INLINE void setStackFrameAlignment(uint32_t value) noexcept {
    ASMJIT_ASSERT(value < 256);
    _stackFrameAlignment = static_cast<uint8_t>(value);
  }

  ASMJIT_INLINE void setCallFrameAlignment(uint32_t value) noexcept {
    ASMJIT_ASSERT(value < 256);
    _callFrameAlignment = static_cast<uint8_t>(value);
  }

  ASMJIT_INLINE void mergeStackFrameSize(uint32_t size) noexcept { _stackFrameSize = Utils::iMax<uint32_t>(_stackFrameSize, size); }
  ASMJIT_INLINE void mergeCallFrameSize(uint32_t size) noexcept { _callFrameSize = Utils::iMax<uint32_t>(_callFrameSize, size); }

  ASMJIT_INLINE void mergeStackFrameAlignment(uint32_t value) noexcept {
    ASMJIT_ASSERT(value < 256);
    _stackFrameAlignment = static_cast<uint8_t>(Utils::iMax<uint32_t>(_stackFrameAlignment, value));
  }

  ASMJIT_INLINE void mergeCallFrameAlignment(uint32_t value) noexcept {
    ASMJIT_ASSERT(value < 256);
    _callFrameAlignment = static_cast<uint8_t>(Utils::iMax<uint32_t>(_callFrameAlignment, value));
  }

  ASMJIT_INLINE void setNaturalStackAlignment(uint32_t value) noexcept {
    ASMJIT_ASSERT(value < 256);
    _naturalStackAlignment = static_cast<uint8_t>(value);
  }

  ASMJIT_INLINE bool hasStackArgsRegId() const noexcept { return _stackArgsRegId != kInvalidReg; }
  ASMJIT_INLINE uint32_t getStackArgsRegId() const noexcept { return _stackArgsRegId; }
  ASMJIT_INLINE void setStackArgsRegId(uint32_t regId) { _stackArgsRegId = regId; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _flags;                       //!< FuncFrame flags.
  uint32_t _dirtyRegs[kNumRegKinds];     //!< Registers used by the function.

  uint8_t _stackFrameAlignment;          //!< Minimum alignment of stack-frame.
  uint8_t _callFrameAlignment;           //!< Minimum alignment of call-frame.
  uint8_t _naturalStackAlignment;        //!< Natural stack alignment as defined by OS/ABI.
  uint8_t _stackArgsRegId;               //!< Register that holds base-address to arguments passed by stack.

  uint32_t _stackFrameSize;              //!< Size of a stack-frame used by the function.
  uint32_t _callFrameSize;               //!< Size of a call-frame (not part of _stackFrameSize).
};

// ============================================================================
// [asmjit::FuncArgIndex]
// ============================================================================

//! Function argument index (lo/hi).
ASMJIT_ENUM(FuncArgIndex) {
  //! Maximum number of function arguments supported by AsmJit.
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
// [asmjit::FuncInOut]
// ============================================================================

//! Function in/out - argument or return value translated from `FuncSignature`.
struct FuncInOut {
  ASMJIT_ENUM(Components) {
    kTypeIdShift      = 24,
    kTypeIdMask       = 0xFF000000U,

    kRegTypeShift     = 8,
    kRegTypeMask      = 0x0000FF00U,

    kRegIdShift       = 0,
    kRegIdMask        = 0x000000FFU,

    kStackOffsetShift = 0,
    kStackOffsetMask  = 0x0000FFFFU,

    kIsByReg          = 0x00010000U,
    kIsByStack        = 0x00020000U,
    kIsIndirectArg    = 0x00040000U
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Get if this FuncInOut is initialized (i.e. contains a valid data).
  ASMJIT_INLINE bool isInitialized() const noexcept { return _value != 0; }

  //! Initialize this in/out by a given `typeId`.
  ASMJIT_INLINE void initTypeId(uint32_t typeId) noexcept {
    _value = typeId << kTypeIdShift;
  }

  //! Initialize this in/out by a given `typeId`, `regType`, and `regId`.
  ASMJIT_INLINE void initReg(uint32_t typeId, uint32_t regType, uint32_t regId) noexcept {
    _value = (typeId << kTypeIdShift) | (regType << kRegTypeShift) | (regId << kRegIdShift) | kIsByReg;
  }

  //! Initialize this in/out by a given `typeId` and `offset`.
  ASMJIT_INLINE void initStack(uint32_t typeId, uint32_t stackOffset) noexcept {
    _value = (typeId << kTypeIdShift) | (stackOffset << kStackOffsetShift) | kIsByStack;
  }

  //! Reset FuncInOut to uninitialized state.
  ASMJIT_INLINE void reset() noexcept { _value = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void assignToReg(uint32_t regType, uint32_t regId) noexcept {
    ASMJIT_ASSERT(!isAssigned());
    _value |= (regType << kRegTypeShift) | (regId << kRegIdShift) | kIsByReg;
  }

  ASMJIT_INLINE void assignToStack(int32_t offset) noexcept {
    ASMJIT_ASSERT(!isAssigned());
    _value |= (offset << kStackOffsetShift) | kIsByStack;
  }

  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool byReg() const noexcept { return (_value & kIsByReg) != 0; }
  //! Get if this argument is passed by stack.
  ASMJIT_INLINE bool byStack() const noexcept { return (_value & kIsByStack) != 0; }
  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool isAssigned() const noexcept { return (_value & (kIsByReg | kIsByStack)) != 0; }
  //! Get if this argument is passed through a pointer (used by Win64 to pass XMM|YMM|ZMM).
  ASMJIT_INLINE bool isIndirectArg() const noexcept { return (_value & kIsIndirectArg) != 0; }

  //! Get virtual type of this argument or return value.
  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _value >> kTypeIdShift; }
  //! Get a physical id of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegId() const noexcept { return (_value & kRegIdMask) >> kRegIdShift; }
  //! Get a register type of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegType() const noexcept { return (_value & kRegTypeMask) >> kRegTypeShift; }
  //! Get a stack offset of this argument (always positive).
  ASMJIT_INLINE int32_t getStackOffset() const noexcept { return (_value & kStackOffsetMask) >> kStackOffsetShift; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _value;
};

// ============================================================================
// [asmjit::FuncSignature]
// ============================================================================

//! Function signature.
//!
//! Contains information about function return type, count of arguments and
//! their TypeIds. Function signature is a low level structure which doesn't
//! contain platform specific or calling convention specific information.
struct FuncSignature {
  enum {
    kNoVarArgs = 0xFF                    //! Doesn't have variable number of arguments (`...`).
  };

  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  //! Setup the prototype.
  ASMJIT_INLINE void setup(
    uint32_t ccId,
    uint32_t ret,
    const uint8_t* args, uint32_t argCount) noexcept {

    ASMJIT_ASSERT(ccId <= 0xFF);
    ASMJIT_ASSERT(argCount <= 0xFF);

    _callConv = static_cast<uint8_t>(ccId);
    _varArgs = kNoVarArgs;
    _argCount = static_cast<uint8_t>(argCount);
    _ret = ret;
    _args = args;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention.
  ASMJIT_INLINE uint32_t getCallConv() const noexcept { return _callConv; }
  //! Get if the function has variable number of arguments (...).
  ASMJIT_INLINE bool hasVarArgs() const noexcept { _varArgs != kNoVarArgs; }
  //! Get the variable arguments (...) index, `kNoVarArgs` if none.
  ASMJIT_INLINE uint32_t getVarArgs() const noexcept { return _varArgs; }

  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }

  ASMJIT_INLINE bool hasRet() const noexcept { return _ret != TypeId::kVoid; }
  //! Get the return value type.
  ASMJIT_INLINE uint32_t getRet() const noexcept { return _ret; }

  //! Get the type of the argument at index `i`.
  ASMJIT_INLINE uint32_t getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _argCount);
    return _args[i];
  }
  //! Get the array of function arguments' types.
  ASMJIT_INLINE const uint8_t* getArgs() const noexcept { return _args; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _callConv;                     //!< Calling convention id.
  uint8_t _varArgs;                      //!< First var args
  uint8_t _argCount;
  uint8_t _ret;
  const uint8_t* _args;
};

// ============================================================================
// [asmjit::FuncSignatureT]
// ============================================================================

//! \internal
#define T(TYPE) TypeIdOf<TYPE>::kTypeId

//! Function signature template (no arguments).
template<typename RET>
struct FuncSignature0 : public FuncSignature {
  ASMJIT_INLINE FuncSignature0(uint32_t callConv = CallConv::kIdHost) noexcept {
    setup(callConv, T(RET), nullptr, 0);
  }
};

//! Function signature template (1 argument).
template<typename RET, typename A0>
struct FuncSignature1 : public FuncSignature {
  ASMJIT_INLINE FuncSignature1(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (2 arguments).
template<typename RET, typename A0, typename A1>
struct FuncSignature2 : public FuncSignature {
  ASMJIT_INLINE FuncSignature2(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (3 arguments).
template<typename RET, typename A0, typename A1, typename A2>
struct FuncSignature3 : public FuncSignature {
  ASMJIT_INLINE FuncSignature3(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (4 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3>
struct FuncSignature4 : public FuncSignature {
  ASMJIT_INLINE FuncSignature4(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (5 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4>
struct FuncSignature5 : public FuncSignature {
  ASMJIT_INLINE FuncSignature5(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (6 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct FuncSignature6 : public FuncSignature {
  ASMJIT_INLINE FuncSignature6(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (7 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct FuncSignature7 : public FuncSignature {
  ASMJIT_INLINE FuncSignature7(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (8 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct FuncSignature8 : public FuncSignature {
  ASMJIT_INLINE FuncSignature8(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (9 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct FuncSignature9 : public FuncSignature {
  ASMJIT_INLINE FuncSignature9(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (10 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct FuncSignature10 : public FuncSignature {
  ASMJIT_INLINE FuncSignature10(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8), T(A9) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};
#undef T

#if ASMJIT_CC_HAS_VARIADIC_TEMPLATES
//! Function signature template.
template<typename RET, typename... ARGS>
struct FuncSignatureT : public FuncSignature {
  ASMJIT_INLINE FuncSignatureT(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { (TypeIdOf<ARGS>::kTypeId)... };
    setup(callConv, TypeIdOf<RET>::kTypeId, args, ASMJIT_ARRAY_SIZE(args));
  }
};
#endif // ASMJIT_CC_HAS_VARIADIC_TEMPLATES

// ============================================================================
// [asmjit::FuncSignatureX]
// ============================================================================

//! Dynamic function signature.
struct FuncSignatureX : public FuncSignature {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncSignatureX(uint32_t callConv = CallConv::kIdHost) noexcept {
    setup(callConv, TypeId::kVoid, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setCallConv(uint32_t callConv) noexcept {
    ASMJIT_ASSERT(callConv <= 0xFF);
    _callConv = static_cast<uint8_t>(callConv);
  }

  //! Set the return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) noexcept { _ret = retType; }
  //! Set the return type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setRetT() noexcept { setRet(TypeIdOf<T>::kTypeId); }

  //! Set the argument at index `i` to the `type`
  ASMJIT_INLINE void setArg(uint32_t i, uint32_t type) noexcept {
    ASMJIT_ASSERT(i < _argCount);
    _builderArgList[i] = type;
  }
  //! Set the argument at index `i` to the type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t i) noexcept { setArg(i, TypeIdOf<T>::kTypeId); }

  //! Append an argument of `type` to the function prototype.
  ASMJIT_INLINE void addArg(uint32_t type) noexcept {
    ASMJIT_ASSERT(_argCount < kFuncArgCount);
    _builderArgList[_argCount++] = static_cast<uint8_t>(type);
  }
  //! Append an argument of type based on `T` to the function prototype.
  template<typename T>
  ASMJIT_INLINE void addArgT() noexcept { addArg(TypeIdOf<T>::kTypeId); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _builderArgList[kFuncArgCount];
};

// ============================================================================
// [asmjit::FuncDecl]
// ============================================================================

//! Function declaration.
struct FuncDecl {
  //! FuncDecl is bound to the same limits as \ref CallConv.
  ASMJIT_ENUM(Limits) {
    kNumRegKinds = CallConv::kNumRegKinds
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize this `FuncDecl` to the given signature.
  ASMJIT_API Error init(const FuncSignature& sign);

  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention, see `CallConv`.
  ASMJIT_INLINE const CallConv& getCallConv() const noexcept { return _callConv; }

  //! Get CallConv flags, see \ref CallConv::Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _callConv.getFlags(); }
  //! Check if a CallConv `flag` is set, see \ref CallConv::Flags.
  ASMJIT_INLINE bool hasFlag(uint32_t ccFlag) const noexcept { return _callConv.hasFlag(ccFlag); }


  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }
  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const noexcept { return _retCount; }

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

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const noexcept { return _retCount != 0; }
  //! Get function return value.
  ASMJIT_INLINE FuncInOut& getRet(uint32_t index = kFuncRetLo) noexcept { return _rets[index]; }
  //! Get function return value.
  ASMJIT_INLINE const FuncInOut& getRet(uint32_t index = kFuncRetLo) const noexcept { return _rets[index]; }

  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const noexcept { return _argStackSize; }

  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _callConv.getRedZoneSize(); }
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _callConv.getSpillZoneSize(); }

  ASMJIT_INLINE uint32_t getPassedRegs(uint32_t kind) const noexcept { return _callConv.getPassedRegs(kind); }
  ASMJIT_INLINE uint32_t getPreservedRegs(uint32_t kind) const noexcept { return _callConv.getPreservedRegs(kind); }

  ASMJIT_INLINE uint32_t getUsedRegs(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < CallConv::kNumRegKinds);
    return _usedRegs[kind];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CallConv _callConv;                    //!< Calling convention.

  uint8_t _argCount;                     //!< Number of function arguments.
  uint8_t _retCount;                     //!< Number of function return values.

  uint32_t _usedRegs[kNumRegKinds];      //!< Registers that contains arguments (signature dependent).
  uint32_t _argStackSize;                //!< Size of arguments passed by stack.

  FuncInOut _args[kFuncArgCountLoHi];    //!< Function arguments.
  FuncInOut _rets[2];                    //!< Function return values.
};

// ============================================================================
// [asmjit::FuncLayout]
// ============================================================================

//! Function layout.
//!
//! Function layout is used directly by prolog and epilog insertion helpers. It
//! contains only information necessary to insert proper prolog and epilog, and
//! should be always calculated from \ref FuncDecl and \ref FuncFrame, where
//! FuncDecl defines function's calling convention and signature, and FuncFrame
//! specifies how much stack is used, and which registers are dirty.
struct FuncLayout {
  //! FuncLayout is bound to the same limits as \ref CallConv.
  ASMJIT_ENUM(Limits) {
    kNumRegKinds = CallConv::kNumRegKinds
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_API Error init(const FuncDecl& decl, const FuncFrame& frame) noexcept;
  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool hasPreservedFP() const noexcept { return static_cast<bool>(_preservedFP); }
  ASMJIT_INLINE bool hasDsaSlotUsed() const noexcept { return static_cast<bool>(_dsaSlotUsed); }
  ASMJIT_INLINE bool hasAlignedVecSR() const noexcept { return static_cast<bool>(_alignedVecSR); }
  ASMJIT_INLINE bool hasDynamicAlignment() const noexcept { return static_cast<bool>(_dynamicAlignment); }
  ASMJIT_INLINE bool hasX86MmxCleanup() const noexcept { return static_cast<bool>(_x86MmxCleanup); }
  ASMJIT_INLINE bool hasX86AvxCleanup() const noexcept { return static_cast<bool>(_x86AvxCleanup); }

  ASMJIT_INLINE uint32_t getSavedRegs(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < kNumRegKinds);
    return _savedRegs[kind];
  }

  //! Get stack size.
  ASMJIT_INLINE uint32_t getStackSize() const noexcept { return _stackSize; }
  //! Get stack alignment.
  ASMJIT_INLINE uint32_t getStackAlignment() const noexcept { return _stackAlignment; }
  //! Get the offset needed to access the function's stack (it skips call-stack).
  ASMJIT_INLINE uint32_t getStackBaseOffset() const noexcept { return _stackBaseOffset; }

  //! Get stack size required to save GP registers.
  ASMJIT_INLINE uint32_t getGpStackSize() const noexcept { return _gpStackSize; }
  //! Get stack size required to save VEC registers.
  ASMJIT_INLINE uint32_t getVecStackSize() const noexcept { return _vecStackSize; }

  ASMJIT_INLINE uint32_t getGpStackOffset() const noexcept { return _gpStackOffset; }
  ASMJIT_INLINE uint32_t getVecStackOffset() const noexcept { return _vecStackOffset; }

  ASMJIT_INLINE uint32_t getStackArgsRegId() const noexcept { return _stackArgsRegId; }
  ASMJIT_INLINE uint32_t getStackArgsOffset() const noexcept { return _stackArgsOffset; }

  ASMJIT_INLINE bool hasStackAdjustment() const noexcept { return _stackAdjustment != 0; }
  ASMJIT_INLINE uint32_t getStackAdjustment() const noexcept { return _stackAdjustment; }

  ASMJIT_INLINE bool hasCalleeStackCleanup() const noexcept { return _calleeStackCleanup != 0; }
  ASMJIT_INLINE uint32_t getCalleeStackCleanup() const noexcept { return _calleeStackCleanup; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _stackAlignment;               //!< Final stack alignment of the functions.
  uint8_t _stackBaseRegId;               //!< GP register that holds address of base stack address (call-frame).
  uint8_t _stackArgsRegId;               //!< GP register that holds address the first argument passed by stack.

  uint32_t _savedRegs[kNumRegKinds];     //!< Registers that will be saved/restored in prolog/epilog

  uint32_t _preservedFP : 1;             //!< Function preserves frame-pointer.
  uint32_t _dsaSlotUsed : 1;             //!< True if `_dsaSlot` contains a valid mem offset.
  uint32_t _alignedVecSR : 1;            //!< Use instructions that perform aligned ops to save/restore XMM regs.
  uint32_t _dynamicAlignment : 1;        //!< Function dynamically aligns stack.
  uint32_t _x86MmxCleanup : 1;           //!< Emit 'emms' in epilog (X86 specific).
  uint32_t _x86AvxCleanup : 1;           //!< Emit 'vzeroupper' in epilog (X86 specific).

  uint32_t _stackSize;                   //!< Stack size (sum of function's stack and call stack).
  uint32_t _stackBaseOffset;             //!< Stack offset (non-zero if kFlagHasCalls is set).
  uint32_t _stackAdjustment;             //!< Stack adjustment in prolog/epilog.
  uint32_t _stackArgsOffset;             //!< Offset to the first argument passed by stack of _stackArgsRegId.

  uint32_t _dsaSlot;                     //!< Memory slot where the prolog inserter stores previous (unaligned) ESP.
  uint16_t _calleeStackCleanup;          //!< How many bytes the callee should add to the stack (X86 STDCALL).
  uint16_t _gpStackSize;                 //!< Stack size required to save GP regs.
  uint16_t _vecStackSize;                //!< Stack size required to save VEC regs.
  uint32_t _gpStackOffset;               //!< Offset where saved GP regs are stored.
  uint32_t _vecStackOffset;              //!< Offset where saved GP regs are stored.
};

// ============================================================================
// [asmjit::FuncUtils]
// ============================================================================

struct FuncUtils {
  static ASMJIT_API Error insertProlog(CodeEmitter* emitter, const FuncLayout& layout);
  static ASMJIT_API Error insertEpilog(CodeEmitter* emitter, const FuncLayout& layout);
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_FUNC_H
