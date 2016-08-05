// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_ASMJIT_H
#define _ASMJIT_ASMJIT_H

// ============================================================================
// [asmjit_mainpage]
// ============================================================================

//! \mainpage
//!
//! AsmJit - Complete x86/x64 JIT and Remote Assembler for C++.
//!
//! Introduction provided by the project page at https://github.com/asmjit/asmjit.
//!
//! AsmJit documentation is structured into the following sections:
//! - \ref asmjit_base "Base" - Base API (architecture independent).
//! - \ref asmjit_x86 "X86/X64" - X86/X64 API.

//! \defgroup asmjit_base AsmJit Base API (architecture independent)
//!
//! \brief Base API.
//!
//! Base API contains all classes that are platform and architecture independent.

//! \defgroup asmjit_x86 AsmJit X86/X64 API
//!
//! \brief X86/X64 API
//!
//! X86/X64 Registers
//! -----------------
//!
//! There are static objects that represents X86 and X64 registers. They can
//! be used directly (like `eax`, `mm`, `xmm`, ...) or created through
//! these functions:
//!
//! - `asmjit::x86::gpb_lo()`     - Create an 8-bit low GPB register operand.
//! - `asmjit::x86::gpb_hi()`     - Create an 8-bit high GPB register operand.
//! - `asmjit::x86::gpw()`        - Create a 16-bit GPW register operand.
//! - `asmjit::x86::gpd()`        - Create a 32-bit GPD register operand.
//! - `asmjit::x86::gpq()`        - Create a 64-bit GPQ register operand.
//! - `asmjit::x86::fp()`         - Create a 80-bit FPU register operand.
//! - `asmjit::x86::mm()`         - Create a 64-bit MMX register operand.
//! - `asmjit::x86::k()`          - Create a 64-bit K register operand.
//! - `asmjit::x86::xmm()`        - Create a 128-bit XMM register operand.
//! - `asmjit::x86::ymm()`        - Create a 256-bit YMM register operand.
//! - `asmjit::x86::zmm()`        - Create a 512-bit ZMM register operand.
//! - `asmjit::x86::bnd()`        - Create a 128-bit BND register operand.
//! - `asmjit::x86::cr()`         - Create a 32/64-bit CR (control register) operand.
//! - `asmjit::x86::dr()`         - Create a 32/64-bit DR (debug register) operand.
//!
//! X86/X64 Addressing
//! ------------------
//!
//! X86 and X64 architectures contains several addressing modes and most ones
//! are possible with AsmJit library. Memory represents are represented by
//! `Mem` class. These functions are used to make operands that represents
//! memory addresses:
//!
//! - `asmjit::x86::ptr()`        - Address size not specified.
//! - `asmjit::x86::byte_ptr()`   - 1 byte.
//! - `asmjit::x86::word_ptr()`   - 2 bytes (GPW size).
//! - `asmjit::x86::dword_ptr()`  - 4 bytes (GPD size).
//! - `asmjit::x86::qword_ptr()`  - 8 bytes (GPQ/MMX size).
//! - `asmjit::x86::tword_ptr()`  - 10 bytes (FPU size).
//! - `asmjit::x86::oword_ptr()`  - 16 bytes (XMM size).
//! - `asmjit::x86::yword_ptr()`  - 32 bytes (YMM size).
//! - `asmjit::x86::zword_ptr()`  - 64 bytes (ZMM size).
//!
//! Most useful function to make pointer should be `asmjit::x86::ptr()`. It
//! creates a pointer to the target with an unspecified size. Unspecified size
//! works in all intrinsics where are used registers (this means that size is
//! specified by register operand or by instruction itself). For example
//! `asmjit::x86::ptr()` can't be used with `Assembler::inc()` instruction. In
//! this case the size must be specified and it's also reason to differentiate
//! between pointer sizes.
//!
//! X86 and X86 support simple address forms like `[base + displacement]` and
//! also complex address forms like `[base + index * scale + displacement]`.
//!
//! X86/X64 Immediate Values
//! ------------------------
//!
//! Immediate values are constants thats passed directly after instruction
//! opcode. To create such value use `asmjit::imm()` or `asmjit::imm_u()`
//! methods to create a signed or unsigned immediate value.
//!
//! X86/X64 CPU Information
//! -----------------------
//!
//! The CPUID instruction can be used to get an exhaustive information about
//! the host X86/X64 processor. AsmJit contains utilities that can get the most
//! important information related to the features supported by the CPU and the
//! host operating system, in addition to host processor name and number of
//! cores. Class `CpuInfo` provides generic information about a host or target
//! processor and contains also a specific X86/X64 information.
//!
//! By default AsmJit queries the CPU information after the library is loaded
//! and the queried information is reused by all instances of `JitRuntime`.
//! The global instance of `CpuInfo` can't be changed, because it will affect
//! the code generation of all `Runtime`s. If there is a need to have a
//! specific CPU information which contains modified features or processor
//! vendor it's possible by creating a new instance of the `CpuInfo` and setting
//! up its members.
//!
//! Cpu detection is important when generating a JIT code that may or may not
//! use certain CPU features. For example there used to be a SSE/SSE2 detection
//! in the past and today there is often AVX/AVX2 detection.
//!
//! The example below shows how to detect a SSE4.1 instruction set:
//!
//! ~~~
//! using namespace asmjit;
//!
//! const CpuInfo& cpuInfo = CpuInfo::getHost();
//!
//! if (cpuInfo.hasFeature(CpuInfo::kX86FeatureSSE4_1)) {
//!   // Processor has SSE4.1.
//! }
//! else if (cpuInfo.hasFeature(CpuInfo::kX86FeatureSSE2)) {
//!   // Processor doesn't have SSE4.1, but has SSE2.
//! }
//! else {
//!   // Processor is archaic; it's a wonder AsmJit works here!
//! }
//! ~~~

// [Dependencies]
#include "./base.h"

// [X86/X64]
#if defined(ASMJIT_BUILD_X86)
#include "./x86.h"
#endif // ASMJIT_BUILD_X86

// [ARM32/ARM64]
#if defined(ASMJIT_BUILD_ARM)
#include "./arm.h"
#endif // ASMJIT_BUILD_ARM

// [Guard]
#endif // _ASMJIT_ASMJIT_H
