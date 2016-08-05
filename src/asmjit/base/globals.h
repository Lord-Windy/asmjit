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
  //!
  //! After the grow threshold is reached the capacity won't be doubled
  //! anymore.
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

  ASMJIT_ENUM(Mode) {
    kModeNone = 0                        //!< Default mode (or no specific mode).
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Arch() noexcept : _signature(0) {}
  ASMJIT_INLINE Arch(const Arch& other) noexcept : _signature(other._signature) {}
  explicit ASMJIT_INLINE Arch(uint32_t type, uint32_t mode = kModeNone) noexcept { init(type, mode); }

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isInitialized() const noexcept { return _type != kTypeNone; }

  ASMJIT_API void init(uint32_t type, uint32_t mode = kModeNone) noexcept;
  ASMJIT_INLINE void reset() noexcept { _signature = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get type of the architecture, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _type; }

  //! Get if the architecture is 64-bit.
  ASMJIT_INLINE bool is64Bit() const noexcept { return _gpSize >= 8; }
  //! Get if the architecture is X86, X64, or X32.
  ASMJIT_INLINE bool isX86Family() const noexcept { return _type >= kTypeX86 && _type <= kTypeX32; }
  //! Get if the architecture is ARM32 or ARM64.
  ASMJIT_INLINE bool isArmFamily() const noexcept { return _type >= kTypeArm32 && _type <= kTypeArm64; }

  //! Instruction-set mode.
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
  ASMJIT_INLINE uint32_t getMode() const noexcept { return _mode; }

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
      uint8_t _mode;                     //!< instruction-set mode or optimization level.
      uint8_t _gpSize;                   //!< Default size of a general purpose register.
      uint8_t _gpCount;                  //!< Count of all general purpose registers.
    };
    uint32_t _signature;                 //!< Architecture signature (32-bit int).
  };
};

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

