// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86COMPILER_H
#define _ASMJIT_X86_X86COMPILER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/codecompiler.h"
#include "../base/simdtypes.h"
#include "../x86/x86assembler.h"
#include "../x86/x86func.h"
#include "../x86/x86misc.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class X86Func;
class X86CCCall;

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86Func]
// ============================================================================

//! X86/X64 function node.
class X86Func : public CCFunc {
public:
  ASMJIT_NONCOPYABLE(X86Func)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86Func` instance.
  ASMJIT_INLINE X86Func(CodeBuilder* cb) noexcept : CCFunc(cb) {
    _decl = &_x86Decl;
    _saveRestoreRegs.reset();

    _alignStackSize = 0;
    _alignedMemStackSize = 0;
    _pushPopStackSize = 0;
    _moveStackSize = 0;
    _extraStackSize = 0;

    _stackFrameRegIndex = kInvalidReg;
    _isStackFrameRegPreserved = false;

    for (uint32_t i = 0; i < ASMJIT_ARRAY_SIZE(_stackFrameCopyGpIndex); i++)
      _stackFrameCopyGpIndex[i] = static_cast<uint8_t>(kInvalidReg);
  }

  //! Destroy the `X86Func` instance (NEVER CALLED).
  ASMJIT_INLINE ~X86Func() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function declaration as `X86FuncDecl`.
  ASMJIT_INLINE X86FuncDecl* getDecl() const noexcept {
    return const_cast<X86FuncDecl*>(&_x86Decl);
  }

  //! Get argument.
  ASMJIT_INLINE VirtReg* getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _x86Decl.getArgCount());
    return static_cast<VirtReg*>(_args[i]);
  }

  //! Get registers which have to be saved in prolog/epilog.
  ASMJIT_INLINE uint32_t getSaveRestoreRegs(uint32_t kind) noexcept { return _saveRestoreRegs.get(kind); }

  //! Get stack size needed to align stack back to the nature alignment.
  ASMJIT_INLINE uint32_t getAlignStackSize() const noexcept { return _alignStackSize; }
  //! Set stack size needed to align stack back to the nature alignment.
  ASMJIT_INLINE void setAlignStackSize(uint32_t s) noexcept { _alignStackSize = s; }

  //! Get aligned stack size used by variables and memory allocated on the stack.
  ASMJIT_INLINE uint32_t getAlignedMemStackSize() const noexcept { return _alignedMemStackSize; }

  //! Get stack size used by push/pop sequences in prolog/epilog.
  ASMJIT_INLINE uint32_t getPushPopStackSize() const noexcept { return _pushPopStackSize; }
  //! Set stack size used by push/pop sequences in prolog/epilog.
  ASMJIT_INLINE void setPushPopStackSize(uint32_t s) noexcept { _pushPopStackSize = s; }

  //! Get stack size used by mov sequences in prolog/epilog.
  ASMJIT_INLINE uint32_t getMoveStackSize() const noexcept { return _moveStackSize; }
  //! Set stack size used by mov sequences in prolog/epilog.
  ASMJIT_INLINE void setMoveStackSize(uint32_t s) noexcept { _moveStackSize = s; }

  //! Get extra stack size.
  ASMJIT_INLINE uint32_t getExtraStackSize() const noexcept { return _extraStackSize; }
  //! Set extra stack size.
  ASMJIT_INLINE void setExtraStackSize(uint32_t s) noexcept { _extraStackSize  = s; }

  //! Get whether the function has stack frame register (only when the stack is misaligned).
  //!
  //! NOTE: Stack frame register can be used for both - aligning purposes or
  //! generating standard prolog/epilog sequence.
  ASMJIT_INLINE bool hasStackFrameReg() const noexcept { return _stackFrameRegIndex != kInvalidReg; }

  //! Get stack frame register index.
  //!
  //! NOTE: Used only when stack is misaligned.
  ASMJIT_INLINE uint32_t getStackFrameRegIndex() const noexcept { return _stackFrameRegIndex; }

  //! Get whether the stack frame register is preserved.
  //!
  //! NOTE: Used only when stack is misaligned.
  ASMJIT_INLINE bool isStackFrameRegPreserved() const noexcept { return static_cast<bool>(_isStackFrameRegPreserved); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! X86 function decl.
  X86FuncDecl _x86Decl;
  //! Registers which must be saved/restored in prolog/epilog.
  X86RegMask _saveRestoreRegs;

  //! Stack size needed to align function back to the nature alignment.
  uint32_t _alignStackSize;
  //! Like `_memStackSize`, but aligned.
  uint32_t _alignedMemStackSize;

  //! Stack required for push/pop in prolog/epilog (X86/X64 specific).
  uint32_t _pushPopStackSize;
  //! Stack required for movs in prolog/epilog (X86/X64 specific).
  uint32_t _moveStackSize;

  //! Stack required to put extra data (for example function arguments
  //! when manually aligning to requested alignment).
  uint32_t _extraStackSize;

  //! Stack frame register.
  uint8_t _stackFrameRegIndex;
  //! Whether the stack frame register is preserved.
  uint8_t _isStackFrameRegPreserved;
  //! Gp registers indexes that can be used to copy function arguments
  //! to a new location in case we are doing manual stack alignment.
  uint8_t _stackFrameCopyGpIndex[6];
};

// ============================================================================
// [asmjit::X86CCCall]
// ============================================================================

//! X86/X64 function-call node.
class X86CCCall : public CCCall {
public:
  ASMJIT_NONCOPYABLE(X86CCCall)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `X86CCCall` instance.
  ASMJIT_INLINE X86CCCall(CodeBuilder* cb, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : CCCall(cb, instId, options, opArray, opCount) {

    _decl = &_x86Decl;
    _usedArgs.reset();
  }

  //! Destroy the `X86CCCall` instance (NEVER CALLED).
  ASMJIT_INLINE ~X86CCCall() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the function prototype.
  ASMJIT_INLINE X86FuncDecl* getDecl() const noexcept {
    return const_cast<X86FuncDecl*>(&_x86Decl);
  }

  // --------------------------------------------------------------------------
  // [Signature]
  // --------------------------------------------------------------------------

  //! Set function signature.
  ASMJIT_INLINE Error setSignature(const FuncSignature& sign) noexcept {
    return _x86Decl.setSignature(sign);
  }

  // --------------------------------------------------------------------------
  // [Arg / Ret]
  // --------------------------------------------------------------------------

  //! Set argument at `i` to `op`.
  ASMJIT_API bool _setArg(uint32_t i, const Operand_& op) noexcept;
  //! Set return at `i` to `op`.
  ASMJIT_API bool _setRet(uint32_t i, const Operand_& op) noexcept;

  //! Set argument at `i` to `reg`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Reg& reg) noexcept { return _setArg(i, reg); }
  //! Set argument at `i` to `imm`.
  ASMJIT_INLINE bool setArg(uint32_t i, const Imm& imm) noexcept { return _setArg(i, imm); }

  //! Set return at `i` to `var`.
  ASMJIT_INLINE bool setRet(uint32_t i, const Reg& reg) noexcept { return _setRet(i, reg); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! X86 declaration.
  X86FuncDecl _x86Decl;
  //! Mask of all registers actually used to pass function arguments.
  //!
  //! NOTE: This bit-mask is not the same as `X86Func::_passed`. It contains
  //! only registers actually used to do the call while `X86Func::_passed`
  //! mask contains all registers for all possible signatures (defined by ABI).
  X86RegMask _usedArgs;
};

// ============================================================================
// [asmjit::X86Compiler]
// ============================================================================

//! Architecture-dependent \ref CodeCompiler targeting X86 and X64.
class ASMJIT_VIRTAPI X86Compiler
  : public CodeCompiler,
    public X86EmitterExplicitT<X86Compiler> {

public:
  ASMJIT_NONCOPYABLE(X86Compiler)
  typedef CodeCompiler Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a `X86Compiler` instance.
  ASMJIT_API X86Compiler(CodeHolder* code = nullptr) noexcept;
  //! Destroy the `X86Compiler` instance.
  ASMJIT_API ~X86Compiler() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API virtual Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Code-Generation]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) override;

  // -------------------------------------------------------------------------
  // [Finalize]
  // -------------------------------------------------------------------------

  ASMJIT_API virtual Error finalize() override;

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  //! Create a new `X86Func`.
  ASMJIT_API X86Func* newFunc(const FuncSignature& sign) noexcept;

  using CodeCompiler::addFunc;

  //! Add a new function.
  ASMJIT_API X86Func* addFunc(const FuncSignature& sign);

  //! Emit a sentinel that marks the end of the current function.
  ASMJIT_API CBSentinel* endFunc();

  //! Get the current function casted to \ref X86Func.
  //!
  //! This method can be called within `addFunc()` and `endFunc()` block to get
  //! current function you are working with. It's recommended to store `CCFunc`
  //! pointer returned by `addFunc<>` method, because this allows you in future
  //! implement function sections outside of the function itself.
  ASMJIT_INLINE X86Func* getFunc() const noexcept { return static_cast<X86Func*>(_func); }

  // --------------------------------------------------------------------------
  // [Ret]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncRet`.
  ASMJIT_API CCFuncRet* newRet(const Operand_& o0, const Operand_& o1) noexcept;
  //! Add a new `CCFuncRet`.
  ASMJIT_API CCFuncRet* addRet(const Operand_& o0, const Operand_& o1) noexcept;

