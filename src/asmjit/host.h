// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_HOST_H
#define _ASMJIT_HOST_H

// [Dependencies]
#include "./base.h"

// [X86 / X64]
#if ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64
#include "./x86.h"

namespace asmjit {

namespace host { using namespace ::asmjit::x86; }
typedef X86Assembler HostAssembler;

#if !defined(ASMJIT_DISABLE_COMPILER)
typedef X86Compiler HostCompiler;
typedef X86CallNode HostCallNode;
typedef X86FuncDecl HostFuncDecl;
typedef X86FuncNode HostFuncNode;
#endif // !ASMJIT_DISABLE_COMPILER

} // asmjit namespace

#endif // ASMJIT_ARCH_X86 || ASMJIT_ARCH_X64

// [Guard]
#endif // _ASMJIT_HOST_H
