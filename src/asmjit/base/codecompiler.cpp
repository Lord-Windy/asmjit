// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Guard]
#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codecompiler.h"
#include "../base/cpuinfo.h"
#include "../base/logging.h"
#include "../base/regalloc_p.h"
#include "../base/utils.h"
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Constants]
// ============================================================================

static const char noName[1] = { '\0' };

// ============================================================================
// [asmjit::CCFuncCall - Arg / Ret]
// ============================================================================

bool CCFuncCall::_setArg(uint32_t i, const Operand_& op) noexcept {
  if ((i & ~kFuncArgHi) >= _decl.getArgCount())
    return false;

  _args[i] = op;
  return true;
}

bool CCFuncCall::_setRet(uint32_t i, const Operand_& op) noexcept {
  if (i >= 2)
    return false;

  _ret[i] = op;
  return true;
}

// ============================================================================
// [asmjit::CodeCompiler - Construction / Destruction]
// ============================================================================

CodeCompiler::CodeCompiler() noexcept
  : CodeBuilder(),
    _func(nullptr),
    _vRegZone(4096 - Zone::kZoneOverhead),
    _vRegArray(&_cbHeap),
    _localConstPool(nullptr),
    _globalConstPool(nullptr) {

  _type = kTypeCompiler;
}
CodeCompiler::~CodeCompiler() noexcept {}

// ============================================================================
// [asmjit::CodeCompiler - Events]
// ============================================================================

Error CodeCompiler::onAttach(CodeHolder* code) noexcept {
  return Base::onAttach(code);
}

Error CodeCompiler::onDetach(CodeHolder* code) noexcept {
  _func = nullptr;

  _localConstPool = nullptr;
  _globalConstPool = nullptr;

  _vRegArray.reset(&_cbHeap);
  _vRegZone.reset(false);

  return Base::onDetach(code);
}

// ============================================================================
// [asmjit::CodeCompiler - Node-Factory]
// ============================================================================

CCHint* CodeCompiler::newHintNode(Reg& r, uint32_t hint, uint32_t value) noexcept {
  if (!r.isVirtReg()) return nullptr;

  VirtReg* vr = getVirtReg(r);
  return newNodeT<CCHint>(vr, hint, value);
}

// ============================================================================
// [asmjit::CodeCompiler - Func]
// ============================================================================

CCFunc* CodeCompiler::addFunc(CCFunc* func) {
  ASMJIT_ASSERT(_func == nullptr);
  _func = func;

  addNode(func);                 // Function node.
  CBNode* cursor = getCursor();  // {CURSOR}.
  addNode(func->getExitNode());  // Function exit label.
  addNode(func->getEnd());       // Function end marker.

  _setCursor(cursor);
  return func;
}

// ============================================================================
// [asmjit::CodeCompiler - Hint]
// ============================================================================

