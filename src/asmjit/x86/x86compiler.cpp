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
#include "../x86/x86compilercontext_p.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Debug]
// ============================================================================

#if !defined(ASMJIT_DEBUG)
#define ASMJIT_ASSERT_OPERAND(op) \
  do {} while(0)
#else
#define ASMJIT_ASSERT_OPERAND(op) \
  do { \
    if (op.isReg() || op.isLabel()) { \
      ASMJIT_ASSERT(op.getId() != kInvalidValue); \
    } \
  } while(0)
#endif

// ============================================================================
// [asmjit::X86TypeData]
// ============================================================================

#define INVALID_TYPE() {{ 0, 0, 0, 0 }}
#define SIGNATURE_OF(REG_TYPE) {{          \
  uint8_t(Operand::kOpReg),                \
  uint8_t(REG_TYPE),                       \
  uint8_t(X86RegTraits<REG_TYPE>::kClass), \
  uint8_t(X86RegTraits<REG_TYPE>::kSize)   \
}}

#define F(FLAG) VirtType::kFlag##FLAG    // Type-Flags.
#define U kInvalidValue                  // Unsupported.

const X86TypeData _x86TypeData = {
  {// Signature                       , Type-Id              , Sz, Type-Flags        , Type-Name
    { SIGNATURE_OF(X86Reg::kRegGpbLo), VirtType::kIdI8      , 1 , 0                 , "gp.i8"   }, // #0
    { SIGNATURE_OF(X86Reg::kRegGpbLo), VirtType::kIdU8      , 1 , 0                 , "gp.u8"   }, // #1
    { SIGNATURE_OF(X86Reg::kRegGpw)  , VirtType::kIdI16     , 2 , 0                 , "gp.i16"  }, // #2
    { SIGNATURE_OF(X86Reg::kRegGpw)  , VirtType::kIdU16     , 2 , 0                 , "gp.u16"  }, // #3
    { SIGNATURE_OF(X86Reg::kRegGpd)  , VirtType::kIdI32     , 4 , 0                 , "gp.i32"  }, // #4
    { SIGNATURE_OF(X86Reg::kRegGpd)  , VirtType::kIdU32     , 4 , 0                 , "gp.u32"  }, // #5
    { SIGNATURE_OF(X86Reg::kRegGpq)  , VirtType::kIdI64     , 8 , 0                 , "gp.i64"  }, // #6
    { SIGNATURE_OF(X86Reg::kRegGpq)  , VirtType::kIdU64     , 8 , 0                 , "gp.u64"  }, // #7
    { INVALID_TYPE()                 , kInvalidValue        , 0 , 0                 , ""        }, // #8
    { INVALID_TYPE()                 , kInvalidValue        , 0 , 0                 , ""        }, // #9
  //{ INVALID_TYPE()                 , kInvalidValue        , 0 , 0                 , ""        }, // #10
  //{ INVALID_TYPE()                 , kInvalidValue        , 0 , 0                 , ""        }, // #11
    { SIGNATURE_OF(X86Reg::kRegFp)   , VirtType::kIdF32     , 4 , 0                 , "fp"      }, // #10
    { SIGNATURE_OF(X86Reg::kRegFp)   , VirtType::kIdF64     , 8 , 0                 , "fp"      }, // #11
    { SIGNATURE_OF(X86Reg::kRegK)    , VirtType::kIdX86K    , 8 , 0                 , "k"       }, // #12
    { SIGNATURE_OF(X86Reg::kRegMm)   , VirtType::kIdX86Mm   , 8 , F(Vector)         , "mm"      }, // #13
    { SIGNATURE_OF(X86Reg::kRegXmm)  , VirtType::kIdX86Xmm  , 16, F(Vector)         , "xmm"     }, // #14
    { SIGNATURE_OF(X86Reg::kRegXmm)  , VirtType::kIdX86XmmSs, 4 , 0         | F(F32), "xmm.ss"  }, // #15
    { SIGNATURE_OF(X86Reg::kRegXmm)  , VirtType::kIdX86XmmSd, 8 , 0         | F(F64), "xmm.sd"  }, // #16
    { SIGNATURE_OF(X86Reg::kRegXmm)  , VirtType::kIdX86XmmPs, 16, F(Vector) | F(F32), "xmm.ps"  }, // #17
    { SIGNATURE_OF(X86Reg::kRegXmm)  , VirtType::kIdX86XmmPd, 16, F(Vector) | F(F64), "xmm.pd"  }, // #18
    { SIGNATURE_OF(X86Reg::kRegYmm)  , VirtType::kIdX86Ymm  , 32, F(Vector)         , "ymm"     }, // #19
    { SIGNATURE_OF(X86Reg::kRegYmm)  , VirtType::kIdX86YmmPs, 32, F(Vector) | F(F32), "ymm.ps"  }, // #20
    { SIGNATURE_OF(X86Reg::kRegYmm)  , VirtType::kIdX86YmmPd, 32, F(Vector) | F(F64), "ymm.pd"  }, // #21
    { SIGNATURE_OF(X86Reg::kRegZmm)  , VirtType::kIdX86Zmm  , 64, F(Vector)         , "zmm"     }, // #22
    { SIGNATURE_OF(X86Reg::kRegZmm)  , VirtType::kIdX86ZmmPs, 64, F(Vector) | F(F32), "zmm.ps"  }, // #23
    { SIGNATURE_OF(X86Reg::kRegZmm)  , VirtType::kIdX86ZmmPd, 64, F(Vector) | F(F64), "zmm.pd"  }  // #24
  },

  //#0, #1, #2, #3, #4, #5, #6, #7, #8, #9,#10,#11,#12,#13,#14,#15,#16,#17,#18,#19,#20,#21,#22,#23,#24
  //I8, U8,I16,U16,I32,U32,I64,U64,PXX,UXX,F32,F64, K , MM,XMM,XSS,XSD,XPS,XPD,YMM,YPS,YPD,ZMM,ZPS,ZPD
  { 0 , 1 , 2 , 3 , 4 , 5 , U , U , 4 , 5 , 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 }, // IdMapX86.
  { 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 6 , 7 , 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 }  // IdMapX64.
};

