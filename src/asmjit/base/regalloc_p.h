// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_REGALLOC_P_H
#define _ASMJIT_BASE_REGALLOC_P_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/bitutils.h"
#include "../base/codecompiler.h"
#include "../base/containers.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::RACell]
// ============================================================================

//! Register allocator's (RA) memory cell.
struct RACell {
  RACell* next;                          //!< Next active cell.
  int32_t offset;                        //!< Cell offset, relative to base-offset.
  uint32_t size;                         //!< Cell size.
  uint32_t alignment;                    //!< Cell alignment.
};

// ============================================================================
// [asmjit::RAData]
// ============================================================================

//! Register allocator's (RA) data associated with each \ref CBNode.
struct RAData {
  ASMJIT_INLINE RAData(uint32_t tiedTotal) noexcept
    : liveness(nullptr),
      state(nullptr),
      tiedTotal(tiedTotal),
      tokenId(0) {}

  BitArray* liveness;                    //!< Liveness bits (populated by liveness-analysis).
  RAState* state;                        //!< Optional saved \ref RAState.
  uint32_t tiedTotal;                    //!< Total count of \ref TiedReg regs.

  //! Processing token.
  //!
  //! Used by some algorithms to mark nodes as visited. If the token is
  //! generated in an incrementing way the visitor can just mark nodes it
  //! visits and them compare the `CBNode`s token with its local token.
  //! If they are equal the node has been visited by exactly this visitor.
  //! Then the visitor doesn't need to clean things up as the next time the
  //! token will be different.
  uint32_t tokenId;
};

// ============================================================================
// [asmjit::RAState]
// ============================================================================

//! Variables' state.
struct RAState {};

// ============================================================================
// [asmjit::RAPipeline]
// ============================================================================

//! \internal
//!
//! Register allocator pipeline used by \ref CodeCompiler.
struct RAPipeline : public CBPipeline {
public:
  ASMJIT_NO_COPY(RAPipeline)

  typedef void (ASMJIT_CDECL* TraceNodeFunc)(RAPipeline* self, CBNode* node_, const char* prefix);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  RAPipeline() noexcept;
  virtual ~RAPipeline() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual Error process(CodeBuilder* cb, Zone* zone) noexcept override;

  //! Run the register allocator for a given function `func`.
  virtual Error compile(CCFunc* func) noexcept;

  //! Called by `compile()` to prepare the register allocator to process the
  //! given function. It should reset and set-up everything (i.e. no states
  //! from a previous compilation should prevail).
  virtual Error prepare(CCFunc* func) noexcept;

  //! Called after `compile()` to clean everything up, no matter if it
  //! succeeded or failed.
  virtual void cleanup() noexcept;

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function.
  ASMJIT_INLINE CCFunc* getFunc() const { return _func; }
  //! Get stop node.
  ASMJIT_INLINE CBNode* getStop() const { return _stop; }

  //! Get extra block.
  ASMJIT_INLINE CBNode* getExtraBlock() const { return _extraBlock; }
  //! Set extra block.
  ASMJIT_INLINE void setExtraBlock(CBNode* node) { _extraBlock = node; }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Get current state.
  ASMJIT_INLINE RAState* getState() const { return _state; }

  //! Load current state from `target` state.
  virtual void loadState(RAState* src) = 0;

  //! Save current state, returning new `RAState` instance.
  virtual RAState* saveState() = 0;

  //! Change the current state to `target` state.
  virtual void switchState(RAState* src) = 0;

  //! Change the current state to the intersection of two states `a` and `b`.
  virtual void intersectStates(RAState* a, RAState* b) = 0;

  // --------------------------------------------------------------------------
  // [Context]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Error makeLocal(VirtReg* vreg) {
    if (vreg->hasLocalId())
      return kErrorOk;

    uint32_t cid = static_cast<uint32_t>(_contextVd.getLength());
    ASMJIT_PROPAGATE(_contextVd.append(vreg));

