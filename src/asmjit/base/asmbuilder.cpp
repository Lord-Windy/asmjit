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
#include "../base/asmbuilder.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::AsmBuilder - Construction / Destruction]
// ============================================================================

AsmBuilder::AsmBuilder(CodeHolder* holder) noexcept
  : CodeGen(kTypeBuilder),
    _nodeAllocator(32768 - Zone::kZoneOverhead),
    _dataAllocator(8192  - Zone::kZoneOverhead),
    _nodeFlowId(0),
    _nodeFlags(0),
    _firstNode(nullptr),
    _lastNode(nullptr),
    _cursor(nullptr) {

  if (holder)
    holder->attach(this);
}
AsmBuilder::~AsmBuilder() noexcept {}

// ============================================================================
// [asmjit::AsmBuilder - Events]
// ============================================================================

Error AsmBuilder::onAttach(CodeHolder* holder) noexcept {
  return Base::onAttach(holder);
}

Error AsmBuilder::onDetach(CodeHolder* holder) noexcept {
  _nodeAllocator.reset(false);
  _dataAllocator.reset(false);
  _labelArray.reset(false);

  _nodeFlowId = 0;
  _nodeFlags = 0;

  _firstNode = nullptr;
  _lastNode = nullptr;
  _cursor = nullptr;

  return Base::onDetach(holder);
}

// ============================================================================
// [asmjit::AsmBuilder - Node-Factory]
// ============================================================================

Error AsmBuilder::getAsmLabel(AsmLabel** pOut, uint32_t id) noexcept {
  if (_lastError) return _lastError;
  ASMJIT_ASSERT(_holder != nullptr);

  size_t index = Operand::unpackId(id);
  if (ASMJIT_UNLIKELY(index >= _holder->getLabelsCount()))
    return DebugUtils::errored(kErrorInvalidLabel);

  if (index >= _labelArray.getLength())
    ASMJIT_PROPAGATE(_labelArray.resize(index + 1));

  AsmLabel* node = _labelArray[index];
  if (!node) {
    node = newNodeT<AsmLabel>(id);
    if (ASMJIT_UNLIKELY(!node))
      return DebugUtils::errored(kErrorNoHeapMemory);
    _labelArray[index] = node;
  }

  *pOut = node;
  return kErrorOk;
}

Error AsmBuilder::registerLabelNode(AsmLabel* node) noexcept {
  if (_lastError) return _lastError;
  ASMJIT_ASSERT(_holder != nullptr);

  // Don't call setLastError() from here, we are noexcept and we are called
  // by `newLabelNode()` and `newFuncNode()`, which are noexcept as too.
  uint32_t id;
  ASMJIT_PROPAGATE(_holder->newLabelId(id));
  size_t index = Operand::unpackId(id);

  // We just added one label so it must be true.
  ASMJIT_ASSERT(_labelArray.getLength() < index + 1);
  ASMJIT_PROPAGATE(_labelArray.resize(index + 1));

  _labelArray[index] = node;
  node->_id = id;
  return kErrorOk;
}

AsmLabel* AsmBuilder::newLabelNode() noexcept {
  AsmLabel* node = newNodeT<AsmLabel>();
  if (!node || registerLabelNode(node) != kErrorOk)
    return nullptr;
  return node;
}

AsmAlign* AsmBuilder::newAlignNode(uint32_t mode, uint32_t alignment) noexcept {
  return newNodeT<AsmAlign>(mode, alignment);
}

AsmData* AsmBuilder::newDataNode(const void* data, uint32_t size) noexcept {
  if (size > AsmData::kInlineBufferSize) {
    void* cloned = _dataAllocator.alloc(size);
    if (!cloned) return nullptr;

    if (data) ::memcpy(cloned, data, size);
    data = cloned;
  }

  return newNodeT<AsmData>(const_cast<void*>(data), size);
}

AsmConstPool* AsmBuilder::newConstPool() noexcept {
  AsmConstPool* node = newNodeT<AsmConstPool>();
  if (!node || registerLabelNode(node) != kErrorOk)
    return nullptr;
  return node;
}

AsmComment* AsmBuilder::newCommentNode(const char* s, size_t len) noexcept {
  if (s) {
    if (len == kInvalidIndex) len = ::strlen(s);
    if (len > 0) {
      s = static_cast<char*>(_dataAllocator.dup(s, len));
      if (!s) return nullptr;
    }
  }

  return newNodeT<AsmComment>(s);
}

