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

// TODO: Move somewhere, don't have to be part of a public API.
#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
typedef unsigned char FastUInt8;
#else
typedef unsigned int FastUInt8;
#endif

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
  //!
  //! After the grow threshold is reached the capacity won't be doubled
  //! anymore.
  kMemAllocGrowMax = 8192 * 1024
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

  //! Unknown instruction (an instruction ID is out of bounds or instruction
  //! name is invalid).
  kErrorUnknownInstruction,

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

  //! Illegal use of a low 8-bit GPB register.
  kErrorIllegalUseOfGpbHi,
  //! Illegal use of a 64-bit GPQ register in 32-bit mode.
  kErrorIllegalUseOfGpq,

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

} // DebugUtils namespace

//! \}

} // asmjit namespace

// ============================================================================
// [asmjit_cast<>]
// ============================================================================

//! \addtogroup asmjit_base
//! \{

//! Cast used to cast pointer to function. It's like reinterpret_cast<>,
//! but uses internally C style cast to work with MinGW.
//!
//! If you are using single compiler and `reinterpret_cast<>` works for you,
//! there is no reason to use `asmjit_cast<>`. If you are writing
//! cross-platform software with various compiler support, consider using
//! `asmjit_cast<>` instead of `reinterpret_cast<>`.
template<typename T, typename Z>
static ASMJIT_INLINE T asmjit_cast(Z* p) noexcept { return (T)p; }

// ============================================================================
// [ASMJIT_ASSERT]
// ============================================================================

#if defined(ASMJIT_DEBUG)
# define ASMJIT_ASSERT(exp)                                  \
  do {                                                       \
    if (ASMJIT_LIKELY(exp))                                  \
      break;                                                 \
    ::asmjit::DebugUtils::assertionFailed(                   \
      __FILE__ + ::asmjit::DebugUtils::kSourceRelPathOffset, \
      __LINE__,                                              \
      #exp);                                                 \
  } while (0)
# define ASMJIT_NOT_REACHED()                                \
  do {                                                       \
    ::asmjit::DebugUtils::assertionFailed(                   \
      __FILE__ + ::asmjit::DebugUtils::kSourceRelPathOffset, \
      __LINE__,                                              \
      "ASMJIT_NOT_REACHED has been reached");                \
    ASMJIT_ASSUME(0);                                        \
  } while (0)
#else
# define ASMJIT_ASSERT(exp) ASMJIT_NOP
# define ASMJIT_NOT_REACHED() ASMJIT_ASSUME(0)
#endif // DEBUG

// ============================================================================
// [ASMJIT_PROPAGATE]
// ============================================================================

//! \internal
//!
//! Used by AsmJit to propagate a possible `Error` produced by `...` to the caller.
#define ASMJIT_PROPAGATE(...)                                \
  do {                                                       \
    ::asmjit::Error _err = __VA_ARGS__;                      \
    if (ASMJIT_UNLIKELY(_err != ::asmjit::kErrorOk))         \
      return _err;                                           \
  } while (0)

//! \}

//! \}

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_GLOBALS_H
