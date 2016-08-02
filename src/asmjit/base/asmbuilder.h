// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_ASMBUILDER_H
#define _ASMJIT_BASE_ASMBUILDER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/constpool.h"
#include "../base/codeholder.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

class AsmAlign;
class AsmComment;
class AsmConstPool;
class AsmData;
class AsmInst;
class AsmJump;
class AsmNode;
class AsmLabel;
class AsmSentinel;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::AsmBuilder]
// ============================================================================

class ASMJIT_VIRTAPI AsmBuilder : public CodeGen {
public:
  ASMJIT_NO_COPY(AsmBuilder)
  typedef CodeGen Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmBuilder` instance.
  ASMJIT_API AsmBuilder(CodeHolder* holder = nullptr) noexcept;
  //! Destroy the `AsmBuilder` instance.
  ASMJIT_API virtual ~AsmBuilder() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error onAttach(CodeHolder* holder) noexcept override;
  ASMJIT_API virtual Error onDetach(CodeHolder* holder) noexcept override;

  // --------------------------------------------------------------------------
  // [Node-Factory]
  // --------------------------------------------------------------------------

  //! \internal
  template<typename T>
  ASMJIT_INLINE T* newNodeT() noexcept { return new(_nodeAllocator.alloc(sizeof(T))) T(this); }

  //! \internal
  template<typename T, typename P0>
  ASMJIT_INLINE T* newNodeT(P0 p0) noexcept { return new(_nodeAllocator.alloc(sizeof(T))) T(this, p0); }

  //! \internal
  template<typename T, typename P0, typename P1>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1) noexcept { return new(_nodeAllocator.alloc(sizeof(T))) T(this, p0, p1); }

  //! \internal
  template<typename T, typename P0, typename P1, typename P2>
  ASMJIT_INLINE T* newNodeT(P0 p0, P1 p1, P2 p2) noexcept { return new(_nodeAllocator.alloc(sizeof(T))) T(this, p0, p1, p2); }

  ASMJIT_API Error registerLabelNode(AsmLabel* node) noexcept;
  //! Get `AsmLabel` by `id`.
  ASMJIT_API Error getAsmLabel(AsmLabel** pOut, uint32_t id) noexcept;
  //! Get `AsmLabel` by `label`.
  ASMJIT_INLINE Error getAsmLabel(AsmLabel** pOut, const Label& label) noexcept { return getAsmLabel(pOut, label.getId()); }

  //! Create a new \ref AsmLabel node.
  ASMJIT_API AsmLabel* newLabelNode() noexcept;
  //! Create a new \ref AsmAlign node.
  ASMJIT_API AsmAlign* newAlignNode(uint32_t mode, uint32_t alignment) noexcept;
  //! Create a new \ref AsmData node.
  ASMJIT_API AsmData* newDataNode(const void* data, uint32_t size) noexcept;
  //! Create a new \ref AsmConstPool node.
  ASMJIT_API AsmConstPool* newConstPool() noexcept;
  //! Create a new \ref AsmComment node.
  ASMJIT_API AsmComment* newCommentNode(const char* s, size_t len) noexcept;

  // --------------------------------------------------------------------------
  // [Node-Builder]
  // --------------------------------------------------------------------------

  //! Add `node` after the current and set current to `node`.
  ASMJIT_API AsmNode* addNode(AsmNode* node) noexcept;
  //! Insert `node` after `ref`.
  ASMJIT_API AsmNode* addAfter(AsmNode* node, AsmNode* ref) noexcept;
  //! Insert `node` before `ref`.
  ASMJIT_API AsmNode* addBefore(AsmNode* node, AsmNode* ref) noexcept;
  //! Remove `node`.
  ASMJIT_API AsmNode* removeNode(AsmNode* node) noexcept;
  //! Remove multiple nodes.
  ASMJIT_API void removeNodes(AsmNode* first, AsmNode* last) noexcept;

  //! Get the first node.
  ASMJIT_INLINE AsmNode* getFirstNode() const noexcept { return _firstNode; }
  //! Get the last node.
  ASMJIT_INLINE AsmNode* getLastNode() const noexcept { return _lastNode; }

  //! Get current node.
  //!
  //! \note If this method returns null it means that nothing has been
  //! emitted yet.
  ASMJIT_INLINE AsmNode* getCursor() const noexcept { return _cursor; }
  //! Set the current node without returning the previous node.
  ASMJIT_INLINE void _setCursor(AsmNode* node) noexcept { _cursor = node; }
  //! Set the current node to `node` and return the previous one.
  ASMJIT_API AsmNode* setCursor(AsmNode* node) noexcept;

  // --------------------------------------------------------------------------
  // [Code-Generation]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Label newLabel() override;
  ASMJIT_API virtual Error bind(const Label& label) override;
  ASMJIT_API virtual Error align(uint32_t mode, uint32_t alignment) override;
  ASMJIT_API virtual Error embed(const void* data, uint32_t size) override;
  ASMJIT_API virtual Error embedConstPool(const Label& label, const ConstPool& pool) override;
  ASMJIT_API virtual Error comment(const char* s, size_t len = kInvalidIndex) override;

  // --------------------------------------------------------------------------
  // [Code-Serialization]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error serialize(CodeGen* dst);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  Zone _nodeAllocator;                   //!< Node allocator.
  Zone _dataAllocator;                   //!< Data and string allocator (includes comments).
  PodVector<AsmLabel*> _labelArray;      //!< AsmLabel array.

  AsmNode* _firstNode;                   //!< First node of the current section.
  AsmNode* _lastNode;                    //!< Last node of the current section.
  AsmNode* _cursor;                      //!< Current node (cursor).

  uint32_t _nodeFlowId;                  //!< Flow-id assigned to each new node.
  uint32_t _nodeFlags;                   //!< Flags assigned to each new node.
};

// ============================================================================
// [asmjit::AsmNode]
// ============================================================================

//! Node (AsmBuilder).
//!
//! Every node represents a building-block used by \ref AsmBuilder. It can be
//! instruction, data, label, comment, directive, or any other high-level
//! representation that can be transformed to the building blocks mentioned.
//! Every class that inherits \ref AsmBuilder can define its own nodes that it
//! can lower to basic nodes.
class AsmNode {
public:
  ASMJIT_NO_COPY(AsmNode)

  // --------------------------------------------------------------------------
  // [Type]
  // --------------------------------------------------------------------------

  //! Type of \ref AsmNode.
  ASMJIT_ENUM(NodeType) {
    kNodeNone       = 0,                 //!< Invalid node (internal, don't use).

    // [Basic]
    kNodeInst       = 1,                 //!< Node is \ref AsmInst or \ref AsmJump.
    kNodeData       = 2,                 //!< Node is \ref AsmData.
    kNodeAlign      = 3,                 //!< Node is \ref AsmAlign.
    kNodeLabel      = 4,                 //!< Node is \ref AsmLabel.
    kNodeComment    = 5,                 //!< Node is \ref AsmComment.
    kNodeSentinel   = 6,                 //!< Node is \ref AsmSentinel.
    kNodeConstPool  = 7,                 //!< Node is \ref AsmConstPool.

    // [Compiler]
    kNodeFunc       = 16,                //!< Node is \ref AsmFunc (considered as \ref AsmLabel by \ref AsmBuilder).
    kNodeFuncExit   = 17,                //!< Node is \ref AsmFuncRet.
    kNodeCall       = 18,                //!< Node is \ref AsmCall.
    kNodePushArg    = 19,                //!< Node is \ref AsmPushArg.
    kNodeHint       = 20,                //!< Node is \ref AsmHint.

    // [User]
    kNodeUser       = 32                 //!< First ID of a user-defined node.
  };

  // --------------------------------------------------------------------------
  // [Flags]
  // --------------------------------------------------------------------------

  ASMJIT_ENUM(Flags) {
    //! The node has been translated by the Compiler.
    kFlagIsTranslated = 0x0001,

    //! Whether the node can be safely removed by the `Compiler` in case it's
    //! unreachable.
    kFlagIsRemovable = 0x0004,

    //! Whether the node is informative only and can be safely removed.
    kFlagIsInformative = 0x0008,

    //! Whether the `AsmInst` is a jump.
    kFlagIsJmp = 0x0010,
    //! Whether the `AsmInst` is a conditional jump.
    kFlagIsJcc = 0x0020,

    //! Whether the `AsmInst` is an unconditinal jump or conditional jump that is
    //! likely to be taken.
    kFlagIsTaken = 0x0040,

    //! Whether the `AsmNode` will return from a function.
    //!
    //! This flag is used by both `AsmSentinel` and `AsmFuncRet`.
    kFlagIsRet = 0x0080,

    //! Whether the instruction is special.
    kFlagIsSpecial = 0x0100,

    //! Whether the instruction is an FPU instruction.
    kFlagIsFp = 0x0200
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new \ref AsmNode - always use \ref AsmBuilder to allocate nodes.
  ASMJIT_INLINE AsmNode(AsmBuilder* ab, uint32_t type) noexcept {
    _prev = nullptr;
    _next = nullptr;
    _type = static_cast<uint8_t>(type);
    _opCount = 0;
    _flags = static_cast<uint16_t>(ab->_nodeFlags);
    _flowId = ab->_nodeFlowId;
    _inlineComment = nullptr;
    _workData = nullptr;
    _tokenId = 0;
  }
  //! SHOULD NEVER BE CALLED - \ref AsmBuilder uses \ref Zone allocator.
  ASMJIT_INLINE ~AsmNode() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get previous node in the compiler stream.
  ASMJIT_INLINE AsmNode* getPrev() const noexcept { return _prev; }
  //! Get next node in the compiler stream.
  ASMJIT_INLINE AsmNode* getNext() const noexcept { return _next; }

  //! Get the node type, see \ref Type.
  ASMJIT_INLINE uint32_t getType() const noexcept { return _type; }
  //! Get the node flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }

  //! Get whether the instruction has flag `flag`.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (static_cast<uint32_t>(_flags) & flag) != 0; }
  //! Set node flags to `flags`.
  ASMJIT_INLINE void setFlags(uint32_t flags) noexcept { _flags = static_cast<uint16_t>(flags); }
  //! Add instruction `flags`.
  ASMJIT_INLINE void orFlags(uint32_t flags) noexcept { _flags |= static_cast<uint16_t>(flags); }
  //! And instruction `flags`.
  ASMJIT_INLINE void andFlags(uint32_t flags) noexcept { _flags &= static_cast<uint16_t>(flags); }
  //! Clear instruction `flags`.
  ASMJIT_INLINE void andNotFlags(uint32_t flags) noexcept { _flags &= ~static_cast<uint16_t>(flags); }

  //! Get whether the node has been translated.
  ASMJIT_INLINE bool isTranslated() const noexcept { return hasFlag(kFlagIsTranslated); }

  //! Get whether the node is removable if it's in unreachable code block.
  ASMJIT_INLINE bool isRemovable() const noexcept { return hasFlag(kFlagIsRemovable); }
  //! Get whether the node is informative only (comment, hint).
  ASMJIT_INLINE bool isInformative() const noexcept { return hasFlag(kFlagIsInformative); }

  //! Whether the node is `AsmLabel`.
  ASMJIT_INLINE bool isLabel() const noexcept { return _type == kNodeLabel; }
  //! Whether the `AsmInst` node is an unconditional jump.
  ASMJIT_INLINE bool isJmp() const noexcept { return hasFlag(kFlagIsJmp); }
  //! Whether the `AsmInst` node is a conditional jump.
  ASMJIT_INLINE bool isJcc() const noexcept { return hasFlag(kFlagIsJcc); }
  //! Whether the `AsmInst` node is a conditional/unconditional jump.
  ASMJIT_INLINE bool isJmpOrJcc() const noexcept { return hasFlag(kFlagIsJmp | kFlagIsJcc); }
  //! Whether the `AsmInst` node is a return.
  ASMJIT_INLINE bool isRet() const noexcept { return hasFlag(kFlagIsRet); }

  //! Get whether the node is `AsmInst` and the instruction is special.
  ASMJIT_INLINE bool isSpecial() const noexcept { return hasFlag(kFlagIsSpecial); }
  //! Get whether the node is `AsmInst` and the instruction uses x87-FPU.
  ASMJIT_INLINE bool isFp() const noexcept { return hasFlag(kFlagIsFp); }

  //! Get flow index.
  ASMJIT_INLINE uint32_t getFlowId() const noexcept { return _flowId; }
  //! Set flow index.
  ASMJIT_INLINE void setFlowId(uint32_t flowId) noexcept { _flowId = flowId; }

  //! Get if the node has an inline comment.
  ASMJIT_INLINE bool hasInlineComment() const noexcept { return _inlineComment != nullptr; }
  //! Get an inline comment string.
  ASMJIT_INLINE const char* getInlineComment() const noexcept { return _inlineComment; }
  //! Set an inline comment string to `s`.
  ASMJIT_INLINE void setInlineComment(const char* s) noexcept { _inlineComment = s; }
  //! Set an inline comment string to null.
  ASMJIT_INLINE void resetInlineComment() noexcept { _inlineComment = nullptr; }

  //! Get if the node has associated work-data.
  ASMJIT_INLINE bool hasWorkData() const noexcept { return _workData != nullptr; }
  //! Get work-data - data used during processing & transformations.
  template<typename T>
  ASMJIT_INLINE T* getWorkData() const noexcept { return (T*)_workData; }
  //! Set work-data to `data`.
  template<typename T>
  ASMJIT_INLINE void setWorkData(T* data) noexcept { _workData = (void*)data; }
  //! Reset work-data to null.
  ASMJIT_INLINE void resetWorkData() noexcept { _workData = nullptr; }

  ASMJIT_INLINE bool matchesToken(uint32_t id) const noexcept { return _tokenId == id; }
  ASMJIT_INLINE uint32_t getTokenId() const noexcept { return _tokenId; }
  ASMJIT_INLINE void setTokenId(uint32_t id) noexcept { _tokenId = id; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  AsmNode* _prev;                        //!< Previous node.
  AsmNode* _next;                        //!< Next node.

  uint8_t _type;                         //!< Node type, see \ref NodeType.
  uint8_t _opCount;                      //!< Count of operands or zero.
  uint16_t _flags;                       //!< Flags, different meaning for every type of the node.
  uint32_t _flowId;                      //!< Flow index.

  const char* _inlineComment;            //!< Inline comment or null if not used.
  void* _workData;                       //!< Work-data used during processing & transformations phases.

  //! Processing token.
  //!
  //! Used by some algorithms to mark nodes as visited. If the token is
  //! generated in an incrementing way the visitor can just mark nodes it
  //! visits and them compare the `AsmNode`s token with its local token.
  //! If they are equal the node has been visited by exactly this visitor.
  //! Then the visitor doesn't need to clean things up as the next time the
  //! token will be different.
  uint32_t _tokenId;

  // TODO: 32-bit gap
};

// ============================================================================
// [asmjit::AsmInst]
// ============================================================================

//! Instruction (AsmBuilder).
//!
//! Wraps an instruction with its options and operands.
class AsmInst : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmInst)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmInst` instance.
  ASMJIT_INLINE AsmInst(AsmBuilder* ab, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : AsmNode(ab, kNodeInst) {

    orFlags(kFlagIsRemovable);
    _instId = static_cast<uint16_t>(instId);
    _reserved = 0;
    _options = options;

    _opCount = static_cast<uint8_t>(opCount);
    _opArray = opArray;

    _updateMemOp();
  }

  //! Destroy the `AsmInst` instance.
  ASMJIT_INLINE ~AsmInst() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the instruction id, see \ref X86Inst::Id.
  ASMJIT_INLINE uint32_t getInstId() const noexcept { return _instId; }
  //! Set the instruction id to `instId`.
  //!
  //! NOTE: Please do not modify instruction code if you don't know what you
  //! are doing. Incorrect instruction code and/or operands can cause random
  //! errors in production builds and will most probably trigger assertion
  //! failures in debug builds.
  ASMJIT_INLINE void setInstId(uint32_t instId) noexcept { _instId = static_cast<uint16_t>(instId); }

  //! Whether the instruction is either a jump or a conditional jump likely to
  //! be taken.
  ASMJIT_INLINE bool isTaken() const noexcept { return hasFlag(kFlagIsTaken); }

  //! Get emit options.
  ASMJIT_INLINE uint32_t getOptions() const noexcept { return _options; }
  //! Set emit options.
  ASMJIT_INLINE void setOptions(uint32_t options) noexcept { _options = options; }
  //! Add emit options.
  ASMJIT_INLINE void addOptions(uint32_t options) noexcept { _options |= options; }
  //! Mask emit options.
  ASMJIT_INLINE void andOptions(uint32_t options) noexcept { _options &= options; }
  //! Clear emit options.
  ASMJIT_INLINE void delOptions(uint32_t options) noexcept { _options &= ~options; }

  //! Get op-mask operand (used to represent AVX-512 op-mask selector).
  ASMJIT_INLINE Operand& getOpMask() noexcept { return _opMask; }
  //! \overload
  ASMJIT_INLINE const Operand& getOpMask() const noexcept { return _opMask; }
  //1 Set op-mask operand.
  ASMJIT_INLINE void setOpMask(const Operand& opMask) noexcept { _opMask = opMask; }

  //! Get operands count.
  ASMJIT_INLINE uint32_t getOpCount() const noexcept { return _opCount; }
  //! Get operands list.
  ASMJIT_INLINE Operand* getOpArray() noexcept { return _opArray; }
  //! \overload
  ASMJIT_INLINE const Operand* getOpArray() const noexcept { return _opArray; }

  //! Get whether the instruction contains a memory operand.
  ASMJIT_INLINE bool hasMemOp() const noexcept { return _memOpIndex != 0xFF; }
  //! Get memory operand.
  //!
  //! NOTE: Can only be called if the instruction has such operand,
  //! see `hasMemOp()`.
  ASMJIT_INLINE Mem* getMemOp() const noexcept {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<Mem*>(&_opArray[_memOpIndex]);
  }
  //! \overload
  template<typename T>
  ASMJIT_INLINE T* getMemOp() const noexcept {
    ASMJIT_ASSERT(hasMemOp());
    return static_cast<T*>(&_opArray[_memOpIndex]);
  }

  //! Set memory operand index, `0xFF` means no memory operand.
  ASMJIT_INLINE void setMemOpIndex(uint32_t index) noexcept { _memOpIndex = static_cast<uint8_t>(index); }
  //! Reset memory operand index to `0xFF` (no operand).
  ASMJIT_INLINE void resetMemOpIndex() noexcept { _memOpIndex = 0xFF; }

  // --------------------------------------------------------------------------
  // [Utils]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void _updateMemOp() noexcept {
    Operand* opArray = getOpArray();
    uint32_t opCount = getOpCount();

    uint32_t i;
    for (i = 0; i < opCount; i++)
      if (opArray[i].isMem())
        goto Update;
    i = 0xFF;

Update:
    setMemOpIndex(i);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint16_t _instId;                      //!< Instruction id, depends on the architecture.
  uint8_t _memOpIndex;                   //!< \internal
  uint8_t _reserved;                     //!< \internal
  uint32_t _options;                     //!< Instruction options.
  Operand _opMask;                       //!< Instruction op-mask (selector).
  Operand* _opArray;                     //!< Instruction operands.
};

// ============================================================================
// [asmjit::AsmJump]
// ============================================================================

//! Asm jump (conditional or direct).
//!
//! Extension of `AsmInst` node, which stores more information about the jump.
class AsmJump : public AsmInst {
public:
  ASMJIT_NO_COPY(AsmJump)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE AsmJump(AsmBuilder* ab, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : AsmInst(ab, instId, options, opArray, opCount),
      _target(nullptr),
      _jumpNext(nullptr) {}
  ASMJIT_INLINE ~AsmJump() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE AsmLabel* getTarget() const noexcept { return _target; }
  ASMJIT_INLINE AsmJump* getJumpNext() const noexcept { return _jumpNext; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  AsmLabel* _target;                     //!< Target node.
  AsmJump* _jumpNext;                    //!< Next jump to the same target in a single linked-list.
};

// ============================================================================
// [asmjit::AsmData]
// ============================================================================

//! Asm data (AsmBuilder).
//!
//! Wraps `.data` directive. The node contains data that will be placed at the
//! node's position in the assembler stream. The data is considered to be RAW;
//! no analysis nor byte-order conversion is performed on RAW data.
class AsmData : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmData)
  enum { kInlineBufferSize = 12 };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmData` instance.
  ASMJIT_INLINE AsmData(AsmBuilder* ab, void* data, uint32_t size) noexcept : AsmNode(ab, kNodeData) {
    if (size <= kInlineBufferSize) {
      if (data) ::memcpy(_buf, data, size);
    }
    else {
      _externalPtr = static_cast<uint8_t*>(data);
    }
    _size = size;
  }

  //! Destroy the `AsmData` instance.
  ASMJIT_INLINE ~AsmData() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get size of the data.
  uint32_t getSize() const noexcept { return _size; }
  //! Get pointer to the data.
  uint8_t* getData() const noexcept { return _size <= kInlineBufferSize ? const_cast<uint8_t*>(_buf) : _externalPtr; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      uint8_t _buf[kInlineBufferSize];   //!< Embedded data buffer.
      uint32_t _size;                    //!< Size of the data.
    };
    struct {
      uint8_t* _externalPtr;             //!< Pointer to external data.
    };
  };
};

// ============================================================================
// [asmjit::AsmAlign]
// ============================================================================

//! Align directive (AsmBuilder).
//!
//! Wraps `.align` directive.
class AsmAlign : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmAlign)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmAlign` instance.
  ASMJIT_INLINE AsmAlign(AsmBuilder* ab, uint32_t mode, uint32_t alignment) noexcept
    : AsmNode(ab, kNodeAlign),
      _mode(mode),
      _alignment(alignment) {}
  //! Destroy the `AsmAlign` instance.
  ASMJIT_INLINE ~AsmAlign() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get align mode.
  ASMJIT_INLINE uint32_t getMode() const noexcept { return _mode; }
  //! Set align mode.
  ASMJIT_INLINE void setMode(uint32_t mode) noexcept { _mode = mode; }