  // --------------------------------------------------------------------------
  // [Call]
  // --------------------------------------------------------------------------

  //! Create a new `X86CCCall`.
  ASMJIT_API X86CCCall* newCall(const Operand_& o0, const FuncSignature& sign) noexcept;
  //! Add a new `X86CCCall`.
  ASMJIT_API X86CCCall* addCall(const Operand_& o0, const FuncSignature& sign) noexcept;

  // --------------------------------------------------------------------------
  // [Args]
  // --------------------------------------------------------------------------

  //! Set a function argument at `argIndex` to `reg`.
  ASMJIT_API Error setArg(uint32_t argIndex, const Reg& reg);

  // --------------------------------------------------------------------------
  // [VirtReg]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error _prepareTypeId(uint32_t& typeIdInOut, uint32_t& signatureOut) noexcept override;

#if !defined(ASMJIT_DISABLE_LOGGING)
#define ASMJIT_NEW_REG(OUT, PARAM, NAME_FMT)            \
  va_list ap;                                           \
  va_start(ap, NAME_FMT);                               \
  _newReg(OUT, PARAM, NAME_FMT, ap);                    \
  va_end(ap)
#else
#define ASMJIT_NEW_REG(OUT, PARAM, NAME_FMT)            \
  ASMJIT_UNUSED(NAME_FMT);                              \
  _newReg(OUT, PARAM, nullptr)
#endif

#define ASMJIT_NEW_REG_USER(FUNC, REG)                  \
  ASMJIT_INLINE REG FUNC(uint32_t typeId) {             \
    REG reg(NoInit);                                    \
    _newReg(reg, typeId, nullptr);                      \
    return reg;                                         \
  }                                                     \
                                                        \
  REG FUNC(uint32_t typeId, const char* nameFmt, ...) { \
    REG reg(NoInit);                                    \
    ASMJIT_NEW_REG(reg, typeId, nameFmt);               \
    return reg;                                         \
  }

#define ASMJIT_NEW_REG_AUTO(FUNC, REG, TYPE_ID)         \
  ASMJIT_INLINE REG FUNC() {                            \
    REG reg(NoInit);                                    \
    _newReg(reg, TYPE_ID, nullptr);                     \
    return reg;                                         \
  }                                                     \
                                                        \
  REG FUNC(const char* nameFmt, ...) {                  \
    REG reg(NoInit);                                    \
    ASMJIT_NEW_REG(reg, TYPE_ID, nameFmt);              \
    return reg;                                         \
  }

