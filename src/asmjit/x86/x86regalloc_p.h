// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86REGALLOC_P_H
#define _ASMJIT_X86_X86REGALLOC_P_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/codecompiler.h"
#include "../base/regalloc_p.h"
#include "../base/utils.h"
#include "../x86/x86assembler.h"
#include "../x86/x86compiler.h"
#include "../x86/x86logging.h"
#include "../x86/x86misc.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86RAData]
// ============================================================================

struct X86RAData : public RAData {
  ASMJIT_INLINE X86RAData(uint32_t tiedTotal) noexcept : RAData(tiedTotal) {
    inRegs.reset();
    outRegs.reset();
    clobberedRegs.reset();
    tiedIndex.reset();
    tiedCount.reset();
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get TiedReg array.
  ASMJIT_INLINE TiedReg* getTiedArray() const noexcept {
    return const_cast<TiedReg*>(tiedArray);
  }

  //! Get TiedReg array for a given register class `rc`.
  ASMJIT_INLINE TiedReg* getTiedArrayByRC(uint32_t rc) const noexcept {
    return const_cast<TiedReg*>(tiedArray) + tiedIndex.get(rc);
  }

  //! Get TiedReg index for a given register class `rc`.
  ASMJIT_INLINE uint32_t getTiedStart(uint32_t rc) const noexcept {
    return tiedIndex.get(rc);
  }

  //! Get TiedReg count for a given register class `rc`.
  ASMJIT_INLINE uint32_t getTiedCountByRC(uint32_t rc) const noexcept {
    return tiedCount.get(rc);
  }

  //! Get TiedReg at the specified `index`.
  ASMJIT_INLINE TiedReg* getTiedAt(uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < tiedTotal);
    return getTiedArray() + index;
  }

  //! Get TiedReg at the specified index for a given register class `rc`.
  ASMJIT_INLINE TiedReg* getTiedAtByRC(uint32_t rc, uint32_t index) const noexcept {
    ASMJIT_ASSERT(index < tiedCount._regs[rc]);
    return getTiedArrayByRC(rc) + index;
  }

  ASMJIT_INLINE void setTiedAt(uint32_t index, TiedReg& tied) noexcept {
    ASMJIT_ASSERT(index < tiedTotal);
    tiedArray[index] = tied;
  }

  // --------------------------------------------------------------------------
  // [Utils]
  // --------------------------------------------------------------------------

  //! Find TiedReg.
  ASMJIT_INLINE TiedReg* findTied(VirtReg* vreg) const {
    TiedReg* tiedArray = getTiedArray();
    uint32_t tiedCount = tiedTotal;

    for (uint32_t i = 0; i < tiedCount; i++)
      if (tiedArray[i].vreg == vreg)
        return &tiedArray[i];

    return nullptr;
  }

  //! Find TiedReg (by class).
  ASMJIT_INLINE TiedReg* findTiedByRC(uint32_t rc, VirtReg* vreg) const {
    TiedReg* tiedArray = getTiedArrayByRC(rc);
    uint32_t tiedCount = getTiedCountByRC(rc);

    for (uint32_t i = 0; i < tiedCount; i++)
      if (tiedArray[i].vreg == vreg)
        return &tiedArray[i];

    return nullptr;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Special registers on input.
  //!
  //! Special register(s) restricted to one or more physical register. If there
  //! is more than one special register it means that we have to duplicate the
  //! variable content to all of them (it means that the same varible was used
  //! by two or more operands). We forget about duplicates after the register
  //! allocation finishes and marks all duplicates as non-assigned.
  X86RegMask inRegs;

  //! Special registers on output.
  //!
  //! Special register(s) used on output. Each variable can have only one
  //! special register on the output, 'X86RAData' contains all registers from
  //! all 'TiedReg's.
  X86RegMask outRegs;

  //! Clobbered registers (by a function call).
  X86RegMask clobberedRegs;

  //! Start indexes of `TiedReg`s per register class.
  X86RegCount tiedIndex;
  //! Count of variables per register class.
  X86RegCount tiedCount;

  //! Linked registers.
  TiedReg tiedArray[1];
};

// ============================================================================
// [asmjit::X86StateCell]
// ============================================================================

//! X86/X64 state-cell.
union X86StateCell {
  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t getState() const {
    return _state;
  }