#undef U
#undef F

#undef INVALID_TYPE
#undef SIGNATURE_OF

// ============================================================================
// [asmjit::X86CallNode - Arg / Ret]
// ============================================================================

bool X86CallNode::_setArg(uint32_t i, const Operand_& op) noexcept {
  if ((i & ~kFuncArgHi) >= _x86Decl.getNumArgs())
    return false;

  _args[i] = op;
  return true;
}

bool X86CallNode::_setRet(uint32_t i, const Operand_& op) noexcept {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::X86Compiler - Construction / Destruction]
// ============================================================================

X86Compiler::X86Compiler(CodeHolder* code) noexcept : CodeCompiler() {
  zax.reset();
  zcx.reset();
  zdx.reset();
  zbx.reset();
  zsp.reset();
  zbp.reset();
  zsi.reset();
  zdi.reset();

  if (code)
    code->attach(this);
}
X86Compiler::~X86Compiler() noexcept {}

// ============================================================================
// [asmjit::X86Compiler - Events]
// ============================================================================

Error X86Compiler::onAttach(CodeHolder* code) noexcept {
  uint32_t archId = code->getArchId();

  const uint32_t* idMap = nullptr;
  const X86Gp* gpRegs = nullptr;

  switch (archId) {
    case ArchInfo::kIdX86: idMap = _x86TypeData.idMapX86; gpRegs = x86OpData.gpd; break;
    case ArchInfo::kIdX64: idMap = _x86TypeData.idMapX64; gpRegs = x86OpData.gpq; break;
    default:
      return DebugUtils::errored(kErrorInvalidArch);
  }

  _archInfo = code->getArchInfo();
  _typeIdMap = idMap;
  _finalized = false;

  ::memcpy(&zax, gpRegs, sizeof(Operand) * 8);
  return Base::onAttach(code);
}