  template<typename RegT>
  RegT newSimilarReg(const RegT& ref, const char* nameFmt, ...) {
    RegT reg(NoInit);
    ASMJIT_NEW_REG(reg, ref, nameFmt);
    return reg;
  }

  ASMJIT_NEW_REG_USER(newReg    , X86Reg )
  ASMJIT_NEW_REG_USER(newGpReg  , X86Gp  )
  ASMJIT_NEW_REG_USER(newMmReg  , X86Mm  )
  ASMJIT_NEW_REG_USER(newKReg   , X86KReg)
  ASMJIT_NEW_REG_USER(newXmmReg , X86Xmm )
  ASMJIT_NEW_REG_USER(newYmmReg , X86Ymm )
  ASMJIT_NEW_REG_USER(newZmmReg , X86Zmm )

  ASMJIT_NEW_REG_AUTO(newI8     , X86Gp  , TypeId::kI8     )
  ASMJIT_NEW_REG_AUTO(newU8     , X86Gp  , TypeId::kU8     )
  ASMJIT_NEW_REG_AUTO(newI16    , X86Gp  , TypeId::kI16    )
  ASMJIT_NEW_REG_AUTO(newU16    , X86Gp  , TypeId::kU16    )
  ASMJIT_NEW_REG_AUTO(newI32    , X86Gp  , TypeId::kI32    )
  ASMJIT_NEW_REG_AUTO(newU32    , X86Gp  , TypeId::kU32    )
  ASMJIT_NEW_REG_AUTO(newI64    , X86Gp  , TypeId::kI64    )
  ASMJIT_NEW_REG_AUTO(newU64    , X86Gp  , TypeId::kU64    )
  ASMJIT_NEW_REG_AUTO(newInt8   , X86Gp  , TypeId::kI8     )
  ASMJIT_NEW_REG_AUTO(newUInt8  , X86Gp  , TypeId::kU8     )
  ASMJIT_NEW_REG_AUTO(newInt16  , X86Gp  , TypeId::kI16    )
  ASMJIT_NEW_REG_AUTO(newUInt16 , X86Gp  , TypeId::kU16    )
  ASMJIT_NEW_REG_AUTO(newInt32  , X86Gp  , TypeId::kI32    )
  ASMJIT_NEW_REG_AUTO(newUInt32 , X86Gp  , TypeId::kU32    )
  ASMJIT_NEW_REG_AUTO(newInt64  , X86Gp  , TypeId::kI64    )
  ASMJIT_NEW_REG_AUTO(newUInt64 , X86Gp  , TypeId::kU64    )
  ASMJIT_NEW_REG_AUTO(newIntPtr , X86Gp  , TypeId::kIntPtr )
  ASMJIT_NEW_REG_AUTO(newUIntPtr, X86Gp  , TypeId::kUIntPtr)