  //! Get align offset in bytes.
  ASMJIT_INLINE uint32_t getAlignment() const noexcept { return _alignment; }
  //! Set align offset in bytes to `offset`.
  ASMJIT_INLINE void setAlignment(uint32_t alignment) noexcept { _alignment = alignment; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _mode;                        //!< Align mode, see \ref AlignMode.
  uint32_t _alignment;                   //!< Alignment (in bytes).
};

// ============================================================================
// [asmjit::AsmLabel]
// ============================================================================

//! Label (AsmBuilder).
class AsmLabel : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmLabel)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmLabel` instance.
  ASMJIT_INLINE AsmLabel(AsmBuilder* ab, uint32_t id = kInvalidValue) noexcept
    : AsmNode(ab, kNodeLabel),
      _id(id),
      _numRefs(0),
      _from(nullptr) {}
  //! Destroy the `AsmLabel` instance.
  ASMJIT_INLINE ~AsmLabel() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the label id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }
  //! Get the label as `Label` operand.
  ASMJIT_INLINE Label getLabel() const noexcept { return Label(_id); }

  //! Get first jmp instruction.
  ASMJIT_INLINE AsmJump* getFrom() const noexcept { return _from; }

  //! Get number of jumps to this target.
  ASMJIT_INLINE uint32_t getNumRefs() const noexcept { return _numRefs; }
  //! Set number of jumps to this target.
  ASMJIT_INLINE void setNumRefs(uint32_t i) noexcept { _numRefs = i; }

  //! Add number of jumps to this target.
  ASMJIT_INLINE void addNumRefs(uint32_t i = 1) noexcept { _numRefs += i; }
  //! Subtract number of jumps to this target.
  ASMJIT_INLINE void subNumRefs(uint32_t i = 1) noexcept { _numRefs -= i; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;                          //!< Label id.
  uint32_t _numRefs;                     //!< Count of jumps here.
  AsmJump* _from;                        //!< Linked-list of nodes that can jump here.
};

// ============================================================================
// [asmjit::AsmComment]
// ============================================================================

//! Comment (AsmBuilder).
class AsmComment : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmComment)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmComment` instance.
  ASMJIT_INLINE AsmComment(AsmBuilder* ab, const char* comment) noexcept : AsmNode(ab, kNodeComment) {
    orFlags(kFlagIsRemovable | kFlagIsInformative);
    _inlineComment = comment;
  }

