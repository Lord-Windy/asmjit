// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ARCHINFO_H
#define _ASMJIT_BASE_ARCHINFO_H

// [Dependencies]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::ArchInfo]
// ============================================================================

//! Basic information about a target architecture.
struct ArchInfo {
  //! Architecture id.
  ASMJIT_ENUM(Id) {
    kIdNone  = 0,                        //!< No/Unknown architecture.
    kIdX86   = 1,                        //!< X86 architecture (32-bit).
    kIdX64   = 2,                        //!< X64 architecture (64-bit) (AMD64).
    kIdX32   = 3,                        //!< X32 architecture (DEAD-END).
    kIdArm32 = 4,                        //!< ARM32 architecture (32-bit).
    kIdArm64 = 5,                        //!< ARM64 architecture (64-bit).

    //! Architecture detected at compile-time (architecture of the host).
    kIdHost  = ASMJIT_ARCH_X86   ? kIdX86   :
               ASMJIT_ARCH_X64   ? kIdX64   :
               ASMJIT_ARCH_ARM32 ? kIdArm32 :
               ASMJIT_ARCH_ARM64 ? kIdArm64 : kIdNone
  };

  // --------------------------------------------------------------------------
  // [Reset ]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  ASMJIT_API void setup(uint32_t archId) noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getArchId() const noexcept { return _archId; }
  ASMJIT_INLINE uint32_t getGpSize() const noexcept { return _gpSize; }
  ASMJIT_INLINE uint32_t getGpTotal() const noexcept { return _gpTotal; }
  ASMJIT_INLINE uint32_t getGpUsable() const noexcept { return _gpUsable; }

  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _naturalStackAlignment; }
  ASMJIT_INLINE void setNaturalStackAlignment(uint8_t sa) noexcept { _naturalStackAlignment = static_cast<uint8_t>(sa); }

  ASMJIT_INLINE uint32_t getCdeclCallConv() const noexcept { return _cdeclCallConv; }
  ASMJIT_INLINE void setCdeclCallConv(uint32_t cc) noexcept { _cdeclCallConv = static_cast<uint8_t>(cc); }

  ASMJIT_INLINE uint32_t getStdCallConv() const noexcept { return _stdCallConv; }
  ASMJIT_INLINE void setStdCallConv(uint32_t cc) noexcept { _stdCallConv = static_cast<uint8_t>(cc); }

  ASMJIT_INLINE uint32_t getFastCallConv() const noexcept { return _fastCallConv; }
  ASMJIT_INLINE void setFastCallConv(uint32_t cc) noexcept { _fastCallConv = static_cast<uint8_t>(cc); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _archId;                       //!< Architecture id.
  uint8_t _gpSize;                       //!< Default size of a general purpose register.
  uint8_t _gpTotal;                      //!< Count of general purpose registers.
  uint8_t _gpUsable;                     //!< Count of usable general purpose registers.

  uint8_t _naturalStackAlignment;        //!< Natural stack alignment (ARCH+OS).
  uint8_t _cdeclCallConv;                //!< Default CDECL calling convention.
  uint8_t _stdCallConv;                  //!< Default STDCALL calling convention.
  uint8_t _fastCallConv;                 //!< Default FASTCALL calling convention.
};

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