  ASMJIT_NEW_REG_AUTO(newGpb    , X86Gp  , TypeId::kU8     )
  ASMJIT_NEW_REG_AUTO(newGpw    , X86Gp  , TypeId::kU16    )
  ASMJIT_NEW_REG_AUTO(newGpd    , X86Gp  , TypeId::kU32    )
  ASMJIT_NEW_REG_AUTO(newGpq    , X86Gp  , TypeId::kU64    )
  ASMJIT_NEW_REG_AUTO(newGpz    , X86Gp  , TypeId::kUIntPtr)
  ASMJIT_NEW_REG_AUTO(newKb     , X86KReg, TypeId::kMask8  )
  ASMJIT_NEW_REG_AUTO(newKw     , X86KReg, TypeId::kMask16 )
  ASMJIT_NEW_REG_AUTO(newKd     , X86KReg, TypeId::kMask32 )
  ASMJIT_NEW_REG_AUTO(newKq     , X86KReg, TypeId::kMask64 )
  ASMJIT_NEW_REG_AUTO(newMm     , X86Mm  , TypeId::kMmx64  )
  ASMJIT_NEW_REG_AUTO(newXmm    , X86Xmm , TypeId::kI32x4  )
  ASMJIT_NEW_REG_AUTO(newXmmSs  , X86Xmm , TypeId::kF32x1  )
  ASMJIT_NEW_REG_AUTO(newXmmSd  , X86Xmm , TypeId::kF64x1  )
  ASMJIT_NEW_REG_AUTO(newXmmPs  , X86Xmm , TypeId::kF32x4  )
  ASMJIT_NEW_REG_AUTO(newXmmPd  , X86Xmm , TypeId::kF64x2  )
  ASMJIT_NEW_REG_AUTO(newYmm    , X86Ymm , TypeId::kI32x8  )
  ASMJIT_NEW_REG_AUTO(newYmmPs  , X86Ymm , TypeId::kF32x8  )
  ASMJIT_NEW_REG_AUTO(newYmmPd  , X86Ymm , TypeId::kF64x4  )
  ASMJIT_NEW_REG_AUTO(newZmm    , X86Zmm , TypeId::kI32x16 )
  ASMJIT_NEW_REG_AUTO(newZmmPs  , X86Zmm , TypeId::kF32x16 )
  ASMJIT_NEW_REG_AUTO(newZmmPd  , X86Zmm , TypeId::kF64x8  )

#undef ASMJIT_NEW_REG_AUTO
#undef ASMJIT_NEW_REG_USER
#undef ASMJIT_NEW_REG

  // --------------------------------------------------------------------------
  // [Stack]
  // --------------------------------------------------------------------------

  //! Create a new memory chunk allocated on the current function's stack.
  ASMJIT_INLINE X86Mem newStack(uint32_t size, uint32_t alignment, const char* name = nullptr) {
    X86Mem m(NoInit);
    _newStack(m, size, alignment, name);
    return m;
  }