Error CodeCompiler::_hint(Reg& r, uint32_t hint, uint32_t value) {
  if (!r.isVirtReg()) return kErrorOk;

  CCHint* node = newHintNode(r, hint, value);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

// ============================================================================
// [asmjit::CodeCompiler - Vars]
// ============================================================================

VirtReg* CodeCompiler::newVirtReg(uint32_t typeId, uint32_t signature, const char* name) noexcept {
  size_t index = _vRegArray.getLength();
  if (ASMJIT_UNLIKELY(index > Operand::kPackedIdCount))
    return nullptr;

  VirtReg* vreg;
  if (_vRegArray.willGrow(1) != kErrorOk || !(vreg = _vRegZone.allocZeroedT<VirtReg>()))
    return nullptr;

  vreg->_id = Operand::packId(static_cast<uint32_t>(index));
  vreg->_regInfo.signature = signature;
  vreg->_name = noName;

#if !defined(ASMJIT_DISABLE_LOGGING)
  if (name && name[0] != '\0')
    vreg->_name = static_cast<char*>(_cbDataZone.dup(name, ::strlen(name), true));
#endif // !ASMJIT_DISABLE_LOGGING

  vreg->_size = TypeId::sizeOf(typeId);
  vreg->_typeId = typeId;
  vreg->_alignment = static_cast<uint8_t>(Utils::iMin<uint32_t>(vreg->_size, 64));
  vreg->_priority = 10;

  // The following are only used by `RAPass`.
  vreg->_raId = kInvalidValue;
  vreg->_state = VirtReg::kStateNone;
  vreg->_physId = kInvalidReg;

  _vRegArray.appendUnsafe(vreg);
  return vreg;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* name) {
  uint32_t signature = 0;

  Error err = _prepareTypeId(typeId, signature);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vreg = newVirtReg(typeId, signature, name);
  if (ASMJIT_UNLIKELY(!vreg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(signature, vreg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, uint32_t typeId, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, typeId, sb.getData());
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* name) {
  uint32_t typeId = ref.getRegType();
  uint32_t signature = 0;

  Error err = _prepareTypeId(typeId, signature);
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  VirtReg* vreg = newVirtReg(typeId, signature, name);
  if (ASMJIT_UNLIKELY(!vreg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  out._initReg(signature, vreg->getId());
  return kErrorOk;
}

Error CodeCompiler::_newReg(Reg& out, const Reg& ref, const char* nameFmt, va_list ap) {
  StringBuilderTmp<256> sb;
  sb.appendFormatVA(nameFmt, ap);
  return _newReg(out, ref, sb.getData());
}

Error CodeCompiler::_newStack(Mem& out, uint32_t size, uint32_t alignment, const char* name) {
  if (size == 0)
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment == 0) alignment = 1;
  if (!Utils::isPowerOf2(alignment))
    return setLastError(DebugUtils::errored(kErrorInvalidArgument));

  if (alignment > 64) alignment = 64;

  VirtReg* vreg = newVirtReg(0, 0, name);
  if (ASMJIT_UNLIKELY(!vreg)) {
    out.reset();
    return setLastError(DebugUtils::errored(kErrorNoHeapMemory));
  }

  vreg->_size = size;
  vreg->_isStack = true;
  vreg->_alignment = static_cast<uint8_t>(alignment);

  // Set the memory operand to GPD/GPQ and its id to VirtReg.
  out = Mem(Init, _nativeGpReg.getRegType(), vreg->getId(), Reg::kRegNone, kInvalidValue, 0, 0, Mem::kFlagRegHome);
  return kErrorOk;
}

Error CodeCompiler::_newConst(Mem& out, uint32_t scope, const void* data, size_t size) {
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
  if (ASMJIT_UNLIKELY(err)) return setLastError(err);

  out = Mem(Init,
    Label::kLabelTag,             // Base type.
    pool->getId(),                // Base id.
    0,                            // Index type.
    kInvalidValue,                // Index id.
    static_cast<int32_t>(off),    // Offset.
    static_cast<uint32_t>(size),  // Size.
    0);                           // Flags.
  return kErrorOk;
}

Error CodeCompiler::alloc(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, kInvalidValue);
}

Error CodeCompiler::alloc(Reg& reg, uint32_t physId) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, physId);
}

Error CodeCompiler::alloc(Reg& reg, const Reg& physReg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintAlloc, physReg.getId());
}

Error CodeCompiler::save(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintSave, kInvalidValue);
}

Error CodeCompiler::spill(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintSpill, kInvalidValue);
}

Error CodeCompiler::unuse(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, CCHint::kHintUnuse, kInvalidValue);
}

uint32_t CodeCompiler::getPriority(Reg& reg) const {
  if (!reg.isVirtReg()) return 0;
  return getVirtRegById(reg.getId())->getPriority();
}

void CodeCompiler::setPriority(Reg& reg, uint32_t priority) {
  if (!reg.isVirtReg()) return;
  if (priority > 255) priority = 255;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (vreg) vreg->_priority = static_cast<uint8_t>(priority);
}

bool CodeCompiler::getSaveOnUnuse(Reg& reg) const {
  if (!reg.isVirtReg()) return false;

  VirtReg* vreg = getVirtRegById(reg.getId());
  return static_cast<bool>(vreg->_saveOnUnuse);
}

void CodeCompiler::setSaveOnUnuse(Reg& reg, bool value) {
  if (!reg.isVirtReg()) return;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (!vreg) return;

  vreg->_saveOnUnuse = value;
}

void CodeCompiler::rename(Reg& reg, const char* fmt, ...) {
  if (!reg.isVirtReg()) return;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (!vreg) return;

  vreg->_name = noName;
  if (fmt && fmt[0] != '\0') {
    char buf[64];

    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf), fmt, ap);
    buf[ASMJIT_ARRAY_SIZE(buf) - 1] = '\0';

    vreg->_name = static_cast<char*>(_cbDataZone.dup(buf, ::strlen(buf), true));
    va_end(ap);
  }
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
