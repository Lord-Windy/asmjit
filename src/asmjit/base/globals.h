// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_GLOBALS_H
#define _ASMJIT_BASE_GLOBALS_H

// [Dependencies]
#include "../build.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::TypeDefs]
// ============================================================================

//! AsmJit error core (uint32_t).
typedef uint32_t Error;

// ============================================================================
// [asmjit::GlobalDefs]
// ============================================================================

//! Invalid index
//!
//! Invalid index is the last possible index that is never used in practice. In
//! AsmJit it is used exclusively with strings to indicate the the length of the
//! string is not known and has to be determined.
static const size_t kInvalidIndex = ~static_cast<size_t>(0);

//! Invalid base address.
static const uint64_t kNoBaseAddress = ~static_cast<uint64_t>(0);

//! Global constants.
ASMJIT_ENUM(GlobalDefs) {
  //! Invalid instruction.
  kInvalidInst = 0,
  //! Invalid register id.
  kInvalidReg = 0xFF,
  //! Invalid value or id.
  kInvalidValue = 0xFFFFFFFF,

  //! Host memory allocator overhead.
  //!
  //! The overhead is decremented from all zone allocators so the operating
  //! system doesn't have to allocate one extra virtual page to keep tract of
  //! the requested memory block.
  //!
  //! The number is actually a guess.
  kMemAllocOverhead = sizeof(intptr_t) * 4,

  //! Memory grow threshold.
  kMemAllocGrowMax = 8192 * 1024
};

// ============================================================================
// [asmjit::ptr_cast]
// ============================================================================

//! Cast designed to cast between function and void* pointers.
template<typename Dst, typename Src>
ASMJIT_INLINE Dst ptr_cast(Src p) noexcept { return (Dst)p; }

// ============================================================================
// [asmjit::Arch]
// ============================================================================

class Arch {
public:
  //! Type of the architecture.
  ASMJIT_ENUM(Type) {
    kTypeNone  = 0,                      //!< No/Unknown architecture.
    kTypeX86   = 1,                      //!< X86 architecture (32-bit).
    kTypeX64   = 2,                      //!< X64 architecture (64-bit) (AMD64).
    kTypeX32   = 3,                      //!< X32 architecture (DEAD-END).
    kTypeArm32 = 4,                      //!< ARM32 architecture (32-bit).
    kTypeArm64 = 5,                      //!< ARM64 architecture (64-bit).

    //! Architecture detected at compile-time (architecture of the host).
    kTypeHost  = ASMJIT_ARCH_X86   ? kTypeX86   :
                 ASMJIT_ARCH_X64   ? kTypeX64   :
                 ASMJIT_ARCH_ARM32 ? kTypeArm32 :
                 ASMJIT_ARCH_ARM64 ? kTypeArm64 : kTypeNone
  };

  ASMJIT_ENUM(SubType) {
    kSubTypeNone        = 0              //!< Default mode (or no specific mode).
  };

