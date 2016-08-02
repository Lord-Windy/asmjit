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
//! A complete JIT and remote assembler for C++ language. It can generate native
//! code for x86 and x64 architectures and supports the whole x86/x64 instruction
//! set - from legacy MMX to the newest AVX2. It has a type-safe API that allows
//! C++ compiler to do semantic checks at compile-time even before the assembled
//! code is generated and executed.
//!
//! AsmJit is not a virtual machine (VM). It doesn't have functionality to
//! implement VM out of the box; however, it can be be used as a JIT backend
//! of your own VM. The usage of AsmJit is not limited at all; it's suitable
//! for multimedia, VM backends, remote code generation, and many other tasks.
//!
//! \section AsmJit_Main_Concepts Code Generation Concepts
//!
//! AsmJit has two completely different code generation concepts. The difference
//! is in how the code is generated. The first concept, also referred as a low
//! level concept, is called `Assembler` and it's the same as writing RAW
//! assembly by inserting instructions that use physical registers directly. In
//! this case AsmJit does only instruction encoding, verification and final code
//! relocation.
//!
//! The second concept, also referred as a high level concept, is called
//! `Compiler`. Compiler lets you use virtually unlimited number of registers
//! (it calls them variables), which significantly simplifies the code generation
//! process. Compiler allocates these virtual registers to physical registers
//! after the code generation is done. This requires some extra effort - Compiler
//! has to generate information for each node (instruction, function declaration,
//! function call, etc...) in the code, perform a variable liveness analysis and
//! translate the code using variables to a code that uses only physical registers.
//!
//! In addition, Compiler understands functions and their calling conventions.
//! It has been designed in a way that the code generated is always a function
//! having a prototype like a real programming language. By having a function
//! prototype the Compiler is able to insert prolog and epilog sequence to the
//! function being generated and it's able to also generate a necessary code
//! to call other function from your own code.
//!
//! There is no conclusion on which concept is better. `Assembler` brings full
//! control and the best performance, while `Compiler` makes the code-generation
//! more fun and more portable.
//!
//! \section AsmJit_Main_Sections Documentation Sections
//!
//! AsmJit documentation is structured into the following sections:
//! - \ref asmjit_base "Base" - Base API (architecture independent).
//! - \ref asmjit_x86 "X86/X64" - X86/X64 API.
//!
//! \section AsmJit_Main_HomePage AsmJit Homepage
//!
//! - https://github.com/kobalicek/asmjit

// ============================================================================
// [asmjit_base]
// ============================================================================