// ============================================================================
// [asmjit::AsmBuilder - Node-Builder]
// ============================================================================

AsmNode* AsmBuilder::addNode(AsmNode* node) noexcept {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);

  if (!_cursor) {
    if (!_firstNode) {
      _firstNode = node;
      _lastNode = node;
    }
    else {
      node->_next = _firstNode;
      _firstNode->_prev = node;
      _firstNode = node;
    }
  }
  else {
    AsmNode* prev = _cursor;
    AsmNode* next = _cursor->_next;

    node->_prev = prev;
    node->_next = next;

    prev->_next = node;
    if (next)
      next->_prev = node;
    else
      _lastNode = node;
  }

  _cursor = node;
  return node;
}

AsmNode* AsmBuilder::addAfter(AsmNode* node, AsmNode* ref) noexcept {
  ASMJIT_ASSERT(node);
  ASMJIT_ASSERT(ref);

  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);

  AsmNode* prev = ref;
  AsmNode* next = ref->_next;

  node->_prev = prev;
  node->_next = next;

  prev->_next = node;
  if (next)
    next->_prev = node;
  else
    _lastNode = node;

  return node;
}

AsmNode* AsmBuilder::addBefore(AsmNode* node, AsmNode* ref) noexcept {
  ASMJIT_ASSERT(node != nullptr);
  ASMJIT_ASSERT(node->_prev == nullptr);
  ASMJIT_ASSERT(node->_next == nullptr);
  ASMJIT_ASSERT(ref != nullptr);

  AsmNode* prev = ref->_prev;
  AsmNode* next = ref;

  node->_prev = prev;
  node->_next = next;

  next->_prev = node;
  if (prev)
    prev->_next = node;
  else
    _firstNode = node;

  return node;
}

static ASMJIT_INLINE void AsmBuilder_nodeRemoved(AsmBuilder* self, AsmNode* node_) noexcept {
  if (node_->isJmpOrJcc()) {
    AsmJump* node = static_cast<AsmJump*>(node_);
    AsmLabel* label = node->getTarget();

    if (label) {
      // Disconnect.
      AsmJump** pPrev = &label->_from;
      for (;;) {
        ASMJIT_ASSERT(*pPrev != nullptr);

        AsmJump* current = *pPrev;
        if (!current) break;

        if (current == node) {
          *pPrev = node->_jumpNext;
          break;
        }

        pPrev = &current->_jumpNext;
      }

      label->subNumRefs();
    }
  }
}

AsmNode* AsmBuilder::removeNode(AsmNode* node) noexcept {
  AsmNode* prev = node->_prev;
  AsmNode* next = node->_next;

  if (_firstNode == node)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == node)
    _lastNode  = prev;
  else
    next->_prev = prev;

  node->_prev = nullptr;
  node->_next = nullptr;

  if (_cursor == node)
    _cursor = prev;
  AsmBuilder_nodeRemoved(this, node);

  return node;
}

void AsmBuilder::removeNodes(AsmNode* first, AsmNode* last) noexcept {
  if (first == last) {
    removeNode(first);
    return;
  }

  AsmNode* prev = first->_prev;
  AsmNode* next = last->_next;

  if (_firstNode == first)
    _firstNode = next;
  else
    prev->_next = next;

  if (_lastNode == last)
    _lastNode  = prev;
  else
    next->_prev = prev;

  AsmNode* node = first;
  for (;;) {
    AsmNode* next = node->getNext();
    ASMJIT_ASSERT(next != nullptr);

    node->_prev = nullptr;
    node->_next = nullptr;

    if (_cursor == node)
      _cursor = prev;
    AsmBuilder_nodeRemoved(this, node);

    if (node == last)
      break;
    node = next;
  }
}

AsmNode* AsmBuilder::setCursor(AsmNode* node) noexcept {
  AsmNode* old = _cursor;
  _cursor = node;
  return old;
}

// ============================================================================
// [asmjit::AsmBuilder - Code-Generation]
// ============================================================================

