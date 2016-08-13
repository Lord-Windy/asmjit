// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER) && defined(ASMJIT_BUILD_X86)

// [Dependencies]
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86regalloc_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::X86CCCall - Arg / Ret]
// ============================================================================

bool X86CCCall::_setArg(uint32_t i, const Operand_& op) noexcept {
  if ((i & ~kFuncArgHi) >= _x86Decl.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool X86CCCall::_setRet(uint32_t i, const Operand_& op) noexcept {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::X86Compiler - Construction / Destruction]
// ============================================================================

X86Compiler::X86Compiler(CodeHolder* code) noexcept : CodeCompiler() {
  if (code)
    code->attach(this);
}
X86Compiler::~X86Compiler() noexcept {}

// ============================================================================
// [asmjit::X86Compiler - Events]
// ============================================================================

Error X86Compiler::onAttach(CodeHolder* code) noexcept {
  if (code->getArchType() == Arch::kTypeX86) {
    ASMJIT_PROPAGATE(Base::onAttach(code));

    _nativeGpArray = x86OpData.gpd;
    _nativeGpReg = _nativeGpArray[0];
    return kErrorOk;
  }

  if (code->getArchType() == Arch::kTypeX64) {
    ASMJIT_PROPAGATE(Base::onAttach(code));

    _nativeGpArray = x86OpData.gpq;
    _nativeGpReg = _nativeGpArray[0];
    return kErrorOk;
  }

  return DebugUtils::errored(kErrorInvalidArch);
}

Error X86Compiler::onDetach(CodeHolder* code) noexcept {
  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::X86Compiler - Finalize]
// ============================================================================

Error X86Compiler::finalize() {
  if (_lastError) return _lastError;

  // Flush the global constant pool.
  if (_globalConstPool) {
    addNode(_globalConstPool);
    _globalConstPool = nullptr;
  }

  X86RAPass ra;
  Error err = ra.process(this, &_cbPassZone);

  _cbPassZone.reset();
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  // TODO: There must be possibility to attach more assemblers, this is not so nice.
  if (_code->_cgAsm) {
    return serialize(_code->_cgAsm);
  }
  else {
    X86Assembler a(_code);
    return serialize(&a);
  }
}

// ============================================================================
// [asmjit::X86Compiler - Inst]
// ============================================================================

Error X86Compiler::_emit(uint32_t instId, const Operand_& o0, const Operand_& o1, const Operand_& o2, const Operand_& o3) {
  uint32_t options = getOptions() | getGlobalOptions();
  const char* inlineComment = getInlineComment();

  uint32_t opCount = static_cast<uint32_t>(!o0.isNone()) +
                     static_cast<uint32_t>(!o1.isNone()) +
                     static_cast<uint32_t>(!o2.isNone()) +
                     static_cast<uint32_t>(!o3.isNone()) ;

  // Handle failure and rare cases first.
  const uint32_t kErrorsAndSpecialCases =
    kOptionMaybeFailureCase | // CodeEmitter in error state.
    kOptionStrictValidation | // Strict validation.
    kOptionHasOp4           | // Has 5th operand (o4, indexed from zero).
    kOptionHasOp5           ; // Has 6th operand (o5, indexed from zero).

  if (ASMJIT_UNLIKELY(options & kErrorsAndSpecialCases)) {
    // Don't do anything if we are in error state.
    if (_lastError) return _lastError;

    // Count 5th and 6th operands.
    if (options & kOptionHasOp4) opCount = 5;
    if (options & kOptionHasOp5) opCount = 6;

    // Strict validation.
    if (options & kOptionStrictValidation) {
      Operand opArray[] = {
        Operand(o0),
        Operand(o1),
        Operand(o2),
        Operand(o3),
        Operand(_op4),
        Operand(_op5)
      };

      Error err = X86Inst::validate(getArchType(), instId, options, _opExtra, opArray, opCount);
      if (err) return setLastError(err);

      // Clear it as it must be enabled explicitly on assembler side.
      options &= ~kOptionStrictValidation;
    }
  }

  resetOptions();
  resetInlineComment();

  // decide between `CBInst` and `CBJump`.
  if (Utils::inInterval<uint32_t>(instId, X86Inst::_kIdJbegin, X86Inst::_kIdJend)) {
    CBJump* node = _cbHeap.allocT<CBJump>(sizeof(CBJump) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBJump));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);
    if (opCount > 4) opArray[4].copyFrom(_op4);
    if (opCount > 5) opArray[5].copyFrom(_op5);
    new(node) CBJump(this, instId, options, opArray, opCount);

    CBLabel* jTarget = nullptr;
    if (!(options & kOptionUnfollow)) {
      if (opArray[0].isLabel()) {
        Error err = getCBLabel(&jTarget, static_cast<Label&>(opArray[0]));
        if (err) return setLastError(err);
      }
      else {
        options |= kOptionUnfollow;
      }
    }
    node->setOptions(options);

    node->orFlags(instId == X86Inst::kIdJmp ? CBNode::kFlagIsJmp | CBNode::kFlagIsTaken : CBNode::kFlagIsJcc);
    node->_target = jTarget;
    node->_jumpNext = nullptr;

    if (jTarget) {
      node->_jumpNext = static_cast<CBJump*>(jTarget->_from);
      jTarget->_from = node;
      jTarget->addNumRefs();
    }

    // The 'jmp' is always taken, conditional jump can contain hint, we detect it.
    if (instId == X86Inst::kIdJmp)
      node->orFlags(CBNode::kFlagIsTaken);
    else if (options & X86Inst::kOptionTaken)
      node->orFlags(CBNode::kFlagIsTaken);

    addNode(node);
    return kErrorOk;
  }
  else {
    CBInst* node = _cbHeap.allocT<CBInst>(sizeof(CBInst) + opCount * sizeof(Operand));
    Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(CBInst));

    if (ASMJIT_UNLIKELY(!node))
      return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    if (opCount > 0) opArray[0].copyFrom(o0);
    if (opCount > 1) opArray[1].copyFrom(o1);
    if (opCount > 2) opArray[2].copyFrom(o2);
    if (opCount > 3) opArray[3].copyFrom(o3);
    if (opCount > 4) opArray[4].copyFrom(_op4);
    if (opCount > 5) opArray[5].copyFrom(_op5);

    addNode(new(node) CBInst(this, instId, options, opArray, opCount));
    return kErrorOk;
  }
}