  ASMJIT_ENUM(X86SubType) {
    kX86SubTypeLegacy   = 0,             //!< Legacy (the most compatible) mode.
    kX86SubTypeAVX      = 1,             //!< AVX mode.
    kX86SubTypeAVX512F  = 2              //!< AVX512F mode.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Arch() noexcept : _signature(0) {}
  ASMJIT_INLINE Arch(const Arch& other) noexcept : _signature(other._signature) {}

  explicit ASMJIT_INLINE Arch(uint32_t type, uint32_t subType = kSubTypeNone) noexcept {
    init(type, subType);
  }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isInitialized() const noexcept { return _type != kTypeNone; }

  ASMJIT_API void init(uint32_t type, uint32_t subType = kSubTypeNone) noexcept;
  ASMJIT_INLINE void reset() noexcept { _signature = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get type of the architecture, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _type; }

  //! Get sub-type of the architecture, see \ref SubType.
  //!
  //! This field has multiple meanings across various architectures.
  //!
  //! X86 & X64
  //! ---------
  //!
  //! Architecture mode means the highest instruction set the code is generated
  //! for (basically the highest optimization level that can be used).
  //!
  //! ARM32
  //! -----
  //!
  //! Architecture mode means the instruction encoding to be used when generating
  //! machine code, thus mode can be used to force generation of THUMB and THUMB2
  //! encoding of regular ARM encoding.
  //!
  //! ARM64
  //! -----
  //!
  //! No meaning yet.
  ASMJIT_INLINE uint32_t getSubType() const noexcept { return _subType; }

  //! Get if the architecture is 64-bit.
  ASMJIT_INLINE bool is64Bit() const noexcept { return _gpSize >= 8; }
  //! Get if the architecture is X86, X64, or X32.
  ASMJIT_INLINE bool isX86Family() const noexcept { return _type >= kTypeX86 && _type <= kTypeX32; }
  //! Get if the architecture is ARM32 or ARM64.
  ASMJIT_INLINE bool isArmFamily() const noexcept { return _type >= kTypeArm32 && _type <= kTypeArm64; }

  //! Get a size of a general-purpose register.
  ASMJIT_INLINE uint32_t getGpSize() const noexcept { return _gpSize; }
  //! Get number of general-purpose registers.
  ASMJIT_INLINE uint32_t getGpCount() const noexcept { return _gpCount; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE const Arch& operator=(const Arch& other) noexcept { _signature = other._signature; return *this; }
  ASMJIT_INLINE bool operator==(const Arch& other) const noexcept { return _signature == other._signature; }
  ASMJIT_INLINE bool operator!=(const Arch& other) const noexcept { return _signature != other._signature; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _type;                     //!< Architecture type.
      uint8_t _subType;                  //!< Architecture sub-type.
      uint8_t _gpSize;                   //!< Default size of a general purpose register.
      uint8_t _gpCount;                  //!< Count of all general purpose registers.
    };
    uint32_t _signature;                 //!< Architecture signature (32-bit int).
  };
};

// ============================================================================
// [asmjit::ErrorCode]
// ============================================================================

//! AsmJit error codes.
ASMJIT_ENUM(ErrorCode) {
  //! No error (success).
  //!
  //! This is default state and state you want.
  kErrorOk = 0,

  //! Heap memory allocation failed.
  kErrorNoHeapMemory,

  //! Virtual memory allocation failed.
  kErrorNoVirtualMemory,

  //! Invalid argument.
  kErrorInvalidArgument,

  //! Invalid state.
  //!
  //! If this error is returned it means that either you are doing something
  //! wrong or AsmJit caught itself by doing something wrong. This error should
  //! not be underestimated.
  kErrorInvalidState,

  //! Incompatible architecture.
  kErrorInvalidArch,

  //! The object is not initialized.
  kErrorNotInitialized,

  //! The object is already initialized.
  kErrorAlreadyInitialized,

  //! CodeHolder can't have attached more than one \ref Assembler at a time.
  kErrorSlotOccupied,

  //! No code generated.
  //!
  //! Returned by runtime if the \ref CodeHolder contains no code.
  kErrorNoCodeGenerated,

  //! Code generated is larger than allowed.
  kErrorCodeTooLarge,

  //! Attempt to use uninitialized label.
  kErrorInvalidLabel,

  //! Label index overflow - a single `Assembler` instance can hold more than
  //! 2 billion labels (2147483391 to be exact). If there is an attempt to
  //! create more labels then this error is reported.
  kErrorLabelIndexOverflow,

  //! Label is already bound.
  kErrorLabelAlreadyBound,

  //! Label is already defined (named labels).
  kErrorLabelAlreadyDefined,

  //! Label name too long.
  kErrorLabelNameTooLong,

  //! Label must always be local if it's anonymous (without a name).
  kErrorInvalidLabelName,

  //! Parent id passed to `CodeHolder::newNamedLabelId()` was invalid.
  kErrorInvalidParentLabel,

  //! Parent id specified for a non-local (global) label.
  kErrorNonLocalLabelCantHaveParent,

  //! Invalid instruction (an instruction ID is out of bounds or instruction
  //! name is invalid).
  kErrorInvalidInstruction,

  //! Illegal instruction.
  //!
  //! This status code can also be returned in X64 mode if AH, BH, CH or DH
  //! registers have been used together with a REX prefix. The instruction
  //! is not encodable in such case.
  //!
  //! Example of raising `kErrorIllegalInstruction` error.
  //!
  //! ~~~
  //! // Invalid address size.
  //! a.mov(dword_ptr(eax), al);
  //!
  //! // Undecodable instruction - AH used with R10, however R10 can only be
  //! // encoded by using REX prefix, which conflicts with AH.
  //! a.mov(byte_ptr(r10), ah);
  //! ~~~
  //!
  //! NOTE: In debug mode assertion is raised instead of returning an error.
  kErrorIllegalInstruction,
  //! Illegal register type.
  kErrorIllegalRegType,

  //! Invalid register's physical id.
  kErrorInvalidPhysId,
  //! Invalid register's virtual id.
  kErrorInvalidVirtId,

  //! Invalid TypeId.
  kErrorInvalidTypeId,

  //! Illegal use of a low 8-bit GPB register.
  kErrorIllegalUseOfGpbHi,
  //! Illegal use of a 64-bit GPQ register in 32-bit mode.
  kErrorIllegalUseOfGpq,
  //! Illegal use of an 80-bit float (TypeInfo::kF80).
  kErrorIllegalUseOfF80,

  //! Illegal (unencodable) addressing used.
  kErrorIllegalAddressing,

  //! Illegal (unencodable) displacement used.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Short form of jump instruction has been used, but the displacement is out
  //! of bounds.
  kErrorIllegalDisplacement,

  //! A variable has been assigned more than once to a function argument (CodeCompiler).
  kErrorOverlappedArgs,

  //! Count of AsmJit error codes.
  kErrorCount
};

// ============================================================================
// [asmjit::Init / NoInit]
// ============================================================================

#if !defined(ASMJIT_DOCGEN)
struct _Init {};
static const _Init Init = {};

struct _NoInit {};
static const _NoInit NoInit = {};
#endif // !ASMJIT_DOCGEN

// ============================================================================
// [asmjit::DebugUtils]
// ============================================================================

namespace DebugUtils {

//! Returns the error `err` passed.
//!
//! Provided for debugging purposes. Putting a breakpoint inside `errored` can
//! help with tracing the origin of any error reported / returned by AsmJit.
static ASMJIT_INLINE Error errored(Error err) noexcept { return err; }

//! Get a printable version of `asmjit::Error` code.
ASMJIT_API const char* errorAsString(Error err) noexcept;

//! Called in debug build to output a debugging message caused by assertion
//! failure or tracing (if enabled by ASMJIT_TRACE).
ASMJIT_API void debugOutput(const char* str) noexcept;

//! Called in debug build on assertion failure.
//!
//! \param file Source file name where it happened.
//! \param line Line in the source file.
//! \param msg Message to display.
//!
//! If you have problems with assertions put a breakpoint at assertionFailed()
//! function (asmjit/base/globals.cpp) and check the call stack to locate the
//! failing code.
ASMJIT_API void ASMJIT_NORETURN assertionFailed(const char* file, int line, const char* msg) noexcept;

#if defined(ASMJIT_DEBUG)
# define ASMJIT_ASSERT(exp)                                          \
  do {                                                               \
    if (ASMJIT_LIKELY(exp))                                          \
      break;                                                         \
    ::asmjit::DebugUtils::assertionFailed(__FILE__, __LINE__, #exp); \
  } while (0)
# define ASMJIT_NOT_REACHED()                                        \
  do {                                                               \
    ::asmjit::DebugUtils::assertionFailed(__FILE__, __LINE__,        \
      "ASMJIT_NOT_REACHED has been reached");                        \
    ASMJIT_ASSUME(0);                                                \
  } while (0)
#else
# define ASMJIT_ASSERT(exp) ASMJIT_NOP
# define ASMJIT_NOT_REACHED() ASMJIT_ASSUME(0)
#endif // DEBUG

//! \internal
//!
//! Used by AsmJit to propagate a possible `Error` produced by `...` to the caller.
#define ASMJIT_PROPAGATE(...)                                \
  do {                                                       \
    ::asmjit::Error _err = __VA_ARGS__;                      \
    if (ASMJIT_UNLIKELY(_err)) return _err;                  \
  } while (0)

} // DebugUtils namespace

//! \}

} // asmjit namespace

//! \}

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_GLOBALS_H