//! Function calling convention.
//!
//! Calling convention is a scheme that defines how function parameters are
//! passed and how the return value handled. In assembler programming it's
//! always needed to comply with function calling conventions, because even
//! small inconsistency can cause undefined behavior or application's crash.
//!
//! AsmJit defines a variety of architecture and OS specific calling conventions
//! and also provides detection at compile time available as `kCallConvHost` to
//! make JIT code-generation easier.
ASMJIT_ENUM(CallConv) {
  //! Calling convention is invalid (can't be used).
  kCallConvNone = 0,

  // --------------------------------------------------------------------------
  // [X86]
  // --------------------------------------------------------------------------

  //! X86 `__cdecl` calling convention (used by C runtime and libraries).
  //!
  //! Compatible across MSVC and GCC.
  //!
  //! Arguments direction:
  //! - Right to left.
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86CDecl = 1,

  //! X86 `__stdcall` calling convention (used mostly by WinAPI).
  //!
  //! Compatible across MSVC and GCC.
  //!
  //! Arguments direction:
  //! - Right to left.
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86StdCall = 2,

  //! X86 `__thiscall` calling convention (MSVC/Intel specific).
  //!
  //! This is MSVC (and Intel) specific calling convention used when targeting
  //! Windows platform for C++ class methods. Implicit `this` pointer (defined
  //! as the first argument) is stored in `ecx` register instead of storing it
  //! on the stack.
  //!
  //! This calling convention is implicitly used by MSVC for class functions.
  //!
  //! C++ class functions that have variable number of arguments use `__cdecl`
  //! calling convention instead.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first argument passed in `ecx`).
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86MsThisCall = 3,

  //! X86 `__fastcall` convention (MSVC/Intel specific).
  //!
  //! The first two arguments (evaluated from the left to the right) are passed
  //! in `ecx` and `edx` registers, all others on the stack from the right to
  //! the left.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first two integers passed in `ecx` and `edx`).
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  //!
  //! NOTE: This calling convention differs from GCC's one.
  kCallConvX86MsFastCall = 4,

  //! X86 `__fastcall` convention (Borland specific).
  //!
  //! The first two arguments (evaluated from the left to the right) are passed
  //! in `ecx` and `edx` registers, all others on the stack from the left to
  //! the right.
  //!
  //! Arguments direction:
  //! - Left to right (except for the first two integers passed in `ecx` and `edx`).
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  //!
  //! NOTE: Arguments on the stack are in passed in left to right order, which
  //! is really Borland specific, all other `__fastcall` calling conventions
  //! use right to left order.
  kCallConvX86BorlandFastCall = 5,

  //! X86 `__fastcall` convention (GCC specific).
  //!
  //! The first two arguments (evaluated from the left to the right) are passed
  //! in `ecx` and `edx` registers, all others on the stack from the right to
  //! the left.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first two integers passed in `ecx` and `edx`).
  //!
  //! Stack is cleaned by:
  //! - Callee.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  //!
  //! NOTE: This calling convention should be compatible with `kCallConvX86MsFastCall`.
  kCallConvX86GccFastCall = 6,

  //! X86 `regparm(1)` convention (GCC specific).
  //!
  //! The first argument (evaluated from the left to the right) is passed in
  //! `eax` register, all others on the stack from the right to the left.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first integer passed in `eax`).
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86GccRegParm1 = 7,

  //! X86 `regparm(2)` convention (GCC specific).
  //!
  //! The first two arguments (evaluated from the left to the right) are passed
  //! in `ecx` and `edx` registers, all others on the stack from the right to
  //! the left.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first two integers passed in `ecx` and `edx`).
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86GccRegParm2 = 8,

  //! X86 `regparm(3)` convention (GCC specific).
  //!
  //! Three first parameters (evaluated from left-to-right) are in
  //! EAX:EDX:ECX registers, all others on the stack in right-to-left direction.
  //!
  //! Arguments direction:
  //! - Right to left (except for the first three integers passed in `ecx`,
  //!   `edx`, and `ecx`).
  //!
  //! Stack is cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `eax:edx` registers.
  //! - Floating point - `fp0` register.
  kCallConvX86GccRegParm3 = 9,

  // --------------------------------------------------------------------------
  // [X64]
  // --------------------------------------------------------------------------

  //! X64 calling convention used by Windows platform (WIN64-ABI).
  //!
  //! The first 4 arguments are passed in the following registers:
  //! - 1. 32/64-bit integer in `rcx` and floating point argument in `xmm0`
  //! - 2. 32/64-bit integer in `rdx` and floating point argument in `xmm1`
  //! - 3. 32/64-bit integer in `r8` and floating point argument in `xmm2`
  //! - 4. 32/64-bit integer in `r9` and floating point argument in `xmm3`
  //!
  //! If one or more argument from the first four doesn't match the list above
  //! it is simply skipped. WIN64-ABI is very specific about this.
  //!
  //! All other arguments are pushed on the stack from the right to the left.
  //! Stack has to be aligned by 16 bytes, always. There is also a 32-byte
  //! shadow space on the stack that can be used to save up to four 64-bit
  //! registers.
  //!
  //! Arguments direction:
  //! - Right to left (except for all parameters passed in registers).
  //!
  //! Stack cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `rax`.
  //! - Floating point - `xmm0`.
  //!
  //! Stack is always aligned to 16 bytes.
  //!
  //! More information about this calling convention can be found on MSDN
  //!   <http://msdn.microsoft.com/en-us/library/9b372w95.aspx>.
  kCallConvX64Win = 10,

  //! X64 calling convention used by Unix platforms (AMD64-ABI).
  //!
  //! First six 32 or 64-bit integer arguments are passed in `rdi`, `rsi`,
  //! `rdx`, `rcx`, `r8`, and `r9` registers. First eight floating point or xmm
  //! arguments are passed in `xmm0`, `xmm1`, `xmm2`, `xmm3`, `xmm4`, `xmm5`,
  //! `xmm6`, and `xmm7` registers.
  //!
  //! There is also a red zene below the stack pointer that can be used by the
  //! function. The red zone is typically from [rsp-128] to [rsp-8], however,
  //! red zone can also be disabled.
  //!
  //! Arguments direction:
  //! - Right to left (except for all arguments passed in registers).
  //!
  //! Stack cleaned by:
  //! - Caller.
  //!
  //! Return value:
  //! - Integer types - `rax`.
  //! - Floating point - `xmm0`.
  //!
  //! Stack is always aligned to 16 bytes.
  kCallConvX64Unix = 11,

  // --------------------------------------------------------------------------
  // [ARM32]
  // --------------------------------------------------------------------------

  //! Legacy calling convention, floating point arguments are passed via GP registers.
  kCallConvArm32SoftFP = 16,
  //! Modern calling convention, uses VFP registers to pass floating point arguments.
  kCallConvArm32HardFP = 17,

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

  _kCallConvX86Start = 1,   //!< \internal
  _kCallConvX86End = 9,     //!< \internal

  _kCallConvX64Start = 10,  //!< \internal
  _kCallConvX64End = 11,    //!< \internal

  _kCallConvArmStart = 16,  //!< \internal
  _kCallConvArmEnd = 17,    //!< \internal

  // --------------------------------------------------------------------------
  // [Host]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_DOCGEN)
  //! Default calling convention based on the current C++ compiler's settings.
  //!
  //! NOTE: This should be always the same as `kCallConvHostCDecl`, but some
  //! compilers allow to override the default calling convention. Overriding
  //! is not detected at the moment.
  kCallConvHost         = DETECTED_AT_COMPILE_TIME,

  //! Default CDECL calling convention based on the current C++ compiler's settings.
  kCallConvHostCDecl    = DETECTED_AT_COMPILE_TIME,

  //! Default STDCALL calling convention based on the current C++ compiler's settings.
  //!
  //! NOTE: If not defined by the host then it's the same as `kCallConvHostCDecl`.
  kCallConvHostStdCall  = DETECTED_AT_COMPILE_TIME,

  //! Compatibility for `__fastcall` calling convention.
  //!
  //! NOTE: If not defined by the host then it's the same as `kCallConvHostCDecl`.
  kCallConvHostFastCall = DETECTED_AT_COMPILE_TIME
#elif ASMJIT_ARCH_X86
  kCallConvHost         = kCallConvX86CDecl,
  kCallConvHostCDecl    = kCallConvX86CDecl,
  kCallConvHostStdCall  = kCallConvX86StdCall,
  kCallConvHostFastCall =
    ASMJIT_CC_MSC       ? kCallConvX86MsFastCall      :
    ASMJIT_CC_GCC       ? kCallConvX86GccFastCall     :
    ASMJIT_CC_CLANG     ? kCallConvX86GccFastCall     :
    ASMJIT_CC_CODEGEAR  ? kCallConvX86BorlandFastCall : kCallConvNone
#elif ASMJIT_ARCH_X64
  kCallConvHost         = ASMJIT_OS_WINDOWS ? kCallConvX64Win : kCallConvX64Unix,
  kCallConvHostCDecl    = kCallConvHost, // Doesn't exist, redirected to host.
  kCallConvHostStdCall  = kCallConvHost, // Doesn't exist, redirected to host.
  kCallConvHostFastCall = kCallConvHost  // Doesn't exist, redirected to host.
#elif ASMJIT_ARCH_ARM32
# if defined(__SOFTFP__)
  kCallConvHost         = kCallConvArm32SoftFP,
# else
  kCallConvHost         = kCallConvArm32HardFP,
# endif
  // These don't exist on ARM.
  kCallConvHostCDecl    = kCallConvHost, // Doesn't exist, redirected to host.
  kCallConvHostStdCall  = kCallConvHost, // Doesn't exist, redirected to host.
  kCallConvHostFastCall = kCallConvHost  // Doesn't exist, redirected to host.
#else
# error "[asmjit] Couldn't determine the target's calling convention."
#endif
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
