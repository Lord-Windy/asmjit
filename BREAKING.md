2016-07-20
----------

Global `asmjit_cast<>` removed and introduced a more type-safe `asmjit::ptr_cast<>`, which can cast a function to `void*` (and vice-versa), but will refuse to cast a function to `void**`, for example. Just change `asmjit_cast` to `asmjit::ptr_cast` and everything should work as usual. As a consequence, the Runtime now contains a typesafe (templated) `add()` and `remove()` methods that accept a function type directly, no need to cast manually to `void*` and `void**`. If you use your own runtime rename your virtual methods from `add` to `_add` and from `release` to `_release` and enjoy the type-safe wrappers.

Merged virtual registers (known as variables or Vars) into registers themselves, making the interface simpler:

```c++
X86GpReg/X86GpVar merged to X86Gp
X86MmReg/X86MmVar merged to X86Mm
X86XmmReg/X86XmmVar merged to X86Xmm
X86YmmReg/X86YmmVar merged to X86Ymm
```

Operand API is mostly intact, omitting Var/Reg should fix most compile-time errors. There is now no difference between a register index and register id internally. If you ever used `reg.getRegIndex()` then use `reg.getId()` instead. Also renamed `isInitialized()` to `isValid()`.

  * There are much more changes, but they are mostly internal and keeping most operand methods compatible.
  * Added new functionality into `asmjit::x86` namespace related to operands.
  * X86Xmm/X86Ymm/X86Zmm register operands now inherit from X86Xyz.
  * Register class is now part of `Reg` operand, you can get it by using `reg.getRegClass()`.
  * Register class enum moved to `X86Reg`, `kX86RegClassGp` is now `X86Reg::kClassGp`.
  * Register type enum moved to `X86Reg`, `kX86RegTypeXmm` is now `X86Reg::kTypeXmm`.
  * Register index enum moved to `X86Gp`, `kX86RegIndexAx` is now `X86Gp::kIdAx`.
  * Segment index enum moved to `X86Seg`, `kX86SegFs` is now `X86Seg::kIdFs`.
  * If you used `asmjit::noOperand` for any reason change it to `Operand()`.

Removed `Logger::Style` and `uint32_t style` parameter in Logging API. It was never used for anything so it was removed.

Refactored instruction database, moved many enums related to instructions into `X86Inst`. Also some instructions were wrong (having wrong signature in Assembler and Compiler) and were fixed.

```c++
X86InstInfo             renamed to X86Inst
kX86InstIdSomething     renamed to X86Inst::kIdSomething
kX86InstOptionSomething renamed to X86Inst::kOptionSomething
kX86CondSomething       renamed to X86Inst::kCondSomething
kX86CmpSomething        renamed to X86Inst::kCmpSomething
kX86VCmpSomething       renamed to X86Inst::kVCmpSomething
kX86PrefetchSomething   renamed to X86Inst::kPrefetchSomething
```

There is a new `AsmEmitter` base class that defines assembler building blocks that are implemented by `Assembler` and `AsmBuilder`. `Compiler` is now based on `AsmBuilder` and shares its instruction storage functionality. API didn't change, just base classes and new functionality has been added. It's now possible to serialize code for further processing by using `AsmBuilder`.

Renamed compile-time macro `ASMJIT_DISABLE_LOGGER` to `ASMJIT_DISABLE_LOGGING`. There is a new `Formatter` class which is also disabled with this option.



2016-03-21
----------

CpuInfo has been completely redesigned. It now supports multiple CPUs without having to inherit it to support a specific architecture. Also all CpuInfo-related constants have been moved to CpuInfo.

Change:

```c++
const X86CpuInfo* cpu = X86CpuInfo::getHost();
cpu->hasFeature(kX86CpuFeatureSSE4_1);
```

to

```c++
const CpuInfo& cpu = CpuInfo::getHost();
cpu.hasFeature(CpuInfo::kX86FeatureSSE4_1);
```

The whole code-base now uses `noexcept` keyword to inform API users that these functions won't throw an exception. Moreover, the possibility to throw exception through `ErrorHandler` has been removed as it seems that nobody has ever used it. `Assembler::emit()` and friends are still not marked as `noexcept` in case this decision is taken back. If there is no complaint even `emit()` functions will be marked `noexcept` in the near future.

2015-12-07
----------

Compiler now attaches to Assembler. This change was required to create resource sharing where Assembler is the central part and Compiler is a "high-level" part that serializes to it. It's an incremental work to implement sections and to allow code generators to create executables and libraries.

Also, Compiler has no longer Logger interface, it uses Assembler's one after it's attached to it.

```c++
JitRuntime runtime;
X86Compiler c(&runtime);

// ... code generation ...

void* p = c.make();
```

to

```c++
JitRuntime runtime;
X86Assembler a(&runtime);
X86Compiler c(&a);

// ... code generation ...

c.finalize();
void* p = a.make();
```

All nodes were prefixed with HL, except for platform-specific nodes, change:

```c++
Node        -> HLNode
FuncNode    -> HLFunc
X86FuncNode -> X86Func
X86CallNode -> X86Call
```

`FuncConv` renamed to `CallConv` and is now part of a function prototype, change:

```c++
compiler.addFunc(kFuncConvHost, FuncBuilder0<Void>());
```

to

```c++
compiler.addFunc(FuncBuilder0<Void>(kCallConvHost));
```

Operand constructors that accept Assembler or Compiler are deprecated. Variables can now be created by using handy shortcuts like newInt32(), newIntPtr(), newXmmPd(), etc... Change:

```c++
X86Compiler c(...);
Label L(c);
X86GpVar x(c, kVarTypeIntPtr, "x");
```

to

```c++
X86Compiler c(...);
Label L = c.newLabel();
X86GpVar x = c.newIntPtr("x");
```