  // --------------------------------------------------------------------------
  // [Const]
  // --------------------------------------------------------------------------

  //! Put data to a constant-pool and get a memory reference to it.
  ASMJIT_INLINE X86Mem newConst(uint32_t scope, const void* data, size_t size) {
    X86Mem m(NoInit);
    _newConst(m, scope, data, size);
    return m;
  }

  //! Put a BYTE `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newByteConst(uint32_t scope, uint8_t val) noexcept { return newConst(scope, &val, 1); }
  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newWordConst(uint32_t scope, uint16_t val) noexcept { return newConst(scope, &val, 2); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newDWordConst(uint32_t scope, uint32_t val) noexcept { return newConst(scope, &val, 4); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newQWordConst(uint32_t scope, uint64_t val) noexcept { return newConst(scope, &val, 8); }

  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt16Const(uint32_t scope, int16_t val) noexcept { return newConst(scope, &val, 2); }
  //! Put a WORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt16Const(uint32_t scope, uint16_t val) noexcept { return newConst(scope, &val, 2); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt32Const(uint32_t scope, int32_t val) noexcept { return newConst(scope, &val, 4); }
  //! Put a DWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt32Const(uint32_t scope, uint32_t val) noexcept { return newConst(scope, &val, 4); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newInt64Const(uint32_t scope, int64_t val) noexcept { return newConst(scope, &val, 8); }
  //! Put a QWORD `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newUInt64Const(uint32_t scope, uint64_t val) noexcept { return newConst(scope, &val, 8); }

  //! Put a SP-FP `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newFloatConst(uint32_t scope, float val) noexcept { return newConst(scope, &val, 4); }
  //! Put a DP-FP `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newDoubleConst(uint32_t scope, double val) noexcept { return newConst(scope, &val, 8); }

  //! Put a MMX `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newMmConst(uint32_t scope, const Data64& val) noexcept { return newConst(scope, &val, 8); }
  //! Put a XMM `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newXmmConst(uint32_t scope, const Data128& val) noexcept { return newConst(scope, &val, 16); }
  //! Put a YMM `val` to a constant-pool.
  ASMJIT_INLINE X86Mem newYmmConst(uint32_t scope, const Data256& val) noexcept { return newConst(scope, &val, 32); }

  // -------------------------------------------------------------------------
  // [Instruction Options]
  // -------------------------------------------------------------------------

  //! Force the compiler to not follow the conditional or unconditional jump.
  ASMJIT_INLINE X86Compiler& unfollow() noexcept { _options |= kOptionUnfollow; return *this; }
  //! Tell the compiler that the destination variable will be overwritten.
  ASMJIT_INLINE X86Compiler& overwrite() noexcept { _options |= kOptionOverwrite; return *this; }

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  //! Call a function.
  ASMJIT_INLINE X86CCCall* call(const X86Gp& dst, const FuncSignature& sign) { return addCall(dst, sign); }
  //! \overload
  ASMJIT_INLINE X86CCCall* call(const X86Mem& dst, const FuncSignature& sign) { return addCall(dst, sign); }
  //! \overload
  ASMJIT_INLINE X86CCCall* call(const Label& label, const FuncSignature& sign) { return addCall(label, sign); }
  //! \overload
  ASMJIT_INLINE X86CCCall* call(const Imm& dst, const FuncSignature& sign) { return addCall(dst, sign); }
  //! \overload
  ASMJIT_INLINE X86CCCall* call(uint64_t dst, const FuncSignature& sign) { return addCall(Imm(dst), sign); }

  //! Return.
  ASMJIT_INLINE CCFuncRet* ret() { return addRet(Operand(), Operand()); }
  //! \overload
  ASMJIT_INLINE CCFuncRet* ret(const X86Gp& o0) { return addRet(o0, Operand()); }
  //! \overload
  ASMJIT_INLINE CCFuncRet* ret(const X86Gp& o0, const X86Gp& o1) { return addRet(o0, o1); }
  //! \overload
  ASMJIT_INLINE CCFuncRet* ret(const X86Xmm& o0) { return addRet(o0, Operand()); }
  //! \overload
  ASMJIT_INLINE CCFuncRet* ret(const X86Xmm& o0, const X86Xmm& o1) { return addRet(o0, o1); }
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_X86_X86COMPILER_H