// ============================================================================
// [asmjit::X86Compiler - Func]
// ============================================================================

X86Func* X86Compiler::newFunc(const FuncSignature& sign) noexcept {
  Error err;

  X86Func* func = newNodeT<X86Func>();
  if (!func) goto _NoMemory;

  err = registerLabelNode(func);
  if (ASMJIT_UNLIKELY(err)) {
    // TODO: Calls setLastError, maybe rethink noexcept?
    setLastError(err);
    return nullptr;
  }

  // Create helper nodes.
  func->_end = newNodeT<CBSentinel>();
  func->_exitNode = newLabelNode();
  if (!func->_exitNode || !func->_end) goto _NoMemory;

  // Function prototype.
  err = func->_x86Decl.setSignature(sign);
  if (err != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // Function arguments stack size. Since function requires _argStackSize to be
  // set, we have to copy it from X86FuncDecl.
  func->_argStackSize = func->_x86Decl.getArgStackSize();
  func->_redZoneSize = static_cast<uint16_t>(func->_x86Decl._callConv.getRedZoneSize());
  func->_spillZoneSize = static_cast<uint16_t>(func->_x86Decl._callConv.getSpillZoneSize());

  // Expected/Required stack alignment.
  func->_naturalStackAlignment = _codeInfo.getStackAlignment();
  func->_requiredStackAlignment = 0;

  // Allocate space for function arguments.
  func->_args = nullptr;
  if (func->getArgCount() != 0) {
    func->_args = _cbHeap.allocT<VirtReg*>(func->getArgCount() * sizeof(VirtReg*));
    if (!func->_args) goto _NoMemory;

    ::memset(func->_args, 0, func->getArgCount() * sizeof(VirtReg*));
  }

  return func;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

X86Func* X86Compiler::addFunc(const FuncSignature& sign) {
  X86Func* func = newFunc(sign);

  if (!func) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  return static_cast<X86Func*>(addFunc(func));
}

CBSentinel* X86Compiler::endFunc() {
  X86Func* func = getFunc();
  if (!func) {
    // TODO:
    return nullptr;
  }

  // Add the local constant pool at the end of the function (if exist).
  setCursor(func->getExitNode());

  if (_localConstPool) {
    addNode(_localConstPool);
    _localConstPool = nullptr;
  }

  // Finalize.
  func->addFuncFlags(kFuncFlagIsFinished);
  _func = nullptr;

  setCursor(func->getEnd());
  return func->getEnd();
}

// ============================================================================
// [asmjit::X86Compiler - Ret]
// ============================================================================

CCFuncRet* X86Compiler::newRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newNodeT<CCFuncRet>(o0, o1);
  if (!node) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }
  return node;
}

CCFuncRet* X86Compiler::addRet(const Operand_& o0, const Operand_& o1) noexcept {
  CCFuncRet* node = newRet(o0, o1);
  if (!node) return nullptr;
  return static_cast<CCFuncRet*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Call]