  ASMJIT_INLINE void setState(uint32_t state) {
    _state = static_cast<uint8_t>(state);
  }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset() { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _packed;

  struct {
    uint8_t _state : 2;
    uint8_t _unused : 6;
  };
};

// ============================================================================
// [asmjit::X86RAState]
// ============================================================================

//! X86/X64 state.
struct X86RAState : RAState {
  enum {
    //! Base index of GP registers.
    kGpIndex = 0,
    //! Count of GP registers.
    kGpCount = 16,

    //! Base index of MMX registers.
    kMmIndex = kGpIndex + kGpCount,
    //! Count of Mm registers.
    kMmCount = 8,

    //! Base index of XMM registers.
    kXmmIndex = kMmIndex + kMmCount,
    //! Count of XMM registers.
    kXmmCount = 16,

    //! Count of all registers in `X86RAState`.
    kAllCount = kXmmIndex + kXmmCount
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE VirtReg** getList() {
    return _list;
  }

  ASMJIT_INLINE VirtReg** getListByRC(uint32_t rc) {
    switch (rc) {
      case X86Reg::kClassGp : return _listGp;
      case X86Reg::kClassMm : return _listMm;
      case X86Reg::kClassXyz: return _listXmm;

      default:
        return nullptr;
    }
  }

  // --------------------------------------------------------------------------
  // [Clear]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void reset(size_t numCells) {
    ::memset(this, 0, kAllCount * sizeof(VirtReg*) +
                      2         * sizeof(X86RegMask) +
                      numCells  * sizeof(X86StateCell));
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    //! List of all allocated variables in one array.
    VirtReg* _list[kAllCount];

    struct {
      //! Allocated GP registers.
      VirtReg* _listGp[kGpCount];
      //! Allocated MMX registers.
      VirtReg* _listMm[kMmCount];
      //! Allocated XMM registers.
      VirtReg* _listXmm[kXmmCount];
    };
  };

  //! Occupied registers (mask).
  X86RegMask _occupied;
  //! Modified registers (mask).
  X86RegMask _modified;

  //! Variables data, the length is stored in `X86RAPass`.
  X86StateCell _cells[1];
};

// ============================================================================
// [asmjit::X86RAPass]
// ============================================================================

#if defined(ASMJIT_DEBUG)
# define ASMJIT_X86_CHECK_STATE _checkState();
#else
# define ASMJIT_X86_CHECK_STATE
#endif // ASMJIT_DEBUG

//! \internal
//!
//! X86 register allocator pipeline.
//!
//! Takes care of generating function prologs and epilogs, and also performs
//! register allocation.
class X86RAPass : public RAPass {
public:
  ASMJIT_NO_COPY(X86RAPass)
  typedef RAPass Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  X86RAPass() noexcept;
  virtual ~X86RAPass() noexcept;

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  virtual Error process(CodeBuilder* cb, Zone* zone) noexcept override;
  virtual Error prepare(CCFunc* func) noexcept override;