//! Function calling convention.
//!
//! Calling convention is a scheme that defines how function arguments are
//! passed and how the return value handled. In assembler programming it's
//! always needed to comply with function calling conventions, because even
//! small inconsistency can cause undefined behavior or application's crash.
//!
//! Platform Independent Conventions
//! --------------------------------
//!
//!   - `kCallConvHost` - Should match the current C++ compiler native calling
//!     convention.
//!
//! X86/X64 Specific Conventions
//! ----------------------------
//!
//! List of calling conventions for 32-bit x86 mode:
//!   - `kCallConvX86CDecl` - Calling convention for C runtime.
//!   - `kCallConvX86StdCall` - Calling convention for WinAPI functions.
//!   - `kCallConvX86MsThisCall` - Calling convention for C++ members under
//!      Windows (produced by MSVC and all MSVC compatible compilers).
//!   - `kCallConvX86MsFastCall` - Fastest calling convention that can be used
//!      by MSVC compiler.
//!   - `kCallConvX86BorlandFastCall` - Borland fastcall convention.
//!   - `kCallConvX86GccFastCall` - GCC fastcall convention (2 register arguments).
//!   - `kCallConvX86GccRegParm1` - GCC regparm(1) convention.
//!   - `kCallConvX86GccRegParm2` - GCC regparm(2) convention.
//!   - `kCallConvX86GccRegParm3` - GCC regparm(3) convention.
//!
//! List of calling conventions for 64-bit x86 mode (x64):
//!   - `kCallConvX64Win` - Windows 64-bit calling convention (WIN64 ABI).
//!   - `kCallConvX64Unix` - Unix 64-bit calling convention (AMD64 ABI).
//!
//! ARM Specific Conventions
//! ------------------------
//!
//! List of ARM calling conventions:
//!   - `kCallConvArm32SoftFP` - Legacy calling convention, floating point
//!     arguments are passed via GP registers.
//!   - `kCallConvArm32HardFP` - Modern calling convention, uses VFP registers
//!     to pass floating point arguments.
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
  // [ARM]
  // --------------------------------------------------------------------------

  kCallConvArm32SoftFP = 16,
  kCallConvArm32HardFP = 17,

  // --------------------------------------------------------------------------
  // [Internal]
  // --------------------------------------------------------------------------

  //! \internal
  _kCallConvX86Start = 1,
  //! \internal
  _kCallConvX86End = 9,

  //! \internal
  _kCallConvX64Start = 10,
  //! \internal
  _kCallConvX64End = 11,

  //! \internal
  _kCallConvArmStart = 16,
  //! \internal
  _kCallConvArmEnd = 17,

  // --------------------------------------------------------------------------
  // [Host]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_DOCGEN)
  //! Default calling convention based on the current compiler's settings.
  //!
  //! NOTE: This should be always the same as `kCallConvHostCDecl`, but some
  //! compilers allow to override the default calling convention. Overriding
  //! is not detected at the moment.
  kCallConvHost         = DETECTED_AT_COMPILE_TIME,
  //! Default C calling convention based on the current compiler's settings.
  kCallConvHostCDecl    = DETECTED_AT_COMPILE_TIME,
  //! Compatibility for `__stdcall` calling convention.
  //!
  //! NOTE: This enumeration is always set to a value which is compatible with
  //! the current compiler's `__stdcall` calling convention. In 64-bit mode
  //! there is no such convention and the value is mapped to `kCallConvX64Win`
  //! or `kCallConvX64Unix`, depending on the host architecture.
  kCallConvHostStdCall  = DETECTED_AT_COMPILE_TIME,
  //! Compatibility for `__fastcall` calling convention.
  //!
  //! NOTE: This enumeration is always set to a value which is compatible with
  //! the current compiler's `__fastcall` calling convention. In 64-bit mode
  //! there is no such convention and the value is mapped to `kCallConvX64Win`
  //! or `kCallConvX64Unix`, depending on the host architecture.
  kCallConvHostFastCall = DETECTED_AT_COMPILE_TIME
#elif ASMJIT_ARCH_X86
  // X86 Host Support.
  kCallConvHost         = kCallConvX86CDecl,
  kCallConvHostCDecl    = kCallConvX86CDecl,
  kCallConvHostStdCall  = kCallConvX86StdCall,
  kCallConvHostFastCall =
    ASMJIT_CC_MSC       ? kCallConvX86MsFastCall      :
    ASMJIT_CC_GCC       ? kCallConvX86GccFastCall     :
    ASMJIT_CC_CLANG     ? kCallConvX86GccFastCall     :
    ASMJIT_CC_CODEGEAR  ? kCallConvX86BorlandFastCall : kCallConvNone
#elif ASMJIT_ARCH_X64
  // X64 Host Support.
  kCallConvHost         = ASMJIT_OS_WINDOWS ? kCallConvX64Win : kCallConvX64Unix,
  // These don't exist in 64-bit mode.
  kCallConvHostCDecl    = kCallConvHost,
  kCallConvHostStdCall  = kCallConvHost,
  kCallConvHostFastCall = kCallConvHost
#elif ASMJIT_ARCH_ARM32
# if defined(__SOFTFP__)
  kCallConvHost         = kCallConvArm32SoftFP,
# else
  kCallConvHost         = kCallConvArm32HardFP,
# endif
  // These don't exist on ARM.
  kCallConvHostCDecl    = kCallConvHost,
  kCallConvHostStdCall  = kCallConvHost,
  kCallConvHostFastCall = kCallConvHost
#else
# error "[asmjit] Couldn't determine the target's calling convention."
#endif
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_ARCHINFO_H
