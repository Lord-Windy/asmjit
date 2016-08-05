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
enum { kCompilerDefaultLookAhead = 64 };

// ============================================================================
// [asmjit::CodeCompiler - Construction / Destruction]
// ============================================================================

CodeCompiler::CodeCompiler() noexcept
  : CodeBuilder(),
    _maxLookAhead(kCompilerDefaultLookAhead),
    _typeIdMap(nullptr),
    _func(nullptr),
    _vRegAllocator(4096 - Zone::kZoneOverhead),
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
  _maxLookAhead = kCompilerDefaultLookAhead;
  _func = nullptr;

  _localConstPool = nullptr;
  _globalConstPool = nullptr;

  _vRegAllocator.reset(false);
  _vRegArray.reset(false);

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
  CBNode* cursor = getCursor(); // {CURSOR}.
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

VirtReg* CodeCompiler::newVirtReg(const VirtType& typeInfo, const char* name) noexcept {
  size_t index = _vRegArray.getLength();
  if (index <= Operand::kPackedIdCount) {
    VirtReg* vreg;
    if (_vRegArray.willGrow(1) != kErrorOk || !(vreg = _vRegAllocator.allocT<VirtReg>()))
      return nullptr;

    vreg->_id = Operand::packId(static_cast<uint32_t>(index));
    vreg->_regInfo.signature = typeInfo.getSignature();

    vreg->_name = noName;
#if !defined(ASMJIT_DISABLE_LOGGING)
    if (name && name[0] != '\0')
      vreg->_name = _dataAllocator.sdup(name);
#endif // !ASMJIT_DISABLE_LOGGING

    vreg->_typeId = static_cast<uint8_t>(typeInfo.getTypeId());
    vreg->_size = typeInfo.getTypeSize();
    vreg->_alignment = static_cast<uint8_t>(Utils::iMin<uint32_t>(typeInfo.getTypeSize(), 64));
    vreg->_priority = 10;
    vreg->_isStack = false;
    vreg->_isMemArg = false;
    vreg->_isMaterialized = false;
    vreg->_saveOnUnuse = false;

    // The following are only used by `RAPass`.
    vreg->_raId = kInvalidValue;
    vreg->_memOffset = 0;
    vreg->_homeMask = 0;
    vreg->_state = VirtReg::kStateNone;
    vreg->_physId = kInvalidReg;
    vreg->_modified = false;
    vreg->_memCell = nullptr;
    vreg->_tied = nullptr;

    _vRegArray.appendUnsafe(vreg);
    return vreg;
  }

  return nullptr;
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

    vreg->_name = _dataAllocator.sdup(buf);
    va_end(ap);
  }
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
