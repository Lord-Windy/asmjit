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
#include "../base/compiler.h"
#include "../base/compilercontext_p.h"
#include "../base/cpuinfo.h"
#include "../base/logging.h"
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
// [asmjit::Compiler - Construction / Destruction]
// ============================================================================

Compiler::Compiler() noexcept
  : AsmBuilder(),
    _maxLookAhead(kCompilerDefaultLookAhead),
    _tokenGenerator(0),
    _typeIdMap(nullptr),
    _func(nullptr),
    _vRegAllocator(4096 - Zone::kZoneOverhead),
    _localConstPool(nullptr),
    _globalConstPool(nullptr) {

  _type = kTypeCompiler;
}
Compiler::~Compiler() noexcept {}

// ============================================================================
// [asmjit::Compiler - Events]
// ============================================================================

Error Compiler::onAttach(CodeHolder* holder) noexcept {
  return Base::onAttach(holder);
}

Error Compiler::onDetach(CodeHolder* holder) noexcept {
  _maxLookAhead = kCompilerDefaultLookAhead;

  _tokenGenerator = 0;
  _func = nullptr;

  _localConstPool = nullptr;
  _globalConstPool = nullptr;

  _vRegAllocator.reset(false);
  _vRegArray.reset(false);

  return Base::onDetach(holder);
}

// ============================================================================
// [asmjit::Compiler - Node-Factory]
// ============================================================================

AsmHint* Compiler::newHintNode(Reg& r, uint32_t hint, uint32_t value) noexcept {
  if (!r.isVirtReg()) return nullptr;

  VirtReg* vr = getVirtReg(r);
  return newNodeT<AsmHint>(vr, hint, value);
}

// ============================================================================
// [asmjit::Compiler - Func]
// ============================================================================

AsmFunc* Compiler::addFunc(AsmFunc* func) {
  ASMJIT_ASSERT(_func == nullptr);
  _func = func;

  addNode(func);                 // Function node.
  AsmNode* cursor = getCursor(); // {CURSOR}.
  addNode(func->getExitNode());  // Function exit label.
  addNode(func->getEnd());       // Function end marker.

  _setCursor(cursor);
  return func;
}

// ============================================================================
// [asmjit::Compiler - Hint]
// ============================================================================

Error Compiler::_hint(Reg& r, uint32_t hint, uint32_t value) {
  if (!r.isVirtReg()) return kErrorOk;

  AsmHint* node = newHintNode(r, hint, value);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

// ============================================================================
// [asmjit::Compiler - Vars]
// ============================================================================

VirtReg* Compiler::newVirtReg(const VirtType& typeInfo, const char* name) noexcept {
  size_t index = _vRegArray.getLength();
  if (index <= Operand::kPackedIdCount) {
    VirtReg* vreg;
    if (_vRegArray.willGrow(1) != kErrorOk || !(vreg = _vRegAllocator.allocT<VirtReg>()))
      return nullptr;

    vreg->_name = noName;
    vreg->_id = Operand::packId(static_cast<uint32_t>(index));
    vreg->_localId = kInvalidValue;

#if !defined(ASMJIT_DISABLE_LOGGING)
    if (name && name[0] != '\0')
      vreg->_name = _dataAllocator.sdup(name);
#endif // !ASMJIT_DISABLE_LOGGING

    vreg->_regInfo.signature = typeInfo.getSignature();
    vreg->_typeId = static_cast<uint8_t>(typeInfo.getTypeId());
    vreg->_priority = 10;

    vreg->_state = VirtReg::kStateNone;
    vreg->_physId = kInvalidReg;
    vreg->_isStack = false;
    vreg->_isMemArg = false;
    vreg->_isCalculated = false;
    vreg->_saveOnUnuse = false;
    vreg->_modified = false;
    vreg->_reserved0 = 0;
    vreg->_alignment = static_cast<uint8_t>(Utils::iMin<uint32_t>(typeInfo.getTypeSize(), 64));

    vreg->_size = typeInfo.getTypeSize();
    vreg->_homeMask = 0;

    vreg->_memOffset = 0;
    vreg->_memCell = nullptr;
    vreg->_tied = nullptr;

    _vRegArray.appendUnsafe(vreg);
    return vreg;
  }

  return nullptr;
}

Error Compiler::alloc(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintAlloc, kInvalidValue);
}

Error Compiler::alloc(Reg& reg, uint32_t physId) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintAlloc, physId);
}

Error Compiler::alloc(Reg& reg, const Reg& physReg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintAlloc, physReg.getId());
}

Error Compiler::save(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintSave, kInvalidValue);
}

Error Compiler::spill(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintSpill, kInvalidValue);
}

Error Compiler::unuse(Reg& reg) {
  if (!reg.isVirtReg()) return kErrorOk;
  return _hint(reg, AsmHint::kHintUnuse, kInvalidValue);
}

uint32_t Compiler::getPriority(Reg& reg) const {
  if (!reg.isVirtReg()) return 0;
  return getVirtRegById(reg.getId())->getPriority();
}

void Compiler::setPriority(Reg& reg, uint32_t priority) {
  if (!reg.isVirtReg()) return;
  if (priority > 255) priority = 255;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (vreg) vreg->_priority = static_cast<uint8_t>(priority);
}

bool Compiler::getSaveOnUnuse(Reg& reg) const {
  if (!reg.isVirtReg()) return false;

  VirtReg* vreg = getVirtRegById(reg.getId());
  return static_cast<bool>(vreg->_saveOnUnuse);
}

void Compiler::setSaveOnUnuse(Reg& reg, bool value) {
  if (!reg.isVirtReg()) return;

  VirtReg* vreg = getVirtRegById(reg.getId());
  if (!vreg) return;

  vreg->_saveOnUnuse = value;
}

void Compiler::rename(Reg& reg, const char* fmt, ...) {
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