  // --------------------------------------------------------------------------
  // [Arch]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool isX64() const { return _zsp.getSize() == 16; }
  ASMJIT_INLINE uint32_t getGpSize() const { return _zsp.getSize(); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get compiler as `X86Compiler`.
  ASMJIT_INLINE X86Compiler* cc() const { return static_cast<X86Compiler*>(_cc); }
  //! Get function as `X86Func`.
  ASMJIT_INLINE X86Func* getFunc() const { return static_cast<X86Func*>(_func); }
  //! Get clobbered registers (global).
  ASMJIT_INLINE uint32_t getClobberedRegs(uint32_t rc) { return _clobberedRegs.get(rc); }

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86RAData* newRAData(uint32_t tiedTotal) noexcept {
    return new(_zone->alloc(sizeof(X86RAData) + tiedTotal * sizeof(TiedReg))) X86RAData(tiedTotal);
  }

  // --------------------------------------------------------------------------
  // [Emit]
  // --------------------------------------------------------------------------

  void emitLoad(VirtReg* vreg, uint32_t physId, const char* reason);
  void emitSave(VirtReg* vreg, uint32_t physId, const char* reason);
  void emitMove(VirtReg* vreg, uint32_t toPhysId, uint32_t fromPhysId, const char* reason);
  void emitSwapGp(VirtReg* aVReg, VirtReg* bVReg, uint32_t aId, uint32_t bId, const char* reason);

  void emitPushSequence(uint32_t regMask);
  void emitPopSequence(uint32_t regMask);

  void emitConvertVarToVar(uint32_t dstType, uint32_t dstIndex, uint32_t srcType, uint32_t srcIndex);
  void emitMoveVarOnStack(uint32_t dstType, const X86Mem* dst, uint32_t srcType, uint32_t srcIndex);
  void emitMoveImmOnStack(uint32_t dstType, const X86Mem* dst, const Imm* src);

  void emitMoveImmToReg(uint32_t dstType, uint32_t dstIndex, const Imm* src);

  // --------------------------------------------------------------------------
  // [Register Management]
  // --------------------------------------------------------------------------

  void _checkState();

  // --------------------------------------------------------------------------
  // [Attach / Detach]
  // --------------------------------------------------------------------------

  //! Attach.
  //!
  //! Attach a register to the 'VirtReg', changing 'VirtReg' members to show
  //! that the variable is currently alive and linking variable with the
  //! current 'X86RAState'.
  template<int C>
  ASMJIT_INLINE void attach(VirtReg* vreg, uint32_t physId, bool modified) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(physId != kInvalidReg);

    // Prevent Esp allocation if C==Gp.
    ASMJIT_ASSERT(C != X86Reg::kClassGp || physId != X86Gp::kIdSp);

    uint32_t regMask = Utils::mask(physId);

    vreg->setState(VirtReg::kStateReg);
    vreg->setModified(modified);
    vreg->setPhysId(physId);
    vreg->addHomeId(physId);

    _x86State.getListByRC(C)[physId] = vreg;
    _x86State._occupied.or_(C, regMask);
    _x86State._modified.or_(C, static_cast<uint32_t>(modified) << physId);

    ASMJIT_X86_CHECK_STATE
  }

  //! Detach.
  //!
  //! The opposite of 'Attach'. Detach resets the members in 'VirtReg'
  //! (physId, state and changed flags) and unlinks the variable with the
  //! current 'X86RAState'.
  template<int C>
  ASMJIT_INLINE void detach(VirtReg* vreg, uint32_t physId, uint32_t vState) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(vreg->getPhysId() == physId);
    ASMJIT_ASSERT(vState != VirtReg::kStateReg);

    uint32_t regMask = Utils::mask(physId);

    vreg->setState(vState);
    vreg->resetPhysId();
    vreg->setModified(false);

    _x86State.getListByRC(C)[physId] = nullptr;
    _x86State._occupied.andNot(C, regMask);
    _x86State._modified.andNot(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Rebase]
  // --------------------------------------------------------------------------

  //! Rebase.
  //!
  //! Change the register of the 'VirtReg' changing also the current 'X86RAState'.
  //! Rebase is nearly identical to 'Detach' and 'Attach' sequence, but doesn't
  //! change the `VirtReg`s modified flag.
  template<int C>
  ASMJIT_INLINE void rebase(VirtReg* vreg, uint32_t newPhysId, uint32_t oldPhysId) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);

    uint32_t newRegMask = Utils::mask(newPhysId);
    uint32_t oldRegMask = Utils::mask(oldPhysId);
    uint32_t bothRegMask = newRegMask ^ oldRegMask;

    vreg->setPhysId(newPhysId);

    _x86State.getListByRC(C)[oldPhysId] = nullptr;
    _x86State.getListByRC(C)[newPhysId] = vreg;

    _x86State._occupied.xor_(C, bothRegMask);
    _x86State._modified.xor_(C, bothRegMask & -static_cast<int32_t>(vreg->isModified()));

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Load / Save]
  // --------------------------------------------------------------------------

  //! Load.
  //!
  //! Load variable from its memory slot to a register, emitting 'Load'
  //! instruction and changing the variable state to allocated.
  template<int C>
  ASMJIT_INLINE void load(VirtReg* vreg, uint32_t physId) {
    // Can be only called if variable is not allocated.
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(vreg->getState() != VirtReg::kStateReg);
    ASMJIT_ASSERT(vreg->getPhysId() == kInvalidReg);

    emitLoad(vreg, physId, "Load");
    attach<C>(vreg, physId, false);

    ASMJIT_X86_CHECK_STATE
  }