Label AsmBuilder::newLabel() {
  uint32_t id = kInvalidValue;

  if (!_lastError) {
    AsmLabel* node = newNodeT<AsmLabel>(id);
    if (ASMJIT_UNLIKELY(!node)) {
      setLastError(DebugUtils::errored(kErrorNoHeapMemory));
    }
    else {
      Error err = registerLabelNode(node);
      if (ASMJIT_UNLIKELY(err)) setLastError(err);
      id = node->getId();
    }
  }

  return Label(id);
}

Error AsmBuilder::bind(const Label& label) {
  if (_lastError) return _lastError;

  AsmLabel* node;
  Error err = getAsmLabel(&node, label);
  if (ASMJIT_UNLIKELY(err))
    return setLastError(err);

  addNode(node);
  return kErrorOk;
}

Error AsmBuilder::align(uint32_t mode, uint32_t alignment) {
  AsmAlign* node = newAlignNode(mode, alignment);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

Error AsmBuilder::embed(const void* data, uint32_t size) {
  AsmData* node = newDataNode(data, size);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

Error AsmBuilder::embedConstPool(const Label& label, const ConstPool& pool) {
  if (_lastError) return _lastError;

  if (!isLabelValid(label))
    return setLastError(DebugUtils::errored(kErrorInvalidLabel));

  ASMJIT_PROPAGATE(align(kAlignData, static_cast<uint32_t>(pool.getAlignment())));
  ASMJIT_PROPAGATE(bind(label));

  AsmData* node = newDataNode(nullptr, static_cast<uint32_t>(pool.getSize()));
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  pool.fill(node->getData());
  addNode(node);
  return kErrorOk;
}

Error AsmBuilder::comment(const char* s, size_t len) {
  AsmComment* node = newCommentNode(s, len);
  if (!node) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

  addNode(node);
  return kErrorOk;
}

// ============================================================================
// [asmjit::AsmBuilder - Code-Serialization]
// ============================================================================

Error AsmBuilder::serialize(CodeGen* dst) {
  AsmNode* node_ = getFirstNode();

  do {
    dst->setInlineComment(node_->getInlineComment());

    switch (node_->getType()) {
      case AsmNode::kNodeAlign: {
        AsmAlign* node = static_cast<AsmAlign*>(node_);
        ASMJIT_PROPAGATE(
          dst->align(node->getMode(), node->getAlignment()));
        break;
      }

      case AsmNode::kNodeData: {
        AsmData* node = static_cast<AsmData*>(node_);
        ASMJIT_PROPAGATE(
          dst->embed(node->getData(), node->getSize()));
        break;
      }

      case AsmNode::kNodeFunc:
      case AsmNode::kNodeLabel: {
        AsmLabel* node = static_cast<AsmLabel*>(node_);
        ASMJIT_PROPAGATE(
          dst->bind(node->getLabel()));
        break;
      }

      case AsmNode::kNodeConstPool: {
        AsmConstPool* node = static_cast<AsmConstPool*>(node_);
        ASMJIT_PROPAGATE(
          dst->embedConstPool(node->getLabel(), node->getConstPool()));
        break;
      }

      case AsmNode::kNodeInst:
      case AsmNode::kNodeCall: {
        AsmInst* node = static_cast<AsmInst*>(node_);

        uint32_t instId = node->getInstId();
        uint32_t options = node->getOptions();

        const Operand* opArray = node->getOpArray();
        uint32_t opCount = node->getOpCount();

        const Operand_* o0 = &dst->_none;
        const Operand_* o1 = &dst->_none;
        const Operand_* o2 = &dst->_none;
        const Operand_* o3 = &dst->_none;

        if (opCount > 0) o0 = &opArray[0];
        if (opCount > 1) o1 = &opArray[1];
        if (opCount > 2) o2 = &opArray[2];
        if (opCount > 3) o3 = &opArray[3];
        if (opCount > 4) dst->setOp4(opArray[4]);
        if (opCount > 5) dst->setOp5(opArray[5]);

        dst->setOptions(options);
        ASMJIT_PROPAGATE(
          dst->_emit(instId, *o0, *o1, *o2, *o3));
        break;
      }

      case AsmNode::kNodeComment: {
        AsmComment* node = static_cast<AsmComment*>(node_);
        ASMJIT_PROPAGATE(
          dst->comment(node->getInlineComment()));
        break;
      }

      default:
        break;
    }

    node_ = node_->getNext();
  } while (node_);

  return kErrorOk;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