//! \defgroup asmjit_base AsmJit Base API (architecture independent)
//!
//! \brief Base API.
//!
//! Base API contains all classes that are platform and architecture independent.
//!
//! Code-Generation and Operands
//! ----------------------------
//!
//! List of the most useful code-generation and operand classes:
//! - \ref asmjit::Assembler - Low-level code-generation.
//! - \ref asmjit::AsmBuilder - Builder that allows to build and keep \ref AsmNode instances
//!   - \ref asmjit::AsmNode - base class for any node:
//!     - \ref asmjit::AsmInst - Instruction/Jump.
//!     - \ref asmjit::AsmData - Data.
//!     - \ref asmjit::AsmAlign - Align directive.
//!     - \ref asmjit::AsmLabel - Label.
//!     - \ref asmjit::AsmComment - Comment.
//!     - \ref asmjit::AsmSentinel - Sentinel (used as a marker, has no other function).
//!     - \ref asmjit::AsmFunc - Function entry, compatible with \ref AsmLabel (Compiler).
//!     - \ref asmjit::AsmFuncRet - Function return (Compiler).
//!     - \ref asmjit::AsmCall - Function call (Compiler).
//!     - \ref asmjit::AsmPushArg - Push stack argument (Compiler).
//!     - \ref asmjit::AsmHint - Register allocator hint (Compiler).
//! - \ref asmjit::Runtime - Describes where the code is stored and how it's executed:
//!   - \ref asmjit::HostRuntime - Runtime that runs on the host machine:
//!     - \ref asmjit::JitRuntime - Runtime designed for JIT code generation and execution.
//! - \ref asmjit::Operand - base class for all operands:
//!   - \ref asmjit::Reg - Register operand (`Assembler` only).
//!   - \ref asmjit::Var - Variable operand (`Compiler` only).
//!   - \ref asmjit::Mem - Memory operand.
//!   - \ref asmjit::Imm - Immediate operand.
//!   - \ref asmjit::Label - Label operand.
//!
//! The following snippet shows how to setup a basic JIT code generation:
//!
//! ~~~
//! using namespace asmjit;
//!
//! int main(int argc, char* argv[]) {
//!   // JIT runtime is designed for JIT code generation and execution.
//!   JitRuntime runtime;
//!
//!   // Assembler instance requires to know the runtime to function.
//!   X86Assembler a(&runtime);
//!
//!   // Compiler (if you indend to use it) requires an assembler instance.
//!   X86Compiler c(&a);
//!
//!   return 0;
//! }
//! ~~~
//!
//! Zone Memory Allocator
//! ---------------------
//!
//! Zone memory allocator is an incremental memory allocator that can be used
//! to allocate data of short life-time. It has much better performance
//! characteristics than all other allocators, because the only thing it can do
//! is to increment a pointer and return its previous address. See \ref Zone
//! for more details.
//!
//! The whole AsmJit library is based on zone memory allocation for performance
//! reasons. It has many other benefits, but the performance was the main one
//! when designing the library.
//!
//! POD Containers
//! --------------
//!
//! POD containers are used by AsmJit to manage its own data structures. The
//! following classes can be used by AsmJit consumers:
//!
//!   - \ref asmjit::BitArray - A fixed bit-array that is used internally.
//!   - \ref asmjit::PodVector<T> - A simple array-like container for storing
//!     POD data.
//!   - \ref asmjit::PodList<T> - A single linked list.
//!   - \ref asmjit::StringBuilder - A string builder that can append strings
//!     and integers.
//!
//! Utility Functions
//! -----------------
//!
//! Utility functions are implementated static class \ref Utils. There are
//! utilities for bit manipulation and bit counting, utilities to get an
//! integer minimum / maximum and various other helpers required to perform
//! alignment checks and binary casting from float to integer and vice versa.
//!
//! String utilities are also implemented by a static class \ref Utils. They
//! are mostly used by AsmJit internals and not really important to end users.
//!
//! SIMD Utilities
//! --------------
//!
//! SIMD code generation often requires to embed constants after each function
//! or at the end of the whole code block. AsmJit contains `Data64`, `Data128`
//! and `Data256` classes that can be used to prepare data useful when generating
//! SIMD code.
//!
//! X86/X64 code generators contain member functions `dmm`, `dxmm`, and `dymm`,
//! which can be used to embed 64-bit, 128-bit and 256-bit data structures into
//! the machine code.

// ============================================================================
// [asmjit_x86]
// ============================================================================

//! \defgroup asmjit_x86 AsmJit X86/X64 API
//!
//! \brief X86/X64 API
//!
//! X86/X64 Code Generation
//! -----------------------
//!
//! X86/X64 code generation is realized throught:
//! - \ref X86Assembler - low-level code generation.
//! - \ref X86Compiler - high-level code generation.
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
//! - `asmjit::x86::cr()`         - Create a 32/64-bit CR (control register) operand.
//! - `asmjit::x86::dr()`         - Create a 32/64-bit DR (debug register) operand.
//! - `asmjit::x86::bnd()`        - Create a 128-bit BND register operand.
//! - `asmjit::x86::xmm()`        - Create a 128-bit XMM register operand.
//! - `asmjit::x86::ymm()`        - Create a 256-bit YMM register operand.
//! - `asmjit::x86::zmm()`        - Create a 512-bit ZMM register operand.
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

// [Host]
#include "./host.h"

// [Guard]
#endif // _ASMJIT_ASMJIT_H