// ============================================================================

X86CCCall* X86Compiler::newCall(const Operand_& o0, const FuncSignature& sign) noexcept {
  Error err;
  uint32_t nArgs;

  X86CCCall* node = _cbHeap.allocT<X86CCCall>(sizeof(X86CCCall) + sizeof(Operand));
  Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(X86CCCall));

  if (ASMJIT_UNLIKELY(!node))
    goto _NoMemory;

  opArray[0].copyFrom(o0);
  new (node) X86CCCall(this, X86Inst::kIdCall, 0, opArray, 1);

  if ((err = node->_x86Decl.setSignature(sign)) != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // If there are no arguments skip the allocation.
  if ((nArgs = sign.getArgCount()) == 0)
    return node;

  node->_args = static_cast<Operand*>(_cbHeap.alloc(nArgs * sizeof(Operand)));
  if (!node->_args) goto _NoMemory;

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

X86CCCall* X86Compiler::addCall(const Operand_& o0, const FuncSignature& sign) noexcept {
  X86CCCall* node = newCall(o0, sign);
  if (!node) return nullptr;
  return static_cast<X86CCCall*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Vars]
// ============================================================================

Error X86Compiler::setArg(uint32_t argIndex, const Reg& r) {
  X86Func* func = getFunc();

  if (!func)
    return setLastError(DebugUtils::errored(kErrorInvalidState));

  if (!isVirtRegValid(r))
    return setLastError(DebugUtils::errored(kErrorInvalidVirtId));

  VirtReg* vr = getVirtReg(r);
  func->setArg(argIndex, vr);

  return kErrorOk;
}

Error X86Compiler::_prepareTypeId(uint32_t& typeIdInOut, uint32_t& signatureOut) noexcept {
  // Zero this so it's clear in case we return an error.
  signatureOut = 0;

  uint32_t typeId = typeIdInOut;

  // Passed RegType instead of TypeId?
  if (typeId < 32)
    typeId = x86OpData.regTypeToTypeId[typeId];

  if (ASMJIT_UNLIKELY(!TypeId::isValid(typeId)))
    return DebugUtils::errored(kErrorInvalidTypeId);

  // First normalize architecture dependent types.
  if (TypeId::isAbstract(typeId)) {
    if (typeId == TypeId::kIntPtr)
      typeId = getGpSize() == 4 ? TypeId::kI32 : TypeId::kI64;
    else
      typeId = getGpSize() == 4 ? TypeId::kU32 : TypeId::kU64;
  }

  // Type size helps to construct all kinds of registers. If the size is zero
  // then the TypeId is invalid.
  uint32_t size = TypeId::sizeOf(typeId);
  if (ASMJIT_UNLIKELY(!size))
    return DebugUtils::errored(kErrorInvalidTypeId);

  if (ASMJIT_UNLIKELY(typeId == TypeId::kF80))
    return DebugUtils::errored(kErrorIllegalUseOfF80);

  uint32_t regType = 0;

  switch (typeId) {
    case TypeId::kI8:
    case TypeId::kU8:
      regType = X86Reg::kRegGpbLo;
      break;

    case TypeId::kI16:
    case TypeId::kU16:
      regType = X86Reg::kRegGpw;
      break;

    case TypeId::kI32:
    case TypeId::kU32:
      regType = X86Reg::kRegGpd;
      break;

    case TypeId::kI64:
    case TypeId::kU64:
      if (getGpSize() < 8)
        return DebugUtils::errored(kErrorIllegalUseOfGpq);

      regType = X86Reg::kRegGpq;
      break;

    // F32 and F64 are always promoted to use vector registers.
    case TypeId::kF32:
      typeId = TypeId::kF32x1;
      regType = X86Reg::kRegXmm;
      break;

    case TypeId::kF64:
      typeId = TypeId::kF64x1;
      regType = X86Reg::kRegXmm;
      break;

    // Mask registers {k}.
    case TypeId::kMask8:
    case TypeId::kMask16:
    case TypeId::kMask32:
    case TypeId::kMask64:
      regType = X86Reg::kRegK;
      break;

    // MMX registers.
    case TypeId::kMmx32:
    case TypeId::kMmx64:
      regType = X86Reg::kRegMm;
      break;

    // XMM|YMM|ZMM registers.
    default:
      if (size <= 16)
        regType = X86Reg::kRegXmm;
      else if (size == 32)
        regType = X86Reg::kRegYmm;
      else
        regType = X86Reg::kRegZmm;
      break;
  }

  typeIdInOut = typeId;
  signatureOut = x86OpData.regInfo[regType].signature;
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && ASMJIT_BUILD_X86