Error X86Compiler::onDetach(CodeHolder* code) noexcept {
  for (uint32_t i = 0; i < 8; i++)
    (static_cast<Operand*>(&zax))[i].reset();
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

  CBNode* node = _firstNode;
  if (!node) return kErrorOk;

  Error err = kErrorOk;
  X86Context ctx(this);

  // Find all functions and use the `X86Context` to translate/emit them.
  do {
    _resetTokenGenerator();
    if (node->getType() == CBNode::kNodeFunc) {
      X86FuncNode* func = static_cast<X86FuncNode*>(node);
      node = func->getEnd();

      err = ctx.compile(func);
      if (err != kErrorOk) break;
    }

    do {
      node = node->getNext();
    } while (node && node->getType() != CBNode::kNodeFunc);

    ctx.cleanup();
    ctx.reset(false);

    if (err != kErrorOk)
      break;
  } while (node);

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

      Error err = X86Inst::validate(getArchId(), instId, options, _opMask, opArray, opCount);
      if (err) return setLastError(err);

      // Clear it as it must be enabled explicitly on assembler side.
      options &= ~kOptionStrictValidation;
    }
  }

  resetOptions();
  resetInlineComment();

  // decide between `CBInst` and `CBJump`.
  if (Utils::inInterval<uint32_t>(instId, X86Inst::_kIdJbegin, X86Inst::_kIdJend)) {
    CBJump* node = _nodeAllocator.allocT<CBJump>(sizeof(CBJump) + opCount * sizeof(Operand));
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
    CBInst* node = _nodeAllocator.allocT<CBInst>(sizeof(CBInst) + opCount * sizeof(Operand));
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

X86FuncNode* X86Compiler::newFunc(const FuncPrototype& p) noexcept {
  Error err;

  X86FuncNode* func = newNodeT<X86FuncNode>();
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
  err = func->_x86Decl.setPrototype(p);
  if (err != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // Function arguments stack size. Since function requires _argStackSize to be
  // set, we have to copy it from X86FuncDecl.
  func->_argStackSize = func->_x86Decl.getArgStackSize();
  func->_redZoneSize = static_cast<uint16_t>(func->_x86Decl.getRedZoneSize());
  func->_spillZoneSize = static_cast<uint16_t>(func->_x86Decl.getSpillZoneSize());

  // Expected/Required stack alignment.
  func->_naturalStackAlignment = _archInfo.getNaturalStackAlignment();
  func->_requiredStackAlignment = 0;

  // Allocate space for function arguments.
  func->_args = nullptr;
  if (func->getNumArgs() != 0) {
    func->_args = _nodeAllocator.allocT<VirtReg*>(func->getNumArgs() * sizeof(VirtReg*));
    if (!func->_args) goto _NoMemory;

    ::memset(func->_args, 0, func->getNumArgs() * sizeof(VirtReg*));
  }

  return func;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

X86FuncNode* X86Compiler::addFunc(const FuncPrototype& p) {
  X86FuncNode* func = newFunc(p);

  if (!func) {
    setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    return nullptr;
  }

  return static_cast<X86FuncNode*>(addFunc(func));
}

CBSentinel* X86Compiler::endFunc() {
  X86FuncNode* func = getFunc();
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

X86CallNode* X86Compiler::newCall(const Operand_& o0, const FuncPrototype& p) noexcept {
  Error err;
  uint32_t nArgs;

  X86CallNode* node = _nodeAllocator.allocT<X86CallNode>(sizeof(X86CallNode) + sizeof(Operand));
  Operand* opArray = reinterpret_cast<Operand*>(reinterpret_cast<uint8_t*>(node) + sizeof(X86CallNode));

  if (ASMJIT_UNLIKELY(!node))
    goto _NoMemory;

  opArray[0].copyFrom(o0);
  new (node) X86CallNode(this, X86Inst::kIdCall, 0, opArray, 1);

  if ((err = node->_x86Decl.setPrototype(p)) != kErrorOk) {
    setLastError(err);
    return nullptr;
  }

  // If there are no arguments skip the allocation.
  if ((nArgs = p.getNumArgs()) == 0)
    return node;

  node->_args = static_cast<Operand*>(_nodeAllocator.alloc(nArgs * sizeof(Operand)));
  if (!node->_args) goto _NoMemory;

  ::memset(node->_args, 0, nArgs * sizeof(Operand));
  return node;

_NoMemory:
  setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  return nullptr;
}

X86CallNode* X86Compiler::addCall(const Operand_& o0, const FuncPrototype& p) noexcept {
  X86CallNode* node = newCall(o0, p);
  if (!node) return nullptr;
  return static_cast<X86CallNode*>(addNode(node));
}

// ============================================================================
// [asmjit::X86Compiler - Vars]
// ============================================================================

Error X86Compiler::setArg(uint32_t argIndex, const Reg& r) {
  X86FuncNode* func = getFunc();

  if (!func)
    return setLastError(DebugUtils::errored(kErrorInvalidState));

  if (!isVirtRegValid(r))
    return setLastError(DebugUtils::errored(kErrorInvalidVirtId));

  VirtReg* vr = getVirtReg(r);
  func->setArg(argIndex, vr);

  return kErrorOk;
}

Error X86Compiler::_newReg(Reg& reg, uint32_t typeId, const char* name) {
  ASMJIT_ASSERT(typeId < VirtType::kIdCount);
  typeId = _typeIdMap[typeId];

  // There is currently only one case this could happen - an attempt to
  // use a 64-bit register (GPQ) when generating for a 32-bit target.
  if (!VirtType::isValidTypeId(typeId)) {
    reg.reset();
    return setLastError(DebugUtils::errored(kErrorIllegalUseOfGpq));
  }

  const VirtType& typeInfo = _x86TypeData.typeInfo[typeId];
  VirtReg* vreg = newVirtReg(typeInfo, name);

  if (!vreg) {
    static_cast<X86Reg&>(reg).reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  reg._initReg(typeInfo.getSignature(), vreg->getId());
  reg._reg.typeId = typeId;
  return kErrorOk;
}

Error X86Compiler::_newReg(Reg& r, uint32_t typeId, const char* fmt, va_list ap) {
  char name[256];

  vsnprintf(name, ASMJIT_ARRAY_SIZE(name), fmt, ap);
  name[ASMJIT_ARRAY_SIZE(name) - 1] = '\0';
  return _newReg(r, typeId, name);
}

// ============================================================================
// [asmjit::X86Compiler - Stack]
// ============================================================================

Error X86Compiler::_newStack(Mem& m, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment > 64)
    alignment = 64;

  VirtType typeInfo = { {{ 0, 0, 0, 0 }}, kInvalidValue, 0, 0, "" };
  VirtReg* vreg = newVirtReg(typeInfo, name);

  if (!vreg) {
    m.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  vreg->_size = size;
  vreg->_isStack = true;
  vreg->_alignment = static_cast<uint8_t>(alignment);

  // Set the memory operand to GPD/GPQ and its ID to vreg.
  m = X86Mem(Init, zax.getRegType(), vreg->getId(), Reg::kRegNone, kInvalidValue, 0, 0, Mem::kFlagIsRegHome);

  return kErrorOk;
}

// ============================================================================
// [asmjit::X86Compiler - Const]
// ============================================================================

Error X86Compiler::_newConst(Mem& m, uint32_t scope, const void* data, size_t size) {
  CBConstPool** pPool;
  if (scope == kConstScopeLocal)
    pPool = &_localConstPool;
  else if (scope == kConstScopeGlobal)
    pPool = &_globalConstPool;
  else
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (!*pPool && !(*pPool = newConstPool()))
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  CBConstPool* pool = *pPool;
  size_t off;

  Error err = pool->add(data, size, off);
  if (err != kErrorOk) return setLastError(err);

  static_cast<X86Mem&>(m) = x86::ptr(pool->getLabel(), static_cast<int32_t>(off), static_cast<uint32_t>(size));
  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER && ASMJIT_BUILD_X86
