// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_COMPILERCONTEXT_P_H
#define _ASMJIT_BASE_COMPILERCONTEXT_P_H

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
      tiedTotal(tiedTotal) {}

  BitArray* liveness;                    //!< Liveness bits (populated by liveness-analysis).
  RAState* state;                        //!< Optional saved \ref RAState.
  uint32_t tiedTotal;                    //!< Total count of \ref TiedReg regs.
};

// ============================================================================
// [asmjit::RAState]
// ============================================================================

//! Variables' state.
struct RAState {};

// ============================================================================
// [asmjit::RAContext]
// ============================================================================

//! \internal
//!
//! Register allocator used by CodeCompiler.
struct RAContext {
  ASMJIT_NO_COPY(RAContext)

  typedef void (ASMJIT_CDECL* TraceNodeFunc)(RAContext* self, CBNode* node_, const char* prefix);

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  RAContext(CodeCompiler* compiler);
  virtual ~RAContext();

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the whole context.
  virtual void reset(bool releaseMemory = false);

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get compiler.
  ASMJIT_INLINE CodeCompiler* getCompiler() const { return _compiler; }

  //! Get function.
  ASMJIT_INLINE CCFunc* getFunc() const { return _func; }
  //! Get stop node.
  ASMJIT_INLINE CBNode* getStop() const { return _stop; }

  //! Get start of the current scope.
  ASMJIT_INLINE CBNode* getStart() const { return _start; }
  //! Get end of the current scope.
  ASMJIT_INLINE CBNode* getEnd() const { return _end; }

  //! Get extra block.
  ASMJIT_INLINE CBNode* getExtraBlock() const { return _extraBlock; }
  //! Set extra block.
  ASMJIT_INLINE void setExtraBlock(CBNode* node) { _extraBlock = node; }

  // --------------------------------------------------------------------------
  // [Error]
  // --------------------------------------------------------------------------

  //! Get the last error code.
  ASMJIT_INLINE Error getLastError() const {
    return getCompiler()->getLastError();
  }

  //! Set the last error code and propagate it through the error handler.
  ASMJIT_INLINE Error setLastError(Error error, const char* message = nullptr) {
    return getCompiler()->setLastError(error, message);
  }

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
      _tmpAllocator.allocZeroed(static_cast<size_t>(len) * BitArray::kEntitySize));
  }

  ASMJIT_INLINE BitArray* copyBits(const BitArray* src, uint32_t len) {
    return static_cast<BitArray*>(
      _tmpAllocator.dup(src, static_cast<size_t>(len) * BitArray::kEntitySize));
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
    PodList<CBNode*>::Link* link = _tmpAllocator.allocT<PodList<CBNode*>::Link>();
    if (!link) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

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
    PodList<CBNode*>::Link* link = _tmpAllocator.allocT<PodList<CBNode*>::Link>();
    if (!link) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

    link->setValue(node);
    _returningList.append(link);

    return kErrorOk;
  }

  //! Add jump-flow data to the jcc flow list.
  ASMJIT_INLINE Error addJccNode(CBNode* node) {
    PodList<CBNode*>::Link* link = _tmpAllocator.allocT<PodList<CBNode*>::Link>();
    if (!link) return setLastError(DebugUtils::errored(kErrorNoHeapMemory));

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
  // [Cleanup]
  // --------------------------------------------------------------------------

  virtual void cleanup();

  // --------------------------------------------------------------------------
  // [Compile]
  // --------------------------------------------------------------------------

  virtual Error compile(CCFunc* func);

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CodeHolder* _code;                     //!< CodeHolder (shortcut).
  CodeCompiler* _compiler;               //!< CodeCompiler.
  CCFunc* _func;                         //!< Function being processed.

  Zone _tmpAllocator;                    //!< RA temporary allocator.
  TraceNodeFunc _traceNode;              //!< Only non-null if ASMJIT_TRACE is enabled.

  //! \internal
  //!
  //! Offset (how many bytes to add) to `VarMap` to get `TiedReg` array. Used
  //! by liveness analysis shared across all backends. This is needed because
  //! `VarMap` is a base class for a specialized version that liveness analysis
  //! doesn't use, it just needs `TiedReg` array.
  uint32_t _varMapToVaListOffset;

  CBNode* _start;                        //!< Start of the current active scope.
  CBNode* _end;                          //!< End of the current active scope.

  CBNode* _extraBlock;                   //!< Node that is used to insert extra code after the function body.
  CBNode* _stop;                         //!< Stop node.

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
#endif // _ASMJIT_BASE_COMPILERCONTEXT_P_H