  //! Destroy the `AsmComment` instance.
  ASMJIT_INLINE ~AsmComment() noexcept {}
};

// ============================================================================
// [asmjit::AsmSentinel]
// ============================================================================

//! Sentinel (AsmBuilder).
//!
//! Sentinel is a marker that is completely ignored by the code-generator.
class AsmSentinel : public AsmNode {
public:
  ASMJIT_NO_COPY(AsmSentinel)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmSentinel` instance.
  ASMJIT_INLINE AsmSentinel(AsmBuilder* ab) noexcept : AsmNode(ab, kNodeSentinel) {
    orFlags(kFlagIsRet);
  }

  //! Destroy the `AsmSentinel` instance.
  ASMJIT_INLINE ~AsmSentinel() noexcept {}
};

// ============================================================================
// [asmjit::AsmConstPool]
// ============================================================================

class AsmConstPool : public AsmLabel {
public:
  ASMJIT_NO_COPY(AsmConstPool)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `AsmConstPool` instance.
  ASMJIT_INLINE AsmConstPool(AsmBuilder* ab, uint32_t id = kInvalidValue) noexcept
    : AsmLabel(ab, id),
      _constPool(&ab->_dataAllocator) {}

  //! Destroy the `AsmConstPool` instance.
  ASMJIT_INLINE ~AsmConstPool() noexcept {}

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE ConstPool& getConstPool() noexcept { return _constPool; }
  ASMJIT_INLINE const ConstPool& getConstPool() const noexcept { return _constPool; }

  //! See \ref ConstPool::add().
  ASMJIT_INLINE Error add(const void* data, size_t size, size_t& dstOffset) noexcept {
    return _constPool.add(data, size, dstOffset);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  ConstPool _constPool;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_ASMBUILDER_H
