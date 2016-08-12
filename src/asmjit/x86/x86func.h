// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86FUNC_H
#define _ASMJIT_X86_X86FUNC_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/func.h"
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86FuncDecl]
// ============================================================================

//! X86 function, including calling convention, arguments and their
//! register indexes or stack positions.
struct X86FuncDecl : public FuncDecl {
  //! Create a new `X86FuncDecl` instance.
  ASMJIT_INLINE X86FuncDecl() { reset(); }

  //! Set function signature.
  //!
  //! This will set function calling convention and setup arguments variables.
  //!
  //! NOTE: This function will allocate variables, it can be called only once.
  ASMJIT_API Error setSignature(const FuncSignature& p);
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_X86_X86FUNC_H
