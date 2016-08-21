// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86FUNC_P_H
#define _ASMJIT_X86_X86FUNC_P_H

#include "../build.h"

// [Dependencies]
#include "../base/func.h"
#include "../x86/x86emitter.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::X86FuncUtils]
// ============================================================================

struct X86FuncUtils {
  static Error initCallConv(CallConv& cc, uint32_t ccId) noexcept;
  static Error initFuncDecl(FuncDecl& decl, const FuncSignature& sign, uint32_t gpSize) noexcept;
  static Error initFuncLayout(FuncLayout& layout, const FuncDecl& decl, const FuncFrame& frame) noexcept;

  static Error insertProlog(X86Emitter* emitter, const FuncLayout& layout);
  static Error insertEpilog(X86Emitter* emitter, const FuncLayout& layout);
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86FUNC_P_H