  //! Save.
  //!
  //! Save the variable into its home location, but keep it as allocated.
  template<int C>
  ASMJIT_INLINE void save(VirtReg* vreg) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(vreg->getState() == VirtReg::kStateReg);
    ASMJIT_ASSERT(vreg->getPhysId() != kInvalidReg);

    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = Utils::mask(physId);

    emitSave(vreg, physId, "Save");

    vreg->setModified(false);
    _x86State._modified.andNot(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Move / Swap]
  // --------------------------------------------------------------------------

  //! Move a register.
  //!
  //! Move register from one index to another, emitting 'Move' if needed. This
  //! function does nothing if register is already at the given index.
  template<int C>
  ASMJIT_INLINE void move(VirtReg* vreg, uint32_t newPhysId) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(vreg->getState() == VirtReg::kStateReg);
    ASMJIT_ASSERT(vreg->getPhysId() != kInvalidReg);

    uint32_t oldPhysId = vreg->getPhysId();
    if (newPhysId != oldPhysId) {
      emitMove(vreg, newPhysId, oldPhysId, "Move");
      rebase<C>(vreg, newPhysId, oldPhysId);
    }

    ASMJIT_X86_CHECK_STATE
  }

  //! Swap two registers
  //!
  //! It's only possible to swap Gp registers.
  ASMJIT_INLINE void swapGp(VirtReg* aVReg, VirtReg* bVReg) {
    ASMJIT_ASSERT(aVReg != bVReg);

    ASMJIT_ASSERT(aVReg->getRegClass() == X86Reg::kClassGp);
    ASMJIT_ASSERT(aVReg->getState() == VirtReg::kStateReg);
    ASMJIT_ASSERT(aVReg->getPhysId() != kInvalidReg);

    ASMJIT_ASSERT(bVReg->getRegClass() == X86Reg::kClassGp);
    ASMJIT_ASSERT(bVReg->getState() == VirtReg::kStateReg);
    ASMJIT_ASSERT(bVReg->getPhysId() != kInvalidReg);

    uint32_t aIndex = aVReg->getPhysId();
    uint32_t bIndex = bVReg->getPhysId();

    emitSwapGp(aVReg, bVReg, aIndex, bIndex, "Swap");

    aVReg->setPhysId(bIndex);
    bVReg->setPhysId(aIndex);

    _x86State.getListByRC(X86Reg::kClassGp)[aIndex] = bVReg;
    _x86State.getListByRC(X86Reg::kClassGp)[bIndex] = aVReg;

    uint32_t m = aVReg->isModified() ^ bVReg->isModified();
    _x86State._modified.xor_(X86Reg::kClassGp, (m << aIndex) | (m << bIndex));

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Alloc / Spill]
  // --------------------------------------------------------------------------

  //! Alloc.
  template<int C>
  ASMJIT_INLINE void alloc(VirtReg* vreg, uint32_t physId) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(physId != kInvalidReg);

    uint32_t oldPhysId = vreg->getPhysId();
    uint32_t oldState = vreg->getState();
    uint32_t regMask = Utils::mask(physId);

    ASMJIT_ASSERT(_x86State.getListByRC(C)[physId] == nullptr || physId == oldPhysId);

    if (oldState != VirtReg::kStateReg) {
      if (oldState == VirtReg::kStateMem)
        emitLoad(vreg, physId, "Alloc");
      vreg->setModified(false);
    }
    else if (oldPhysId != physId) {
      emitMove(vreg, physId, oldPhysId, "Alloc");

      _x86State.getListByRC(C)[oldPhysId] = nullptr;
      regMask ^= Utils::mask(oldPhysId);
    }
    else {
      ASMJIT_X86_CHECK_STATE
      return;
    }

    vreg->setState(VirtReg::kStateReg);
    vreg->setPhysId(physId);
    vreg->addHomeId(physId);

    _x86State.getListByRC(C)[physId] = vreg;
    _x86State._occupied.xor_(C, regMask);
    _x86State._modified.xor_(C, regMask & -static_cast<int32_t>(vreg->isModified()));

    ASMJIT_X86_CHECK_STATE
  }

  //! Spill.
  //!
  //! Spill variable/register, saves the content to the memory-home if modified.
  template<int C>
  ASMJIT_INLINE void spill(VirtReg* vreg) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);

    if (vreg->getState() != VirtReg::kStateReg) {
      ASMJIT_X86_CHECK_STATE
      return;
    }

    uint32_t physId = vreg->getPhysId();
    ASMJIT_ASSERT(physId != kInvalidReg);
    ASMJIT_ASSERT(_x86State.getListByRC(C)[physId] == vreg);

    if (vreg->isModified())
      emitSave(vreg, physId, "Spill");
    detach<C>(vreg, physId, VirtReg::kStateMem);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Modify]
  // --------------------------------------------------------------------------

  template<int C>
  ASMJIT_INLINE void modify(VirtReg* vreg) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);

    uint32_t physId = vreg->getPhysId();
    uint32_t regMask = Utils::mask(physId);

    vreg->setModified(true);
    _x86State._modified.or_(C, regMask);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [Unuse]
  // --------------------------------------------------------------------------

  //! Unuse.
  //!
  //! Unuse variable, it will be detached it if it's allocated then its state
  //! will be changed to VirtReg::kStateNone.
  template<int C>
  ASMJIT_INLINE void unuse(VirtReg* vreg, uint32_t vState = VirtReg::kStateNone) {
    ASMJIT_ASSERT(vreg->getRegClass() == C);
    ASMJIT_ASSERT(vState != VirtReg::kStateReg);

    uint32_t physId = vreg->getPhysId();
    if (physId != kInvalidReg)
      detach<C>(vreg, physId, vState);
    else
      vreg->setState(vState);

    ASMJIT_X86_CHECK_STATE
  }

  // --------------------------------------------------------------------------
  // [State]
  // --------------------------------------------------------------------------

  //! Get state as `X86RAState`.
  ASMJIT_INLINE X86RAState* getState() const { return const_cast<X86RAState*>(&_x86State); }

  virtual void loadState(RAState* src) override;
  virtual RAState* saveState() override;

  virtual void switchState(RAState* src) override;
  virtual void intersectStates(RAState* a, RAState* b) override;

  // --------------------------------------------------------------------------
  // [Memory]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86Mem getVarMem(VirtReg* vreg) {
    (void)getVarCell(vreg);
    return X86Mem(Init,
      cc()->_nativeGpReg.getRegType(), vreg->getId(),
      Reg::kRegNone, kInvalidValue,
      0, 0, Mem::kFlagIsRegHome);
  }

  // --------------------------------------------------------------------------
  // [Fetch]
  // --------------------------------------------------------------------------

  virtual Error fetch() override;

  // --------------------------------------------------------------------------
  // [Annotate]
  // --------------------------------------------------------------------------

  virtual Error annotate() override;

  // --------------------------------------------------------------------------
  // [Translate]
  // --------------------------------------------------------------------------

  virtual Error translate() override;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Count of X86/X64 registers.
  X86RegCount _regCount;
  //! X86/X64 stack-pointer (esp or rsp).
  X86Gp _zsp;
  //! X86/X64 frame-pointer (ebp or rbp).
  X86Gp _zbp;

  //! X86/X64 specific compiler state, linked to `_state`.
  X86RAState _x86State;
  //! Clobbered registers (for the whole function).
  X86RegMask _clobberedRegs;

  //! Memory cell where is stored address used to restore manually
  //! aligned stack.
  RACell* _stackFrameCell;

  //! Global allocable registers mask.
  uint32_t _gaRegs[X86Reg::kClassCount];

  //! Function arguments base pointer (register).
  uint8_t _argBaseReg;
  //! Function variables base pointer (register).
  uint8_t _varBaseReg;

  //! Function arguments base offset.
  int32_t _argBaseOffset;
  //! Function variables base offset.
  int32_t _varBaseOffset;

  //! Function arguments displacement.
  int32_t _argActualDisp;
  //! Function variables displacement.
  int32_t _varActualDisp;

  //! Temporary string builder used for logging.
  StringBuilderTmp<256> _stringBuilder;

#if !defined(ASMJIT_DISABLE_LOGGING)
  X86Formatter _formatter;
#endif // ASMJIT_DISABLE_LOGGING
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_X86_X86REGALLOC_P_H