    vreg->setLocalId(cid);
    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Mem]
  // --------------------------------------------------------------------------

  RACell* _newVarCell(VirtReg* vreg);
  RACell* _newStackCell(uint32_t size, uint32_t alignment);

  ASMJIT_INLINE RACell* getVarCell(VirtReg* vreg) {
    RACell* cell = vreg->getMemCell();
    return cell ? cell : _newVarCell(vreg);
  }

  virtual Error resolveCellOffsets();

  // --------------------------------------------------------------------------
  // [Bits]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE BitArray* newBits(uint32_t len) {
    return static_cast<BitArray*>(
      _zone->allocZeroed(static_cast<size_t>(len) * BitArray::kEntitySize));
  }

  ASMJIT_INLINE BitArray* copyBits(const BitArray* src, uint32_t len) {
    return static_cast<BitArray*>(
      _zone->dup(src, static_cast<size_t>(len) * BitArray::kEntitySize));
  }

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  //! Fetch.
  //!
  //! Fetch iterates over all nodes and gathers information about all variables
  //! used. The process generates information required by register allocator,
  //! variable liveness analysis and translator.
  virtual Error fetch() = 0;

  // --------------------------------------------------------------------------
  // [Unreachable Code]
  // --------------------------------------------------------------------------

  //! Add unreachable-flow data to the unreachable flow list.
  ASMJIT_INLINE Error addUnreachableNode(CBNode* node) {
    PodList<CBNode*>::Link* link = _zone->allocT<PodList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _unreachableList.append(link);

    return kErrorOk;
  }

  //! Remove unreachable code.
  virtual Error removeUnreachableCode();

  // --------------------------------------------------------------------------
  // [Code-Flow]
  // --------------------------------------------------------------------------

  //! Add returning node (i.e. node that returns and where liveness analysis
  //! should start).
  ASMJIT_INLINE Error addReturningNode(CBNode* node) {
    PodList<CBNode*>::Link* link = _zone->allocT<PodList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _returningList.append(link);

    return kErrorOk;
  }

  //! Add jump-flow data to the jcc flow list.
  ASMJIT_INLINE Error addJccNode(CBNode* node) {
    PodList<CBNode*>::Link* link = _zone->allocT<PodList<CBNode*>::Link>();
    if (!link) return DebugUtils::errored(kErrorNoHeapMemory);

    link->setValue(node);
    _jccList.append(link);

    return kErrorOk;
  }

  // --------------------------------------------------------------------------
  // [Analyze]
  // --------------------------------------------------------------------------

  //! Perform variable liveness analysis.
  //!
  //! Analysis phase iterates over nodes in reverse order and generates a bit
  //! array describing variables that are alive at every node in the function.
  //! When the analysis start all variables are assumed dead. When a read or
  //! read/write operations of a variable is detected the variable becomes
  //! alive; when only write operation is detected the variable becomes dead.
  //!
  //! When a label is found all jumps to that label are followed and analysis
  //! repeats until all variables are resolved.
  virtual Error livenessAnalysis();

  // --------------------------------------------------------------------------
  // [Annotate]
  // --------------------------------------------------------------------------

  virtual Error annotate() = 0;
  virtual Error formatInlineComment(StringBuilder& dst, CBNode* node);

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  //! Translate code by allocating registers and handling state changes.
  virtual Error translate() = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeCompiler* _cc;                     //!< CodeCompiler.
  Zone* _zone;                           //!< Zone allocator passed to `process()`.

  CCFunc* _func;                         //!< Function being processed.
  CBNode* _stop;                         //!< Stop node.
  CBNode* _extraBlock;                   //!< Node that is used to insert extra code after the function body.

  TraceNodeFunc _traceNode;              //!< Only non-null if ASMJIT_TRACE is enabled.

  //! \internal
  //!
  //! Offset (how many bytes to add) to `VarMap` to get `TiedReg` array. Used
  //! by liveness analysis shared across all backends. This is needed because
  //! `VarMap` is a base class for a specialized version that liveness analysis
  //! doesn't use, it just needs `TiedReg` array.
  uint32_t _varMapToVaListOffset;

  uint8_t _emitComments;                 //!< Whether to emit comments.

  PodList<CBNode*> _unreachableList;     //!< Unreachable nodes.
  PodList<CBNode*> _returningList;       //!< Returning nodes.
  PodList<CBNode*> _jccList;             //!< Jump nodes.

  PodVector<VirtReg*> _contextVd;        //!< All variables used by the current function.

  RACell* _memVarCells;                  //!< Memory used to spill variables.
  RACell* _memStackCells;                //!< Memory used to allocate memory on the stack.

  uint32_t _mem1ByteVarsUsed;            //!< Count of 1-byte cells.
  uint32_t _mem2ByteVarsUsed;            //!< Count of 2-byte cells.
  uint32_t _mem4ByteVarsUsed;            //!< Count of 4-byte cells.
  uint32_t _mem8ByteVarsUsed;            //!< Count of 8-byte cells.
  uint32_t _mem16ByteVarsUsed;           //!< Count of 16-byte cells.
  uint32_t _mem32ByteVarsUsed;           //!< Count of 32-byte cells.
  uint32_t _mem64ByteVarsUsed;           //!< Count of 64-byte cells.
  uint32_t _memStackCellsUsed;           //!< Count of stack memory cells.

  uint32_t _memMaxAlign;                 //!< Maximum memory alignment used by the function.
  uint32_t _memVarTotal;                 //!< Count of bytes used by variables.
  uint32_t _memStackTotal;               //!< Count of bytes used by stack.
  uint32_t _memAllTotal;                 //!< Count of bytes used by variables and stack after alignment.

  uint32_t _annotationLength;            //!< Default length of an annotated instruction.
  RAState* _state;                       //!< Current RA state.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_REGALLOC_P_H
