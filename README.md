AsmJit
------

Complete x86/x64 JIT and Remote Assembler for C++.

  * [Official Repository (asmjit/asmjit)](https://github.com/asmjit/asmjit)
  * [Official Blog (asmbits)](https://asmbits.blogspot.com/ncr)
  * [Official Chat (gitter)](https://gitter.im/asmjit/asmjit)
  * [Permissive ZLIB license](./LICENSE.md)

'NEXT' TODO:

  * This readme contains outdated code.
  * Named labels and symbol support required.
  * Only the first section works atm.
  * ArchInfo needs a better name, ArchInfo requires also target feature set for x86 (None, AVX, AVX512) and ARM (None, Thumb).
  * DON'T USE asmjit::Compiler with this version, it requires some fixing, only use to evaluate new X86Assembler and X86Inst features.
  * AVX512 validation not supported yet.
  * AVX512 {sae} and {er} not supported yet.

Introduction
------------

AsmJit is a complete JIT and remote assembler for C++ language. It can generate native code for x86 and x64 architectures and supports the whole x86/x64 instruction set - from legacy MMX to the newest AVX512. It has a type-safe API that allows C++ compiler to do semantic checks at compile-time even before the assembled code is generated and/or executed.

AsmJit is not a virtual machine (VM). It doesn't have functionality to implement VM out of the box; however, it can be be used as a JIT backend of your own VM. The usage of AsmJit is not limited at all; it's suitable for multimedia, VM backends, remote code generation, and many other tasks.

Features
--------

  * Complete x86/x64 instruction set - MMX, SSEx, BMIx, ADX, TBM, XOP, AVXx, FMAx, and AVX512.
  * Assembler, AsmBuilder, and Compiler code-generation concepts - each suitable for different tasks.
  * Built-in CPU vendor and features detection.
  * Advanced logging/formatting and robust error handling.
  * Virtual memory management similar to malloc/free for JIT code generation and execution.
  * Lightweight and embeddable - 200-250kB compiled (with all built-in features).
  * Some built-in features can be disabled to make the library even smaller.
  * Zero dependencies - no external libraries, no STL/RTTI.
  * Doesn't use exceptions, but allows to plug-in a "throwable" error handler.

Important
---------

  * Breaking changes are described in [BREAKING.md](./BREAKING.md)

Supported Environments
----------------------

### Operating Systems

  * BSDs (not tested regularly).
  * Linux (tested by Travis-CI).
  * Mac (tested by Travis-CI).
  * Windows (tested manually, at least WinXP).

### C++ Compilers

  * Clang (tested by Travis-CI).
  * CodeGear (including BorlandC++, not tested regularly).
  * GCC (tested by Travis-CI).
  * MinGW (tested manually).
  * MSVC (tested manually, at least VS2003 is required).
  * Other compilers require some testing and support in [asmjit/build.h](./asmjit/build.h).

### Backends

  * ARM (work-in-progress).
  * X86 (tested by Travis-CI).
  * X64 (tested by Travis-CI).

Project Organization
--------------------

  * `/`             - Project root
    * `src`         - Source code
      * `asmjit`    - Source code and headers (always point include path in here)
        * `base`    - Generic API and interfaces, used by all backends
        * `arm`     - ARM/ARM64 specific API, used only by ARM and ARM64 backends
        * `x86`     - X86/X64 specific API, used only by X86 and X64 backends
      * `test`      - Unit and integration tests (don't embed in your project)
    * `tools`       - Tools used for configuring, documenting and generating files

Configuring & Building
----------------------

AsmJit is designed to be easy embeddable in any project. However, it depends on some compile-time macros that can be used to build a specific version of AsmJit that includes or excludes certain features. A typical way of building AsmJit is to use [cmake](http://www.cmake.org), but it's also possible to just include AsmJit source code in our project and just build it. The easiest way to include AsmJit in your project is to just include AsmJit sources within and to define `ASMJIT_STATIC` (preprocessor). This way AsmJit can be just updated from time to time without any changes to it. Do not include AsmJit test files (`asmjit/test` directory) in such case.

### Build Type

  * `ASMJIT_EMBED` - Parameter that can be set to cmake to turn off building library, useful if you want to include asmjit in your project without building the library. `ASMJIT_EMBED` behaves identically as `ASMJIT_STATIC`.
  * `ASMJIT_STATIC` - Define when building AsmJit as a static library. No symbols will be exported by AsmJit by default.

  * By default AsmJit build is configured as a shared library so none of `ASMJIT_EMBED` and `ASMJIT_STATIC` have to be defined explicitly.

### Build Mode

  * `ASMJIT_DEBUG` - Define to always turn debugging on (regardless of build-mode).
  * `ASMJIT_RELEASE` - Define to always turn debugging off (regardless of build-mode).
  * `ASMJIT_TRACE` - Define to enable AsmJit tracing. Tracing is used to catch bugs in AsmJit and it has to be enabled explicitly. When AsmJit is compiled with `ASMJIT_TRACE` it uses `stdout` to log information related to AsmJit execution. This log can be helpful when examining liveness analysis, register allocation or any other part of AsmJit.

  * By default none of these is defined, AsmJit detects mode based on compile-time macros (useful when using IDE that has switches for Debug/Release/etc...).

### Architectures

  * `ASMJIT_BUILD_ARM32` - Build ARM32 and ARM64 backend.
  * `ASMJIT_BUILD_X86` - Build X86 and X64 backend.
  * `ASMJIT_BUILD_HOST` - Build host backend, if only `ASMJIT_BUILD_HOST` is used only the host architecture detected at compile-time will be included.

  * By default only `ASMJIT_BUILD_HOST` is defined.

### Features

  * `ASMJIT_DISABLE_TEXT` - Disable everything that uses text-representation and that causes certain strings to be stored in the resulting binary. For example when this flag is enabled all instruction and error names (and related APIs) will not be available. This flag has to be disabled together with `ASMJIT_DISABLE_LOGGING`. This option is suitable for deployment builds.
  * `ASMJIT_DISABLE_LOGGING` - Disable `Logger` and `Formatter` features completely. Use this flag if you don't need `Logger` and `Formatter` classes, suitable for deployment builds.
  * `ASMJIT_DISABLE_COMPILER` - Disable `Compiler` completely. Use this flag if you don't use `Compiler` and don't want it compiled in.

Using AsmJit
------------

AsmJit library uses one global namespace called `asmjit` that provides the whole API. Architecture specific code is prefixed by the architecture name and architecture specific registers and operand builders have their own namespace. For example API targeting both X86 and X64 architectures is prefixed with "X86", enums by `kX86`, and registers & operand builders are accessible through `x86` namespace. This design is very different from the initial version of AsmJit and it seems now as the most convenient one.

### Runtime & Emitters

AsmJit provides `Runtime` and code emitters like `Assembler`, `AsmBuilder`, and `Compiler`. `Runtime` specifies where the code will be emitted and acts as a storage while the rest provide API to actually emit the code - assembler instructions, labels, directives, and high-level features offered by `Compiler`. Each code emitter is suitable for something else:

  * `Assembler` - Encodes instructions and data directly into the code-buffer. Assembler is a low-level concept and the most efficient way of generating machine-code. Assembler is suitable for people that already have their own register allocator and demand a full control over the generated machine-code. It uses physical registers directly and doesn't provide high-level features like code injection or handling function calling conventions offered by other emitters. Assembler supports code relocation and the generated machine-code can be relocated to any user-defined addresses.
  * `AsmBuilder` - Very similar to `Assembler`, but instead of emitting machine-code it stores everything in a representation suitable for additional processing. It uses nodes (`AsmNode` and friends) to represent instructions and other building blocks. Since `AsmBuilder` does emit the code directly it's suitable for inspecting, injecting, and patching.
  * `Compiler` - Based on `AsmBuilder`, but provides virtual registers that are allocated into physical registers by a register allocator. Compiler is basically a high-level assembler that is very easy to start with. It understands functions and their calling conventions. It was designed in a way that the code generated is always a function having a certain signature like in a real programming language. By having information about functions and their signatures it' able to insert prolog and epilog sequence automatically it's able to also generate a necessary code to call other function from the generated code. The `Compiler` is the simplest way to start using AsmJit.

### Instruction Operands

Operand is a part of an instruction, which specifies the data the instruction will operate on. All operands share the same `Operand` base class and have the same size, it's possible to define an array of operands and then just setup each operand individually setting their types and contents dynamically. There are five types of operands in AsmJit:

  * None - operand not initialized or not used, default when you create `Operand()`.
  * Register - describes either physical or virtual register, all registers inherit from `Reg` operand and then specialize, for example X86/X64 defines `X86Reg` operand, which contains some archiecture-specific functionality and constants.
  * Memory address - used to reference a memory location, provided by `Mem` class, specialized by `X86Mem` for X86/X64.
  * Immediate value - Immediate values are usually part of instructions (encoded within the instruction itself) or data, provided by `Imm` class.
  * Label - used to reference a location in code or data, provided by `Label` class.

Base class for all operands is `Operand`. It contains interface that can be used by all types of operands only and it is typically passed by value, not as a pointer. The classes `Reg`, `Mem`, `Label` and `Imm` all inherit from `Operand` and provide operand-specific functionality. Architecture specific operands are prefixed by the architecture like `X86Reg` and `X86Mem`. Most of architectures provide several types of registers, for example X86/X64 architecture has `X86Gp`, `X86Mm`, `X86Fp`, `X86Xmm`, `X86Ymm`, and others and also provides some extras including segment registers and `rip` (relative instruction pointer).

When using a code-generator some operands have to be created explicitly by using its interface. For example labels are created by using `newLabel()` method of the code-generator and virtual registers are created by using architecture specific methods like `newGp()`, `newMm()` or `newXmm()`.

### Function Prototypes

AsmJit needs to know the prototype of the function it will generate or call. AsmJit contains a mapping between a type and the register that will be used to represent it. To make life easier there is a function builder that does the mapping on the fly. Function builder is a template class that helps with creating a function prototype by using native C/C++ types that describe function arguments and return value. It translates C/C++ native types into AsmJit specific IDs and makes these IDs accessible to Compiler.

### Putting It All Together

Let's put all together and generate a first function that sums its two arguments and returns the result. At the end the generated function is called from a C++ code.

```c++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  // Create JitRuntime.
  JitRuntime rt;

  // Create X86Assembler and X86Compiler.
  X86Assembler a(&rt);
  X86Compiler c(&a);

  // Build function having two arguments and a return value of type 'int'.
  // First type in function builder describes the return value. kCallConvHost
  // tells the compiler to use the host calling convention.
  c.addFunc(FuncBuilder2<int, int, int>(kCallConvHost));

  // Create 32-bit virtual registers and assign some names to them. Using names
  // is not necessary, however, it can make debugging easier as these show in
  // annotations, if turned-on.
  X86Gp x = c.newI32("x");
  X86Gp y = c.newI32("y");

  // Tell asmjit to use these variables as function arguments.
  c.setArg(0, x);
  c.setArg(1, y);

  // x = x + y;
  c.add(x, y);

  // Tell asmjit to return `x`.
  c.ret(x);

  // Finalize the current function.
  c.endFunc();

  // Now the Compiler contains the whole function, but the code is not yet
  // generated. To tell the compiler to serialize the code to `Assembler`
  // `c.finalize()` has to be called. After finalization the `Compiler`
  // won't contain the code anymore and will be detached from the `Assembler`.
  c.finalize();

  // After finalization the code has been send to `Assembler`. It contains
  // a handy method `make()`, which returns a pointer that points to the
  // first byte of the generated code, which is the function entry in our
  // case.
  void* funcPtr = a.make();

  // In order to run 'funcPtr' it has to be casted to the desired type.
  // Typedef is a recommended and safe way to create a function-type.
  typedef int (*FuncType)(int, int);

  // Using asmjit_cast is purely optional, it's basically a C-style cast
  // that tries to make it visible that a function-type is returned.
  FuncType func = asmjit_cast<FuncType>(funcPtr);

  // Finally, run it and do something with the result...
  int z = func(1, 2);
  printf("z=%d\n", z); // Outputs "z=3".

  // The function will remain in memory after Compiler and Assembler are
  // destroyed. This is why the `JitRuntime` is used - it keeps track of
  // the code generated. When `Runtime` is destroyed it also invalidates
  // all code relocated by it (which is in our case also our `func`). So
  // it's safe to just do nothing in our case, because destroying `Runtime`
  // will free `func` as well, however, it's always better to release the
  // generated code that is not needed anymore manually.
  rt.release((void*)func);

  return 0;
}
```

The code should be self explanatory, however there are some details to be clarified.

The code above generates and calls a function of `kCallConvHost` calling convention. 32-bit architecture contains a wide range of function calling conventions that can be all used by a single program, so it's important to know which calling convention is used by your C/C++ compiler so you can call the function. However, most compilers should generate CDecl by default. In 64-bit mode there are only two calling conventions, one is specific for Windows (Win64 calling convention) and the other for Unix (AMD64 calling convention). The `kCallConvHost` is defined to be one of CDecl, Win64 or AMD64 depending on your architecture and operating system.

Default integer size is platform specific, virtual types `kVarTypeIntPtr` and `kVarTypeUIntPtr` can be used to make the code more portable and they should be always used when a pointer type is needed. When no type is specified AsmJit always defaults to `kVarTypeIntPtr`. The code above works with integers where the default behavior has been overidden to 32-bits. Note it's always a good practice to specify the type of the variable used. Alternative form of creating a variable is `c.newGpVar(...)`, `c.newMmVar(...)`, `c.newXmmVar` and so on...

The function starts with `c.addFunc()` and ends with `c.endFunc()`. It's not allowed to put code outside of the function; however, embedding data outside of the function body is allowed.

### Using Labels

Labels are essential for making jumps, function calls or to refer to a data that is embedded in the code section. Label has to be explicitly created by using `newLabel()` method of your code generator in order to be used. The following example executes a code that depends on the condition by using a `Label` and conditional jump instruction. If the first parameter is zero it returns `a + b`, otherwise `a - b`.

```c++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime rt;

  X86Assembler a(&rt);
  X86Compiler c(&a);

  // This function uses 3 arguments - `int func(int, int, int)`.
  c.addFunc(FuncBuilder3<int, int, int, int>(kCallConvHost));

  X86Gp op = c.newI32("op");
  X86Gp x = c.newI32("x");
  X86Gp y = c.newI32("y");

  c.setArg(0, op);
  c.setArg(1, x);
  c.setArg(2, y);

  // Create labels.
  Label L_Sub = c.newLabel();
  Label L_Skip = c.newLabel();

  // If (op != 0)
  //   goto L_Sub;
  c.test(op, op);
  c.jne(L_Sub);

  // x = x + y;
  // goto L_Skip;
  c.add(x, y);
  c.jmp(L_Skip);

  // L_Sub:
  // x = x - y;
  c.bind(L_Sub);
  c.sub(x, y);

  // L_Skip:
  c.bind(L_Skip);

  c.ret(x);
  c.endFunc();
  c.finalize();

  // The prototype of the generated function.
  typedef int (*FuncType)(int, int, int);
  FuncType func = asmjit_cast<FuncType>(a.make());

  int res0 = func(0, 1, 2);
  int res1 = func(1, 1, 2);

  printf("res0=%d\n", res0); // Outputs "res0=3".
  printf("res1=%d\n", res1); // Outputs "res1=-1".

  rt.release((void*)func);
  return 0;
}
```

In this example conditional and unconditional jumps were used with labels together. Labels have to be created explicitely by `Compiler` by using a `Label L = c.newLabel()` form. Each label as an unique ID that identifies it, however it's not a string and there is no way to query for a `Label` instance that already exists at the moment. Label is like any other operand moved by value, so the copy of the label will still reference the same label and changing a copied label will not change the original label.

Each label has to be bound to the location in the code by using `bind()`; however, it can be bound only once! Trying to bind the same label multiple times has undefined behavior - assertion failure is the best case.

### Memory Addressing

X86/X64 architectures have several memory addressing modes which can be used to combine base register, index register and a displacement. In addition, index register can be shifted by a constant from 1 to 3 that can help with addressing elements up to 8-byte long in an array. AsmJit supports all forms of memory addressing. Memory operand can be created by using `asmjit::X86Mem` or by using related non-member functions like `asmjit::x86::ptr` or `asmjit::x86::ptr_abs`. Use `ptr` to create a memory operand having a base register with optional index register and a displacement; use and `ptr_abs` to create a memory operand referring to an absolute address in memory (32-bit) and optionally having an index register.

In the following example various memory addressing modes are used to demonstrate how to construct and use them. It creates a function that accepts an array and two indexes which specify which elements to sum and return.

```c++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime rt;

  X86Assembler a(&rt);
  X86Compiler c(&a);

  // Function returning 'int' accepting pointer and two indexes.
  c.addFunc(FuncBuilder3<int, const int*, intptr_t, intptr_t>(kCallConvHost));

  X86Gp p = c.newIntPtr("p");
  X86Gp xIndex = c.newIntPtr("xIndex");
  X86Gp yIndex = c.newIntPtr("yIndex");

  c.setArg(0, p);
  c.setArg(1, xIndex);
  c.setArg(2, yIndex);

  X86Gp x = c.newI32("x");
  X86Gp y = c.newI32("y");

  // Read `x` by using a memory operand having base register, index register
  // and scale. Translates to `mov x, dword ptr [p + xIndex << 2]`.
  c.mov(x, x86::ptr(p, xIndex, 2));

  // Read `y` by using a memory operand having base register only. Registers
  // `p` and `yIndex` are both modified.

  // Shift bIndex by 2 (exactly the same as multiplying by 4).
  // And add scaled 'bIndex' to 'p' resulting in 'p = p + bIndex * 4'.
  c.shl(yIndex, 2);
  c.add(p, yIndex);

  // Read `y`.
  c.mov(y, x86::ptr(p));

  // x = x + y;
  c.add(x, y);

  c.ret(x);
  c.endFunc();
  c.finalize();

  // The prototype of the generated function.
  typedef int (*FuncType)(const int*, intptr_t, intptr_t);
  FuncType func = asmjit_cast<FuncType>(a.make());

  // Array passed to `func`.
  static const int array[] = { 1, 2, 3, 5, 8, 13 };

  int xVal = func(array, 1, 2);
  int yVal = func(array, 3, 5);

  printf("xVal=%d\n", xVal); // Outputs "xVal=5".
  printf("yVal=%d\n", yVal); // Outputs "yVal=18".

  rt.release((void*)func);
  return 0;
}
```

### Using Stack

AsmJit uses stack automatically to spill virtual registers if there is not enough physical registers to keep them all allocated. The stack frame is managed by `Compiler` that provides also an interface to allocate chunks of memory of user specified size and alignment.

In the following example a stack of 256 bytes size is allocated, filled by bytes starting from 0 to 255 and then iterated again to sum all the values.

```c++
#include <asmjit/asmjit.h>

using namespace asmjit;

int main(int argc, char* argv[]) {
  JitRuntime rt;

  X86Assembler a(&rt);
  X86Compiler c(&a);

  // Function returning 'int' without any arguments.
  c.addFunc(FuncBuilder0<int>(kCallConvHost));

  // Allocate 256 bytes on the stack aligned to 4 bytes.
  X86Mem stack = c.newStack(256, 4);

  X86Gp p = c.newIntPtr("p");
  X86Gp i = c.newIntPtr("i");

  // Load a stack address to `p`. This step is purely optional and shows
  // that `lea` is useful to load a memory operands address (even absolute)
  // to a general purpose register.
  c.lea(p, stack);

  // Clear `i`. Notice that `xor_()` is used instead of `xor()` as it's keyword.
  c.xor_(i, i);

  Label L1 = c.newLabel();
  Label L2 = c.newLabel();

  // First loop, fill the stack allocated by a sequence of bytes from 0 to 255.
  c.bind(L1);

  // Mov byte ptr[p + i], i.
  //
  // Any operand can be cloned and modified. By cloning `stack` and calling
  // `setIndex()` we created a new memory operand based on stack having an
  // index register assigned to it.
  c.mov(stack.clone().setIndex(i), i.r8());

  // if (++i < 256)
  //   goto L1;
  c.inc(i);
  c.cmp(i, 256);
  c.jb(L1);

  // Second loop, sum all bytes stored in `stack`.
  X86Gp sum = c.newI32("sum");
  X86Gp val = c.newI32("val");

  c.xor_(i, i);
  c.xor_(sum, sum);

  c.bind(L2);

  // Movzx val, byte ptr [stack + i]
  c.movzx(val, stack.clone().setIndex(i).setSize(1));
  // sum += val;
  c.add(sum, val);

  // if (++i < 256)
  //   goto L2;
  c.inc(i);
  c.cmp(i, 256);
  c.jb(L2);

  c.ret(sum);
  c.endFunc();
  c.finalize();

  typedef int (*FuncType)(void);
  FuncType func = asmjit_cast<FuncType>(a.make());

  printf("sum=%d\n", func()); // Outputs "sum=32640".

  rt.release((void*)func);
  return 0;
}
```

### Built-In Logging

Failures are common when working at machine-code level. AsmJit does already a good job with function overloading to prevent from emitting semantically incorrect instructions; however, AsmJit can't prevent from emitting code that is semantically correct, but contains bug(s). Logging has always been an important part of AsmJit's infrastructure and the output can be very valuable after something went wrong.

AsmJit contains extensible logging interface defined by `Logger` and specialized by `FileLogger` and `StringLogger` classes (you can create your own as well). `FileLogger` can log into a standard C-based `FILE*` stream while `StringLogger` logs into an internal buffer that can be used after the code generation is done.

After a logger is attached to `Assembler` its methods will be called every time `Assembler` emits something. A single `Logger` instance can be used multiple times, however, loggers that contain state(s) like `StringLogger` must not be used by two or more threads at the same time.

The following snippet describes how to log into `FILE*`:

```c++
// Create logger logging to `stdout`. Logger life-time should always be
// greater than a life-time of the code generator. Alternatively the
// logger can be reset before it's destroyed.
FileLogger logger(stdout);

// Create runtime and assembler and attach the logger to the assembler.
JitRuntime rt;
X86Assembler a(&rt);
a.setLogger(&logger);

// ... Generate the code ...
```

The following snippet describes how to log into a string:

```c++
StringLogger logger;

JitRuntime rt;
X86Assembler a(&rt);
a.setLogger(&logger);

// ... Generate the code ...

printf("Logger Content:\n%s", logger.getString());

// You can use `logger.clearString()` if the intend is to reuse the logger.
```

Logger can be configured to show more information by using `logger.addOptions()` method. The following options are available:

  * `Logger::kOptionBinaryForm` - Output instructions also in binary form.
  * `Logger::kOptionHexImmediate` - Output constants in hexadecimal form.
  * `Logger::kOptionHexDisplacement` - Output displacements in hexadecimal form.

### Error Handling

AsmJit uses error codes to represent and return errors; and every function where error can occur will also return it. It's recommended to only check errors of the most important functions and to write a custom `ErrorHandler` to handle the rest. An error can happen in many places, but error handler is mostly used by `Assembler` to report a fatal problem. There are multiple ways of using `ErrorHandler`:

  * 1. Returning `true` or `false` from `handleError()`. If `true` is returned it means that error was handled and AsmJit can continue execution. The error code still be propagated to the caller, but won't put the origin into an error state (it won't set last-error). However, `false` reports to AsmJit that the error cannot be handled - in such case it stores the error, which can retrieved later by `getLastError()`. Returning `false` is the default behavior when no error handler is provided. To put the assembler into a non-error state again the `resetLastError()` must be called.
  * 2. Throwing an exception. AsmJit doesn't use exceptions and is completely exception-safe, but you can throw exception from the error handler if this way is easier / preferred by you. Throwing an exception acts virtually as returning `true` - AsmJit won't store the error.
  * 3. Using plain old C's `setjmp()` and `longjmp()`. Asmjit always puts `Assembler` and `Compiler` to a consistent state before calling the `handleError()` so `longjmp()` can be used without issues to cancel the code-generation if an error occurred.

Here is an example of using `ErrorHandler` to just print the error but do nothing else:

```c++
// Error handling #1:
#include <asmjit/asmjit.h>

using namespace asmjit;

class MyErrorHandler : public ErrorHandler {
public:
  // Return `true` to set last error to `err`, return `false` to do nothing.
  // The `origin` points to the `X86Assembler` instance (&a in our case).
  virtual bool handleError(Error err, const char* message, void* origin) {
    printf("ASMJIT ERROR: 0x%0.8X [%s]\n", err, message);
    return false;
  }
};

int main(int argc, char* argv[]) {
  JitRuntime rt;
  MyErrorHandler eh;

  X86Assembler a(&rt);
  a.setErrorHandler(&eh);

  // Use RAW emit to emit an illegal instruction.
  Error err = a.emit(X86Inst::kIdMov, x86::eax, x86::xmm4, x86::xmm1);

  // Message printed, the error contains the same error as passed to the
  // error handler. Since we returned `false` the assembler is in an error
  // state. Use `resetLastError` to reset it back to normal.
  assert(a.getLastError() == err);
  a.resetLastError();

  // After the error is reset it should return `kErrorOk` like nothing happened.
  assert(a.getLastError() == kErrorOk);

  return 0;
}
```

NOTE: If error happens during instruction emitting / encoding the assembler behaves transactionally - the output buffer won't advance if failed, thus a fully encoded instruction is either emitted or not. AsmJit is very safe and strict in this regard. The error handling shown above is useful, but it's still not the best way of dealing with errors in AsmJit. The following example shows how to use exception handling to handle errors in a safe way:

```c++
// Error handling #2:
#include <asmjit/asmjit.h>
#include <exception>

using namespace asmjit;

class AsmJitException : public std::exception {
public:
  AsmJitException(Error err, const char* message) noexcept
    : error(err),
    : message(message) {}

  Error error;
  std::string message;
}

class MyErrorHandler : public ErrorHandler {
public:
  // `origin` points to the `X86Assembler` instance (&a) in our case.
  virtual bool handleError(Error err, const char* message, void* origin) {
    throw AsmJitException(err, message);
  }
};

int main(int argc, char* argv[]) {
  JitRuntime rt;
  MyErrorHandler eh;

  X86Assembler a(&rt);
  a.setErrorHandler(&eh);

  try {
    // This will call `eh.handleError()`, which will throw.
    a.emit(X86Inst::kIdMov, x86::eax, x86::xmm4, x86::xmm1);
  }
  catch (const AsmJitException& ex) {
    printf("ASMJIT ERROR: 0x%0.8X [%s]\n", ex.error, ex.message.c_str());
  }
}
```

If C++ exceptions are not what you like or your project turns off them completely there is still a way of reducing the error handling to a minimum by using a standard `setjmp/longjmp` pair. AsmJit is exception-safe and doesn't use RAII for resource management internally, so you can just jump from the error handler without causing any side-effects or memory leaks. The following example demonstrates how to do that:

```c++
// Error handling #2:
#include <asmjit/asmjit.h>
#include <setjmp.h>

using namespace asmjit;

class MyErrorHandler : public ErrorHandler {
public:
  inline bool init() noexcept {
    return setjmp(_state) == 0;
  }

  virtual bool handleError(Error err, const char* message, void* origin) {
    longjmp(_state, 1);
  }

  jmp_buf _state;
};

int main(int argc, char* argv[]) {
  JitRuntime rt;
  MyErrorHandler eh;

  X86Assembler a(&rt);
  a.setErrorHandler(&eh);

  if (eh.init()) {
    // This will call `eh.handleError()`, which will call `longjmp()`.
    a.emit(X86Inst::kIdMov, x86::eax, x86::xmm4, x86::xmm1);
  }
  else {
    Error err = a.getLastError();
    printf("ASMJIT ERROR: 0x%0.8X [%s]\n", err, DebugUtils::errorAsString(err));
  }
}
```

### Using Constant Pool

To be documented.

### Code Injection

Code injection was one of key concepts of Compiler from the beginning. Compiler records all emitted instructions in a double-linked list which can be manipulated before `make()` is called. Any call to Compiler that adds instruction, function or anything else in fact manipulates this list by inserting nodes into it.

To manipulate the current cursor use Compiler's `getCursor()` and `setCursor()` methods. The following snippet demonstrates the proper way of code injection.

```c++
X86Compiler c(...);

X86Gp x = c.newI32("x");
X86Gp y = c.newI32("y");

AsmNode* here = c.getCursor();
c.mov(y, 2);

// Now, `here` can be used to inject something before `mov y, 2`. To inject
// something it's always good to remember the current cursor so it can be set
// back after the injecting is done. When `setCursor()` is called it returns
// the old cursor to be remembered.
AsmNode* prev = c.setCursor(here);
c.mov(x, 1);
c.setCursor(prev);
```

The resulting code would look like:

```
c.mov(x, 1);
c.mov(y, 2);
```

### TODO

...More documentation...

Support
-------

Please consider a donation if you use the project and would like to keep it active in the future.

  * [Donate by PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=QDRM6SRNG7378&lc=EN;&item_name=asmjit&currency_code=EUR)

Received From:

  * [PELock - Software copy protection and license key system](https://www.pelock.com)

Authors & Maintainers
---------------------

  * Petr Kobalicek <kobalicek.petr@gmail.com>
