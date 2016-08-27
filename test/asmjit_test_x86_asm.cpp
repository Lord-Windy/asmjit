// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Dependencies]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "./asmjit.h"
#include "./asmjit_test_misc.h"

using namespace asmjit;

typedef void (*SumIntsFunc)(int* dst, const int* a, const int* b);

int main(int argc, char* argv[]) {
  JitRuntime rt;                          // Create JIT Runtime

  CodeHolder code;                        // Create a CodeHolder.
  code.init(rt.getCodeInfo());            // Initialize it to match `rt`.
  X86Assembler a(&code);                  // Create and attach X86Assembler to `code`.

  FileLogger logger(stderr);
  code.setLogger(&logger);

  // Decide which registers will be mapped to function arguments. Try changing
  // registers of `dst`, `src_a`, and `src_b` and see what happens in function's
  // prolog and epilog.
  X86Gp dst   = a.zax();
  X86Gp src_a = a.zcx();
  X86Gp src_b = a.zdx();

  X86Xmm vec0 = x86::xmm0;
  X86Xmm vec1 = x86::xmm1;

  // Create and initialize `FuncDetail` and `FuncFrameInfo`. Both are
  // needed to create a function and they hold different kind of data.
  FuncDetail func;
  func.init(FuncSignature3<void, int*, const int*, const int*>(CallConv::kIdHost));

  FuncFrameInfo ffi;
  ffi.setDirtyRegs(X86Reg::kKindVec,      // Make XMM0 and XMM1 dirty. VEC kind
                   Utils::mask(0, 1));    // describes XMM|YMM|ZMM registers.

  FuncArgsMapper args(&func);             // Create function arguments mapper.
  args.assignAll(dst, src_a, src_b);      // Assign our registers to arguments.
  args.updateFrameInfo(ffi);              // Reflect our args in FuncFrameInfo.

  FuncFrameLayout layout;                 // Create the FuncFrameLayout, which
  layout.init(func, ffi);                 // contains metadata of prolog/epilog.

  FuncUtils::emitProlog(&a, layout);      // Emit function prolog.
  FuncUtils::allocArgs(&a, layout, args); // Allocate arguments to registers.
  a.movdqu(vec0, x86::ptr(src_a));        // Load 4 ints from [src_a] to XMM0.
  a.movdqu(vec1, x86::ptr(src_b));        // Load 4 ints from [src_b] to XMM1.
  a.paddd(vec0, vec1);                    // Add 4 ints in XMM1 to XMM0.
  a.movdqu(x86::ptr(dst), vec0);          // Store the result to [dst].
  FuncUtils::emitEpilog(&a, layout);      // Emit function epilog and return.

  SumIntsFunc fn;
  Error err = rt.add(&fn, &code);         // Add the code generated to the runtime.
  if (err) return 1;                      // Handle a possible error case.

  // Execute the generated function.
  int inA[4] = { 4, 3, 2, 1 };
  int inB[4] = { 1, 5, 2, 8 };
  int out[4];
  fn(out, inA, inB);

  // Prints {5 8 4 9}
  printf("{%d %d %d %d}\n", out[0], out[1], out[2], out[3]);

  rt.release(fn);

  if (out[0] == 5 && out[1] == 8 && out[2] == 4 && out[3] == 9)
    return 0;
  else
    return 1;
}
