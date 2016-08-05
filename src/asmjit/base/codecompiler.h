// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_CODECOMPILER_H
#define _ASMJIT_BASE_CODECOMPILER_H

#include "../build.h"
#if !defined(ASMJIT_DISABLE_COMPILER)

// [Dependencies]
#include "../base/assembler.h"
#include "../base/codebuilder.h"
#include "../base/constpool.h"
#include "../base/containers.h"
#include "../base/func.h"
#include "../base/operand.h"
#include "../base/utils.h"
#include "../base/zone.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct VirtReg;
struct TiedReg;
struct RAState;
struct RACell;

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::ConstScope]
// ============================================================================

//! Scope of the constant.
ASMJIT_ENUM(ConstScope) {
  //! Local constant, always embedded right after the current function.
  kConstScopeLocal = 0,
  //! Global constant, embedded at the end of the currently compiled code.
  kConstScopeGlobal = 1
};

// ============================================================================
// [asmjit::VirtReg]
// ============================================================================

//! Virtual register data (CodeCompiler).
struct VirtReg {
  //! A state of a virtual register (used during register allocation).
  ASMJIT_ENUM(State) {
    kStateNone = 0,                      //!< Not allocated, not used.
    kStateReg = 1,                       //!< Allocated in register.
    kStateMem = 2                        //!< Allocated in memory or spilled.
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the virtual-register id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }

  //! Get if the virtual-register has a local id.
  ASMJIT_INLINE bool hasLocalId() const { return _localId != kInvalidValue; }
  //! Get virtual-register's local-id (used by RA).
  ASMJIT_INLINE uint32_t getLocalId() const noexcept { return _localId; }
  //! Set virtual-register's local id.
  ASMJIT_INLINE void setLocalId(uint32_t localId) { _localId = localId; }
  //! Reset virtual-register's local id.
  ASMJIT_INLINE void resetLocalId() { _localId = kInvalidValue; }

  //! Get virtual-register's name.
  ASMJIT_INLINE const char* getName() const noexcept { return _name; }
  //! Get virtual-register's size.
  ASMJIT_INLINE uint32_t getSize() const { return _size; }
  //! Get virtual-register's alignment.
  ASMJIT_INLINE uint32_t getAlignment() const { return _alignment; }

  ASMJIT_INLINE uint32_t getSignature() const { return _regInfo.signature; }
  //! Get a physical register type.
  ASMJIT_INLINE uint32_t getRegType() const { return _regInfo.regType; }
  //! Get a physical register class.
  ASMJIT_INLINE uint32_t getRegClass() const { return _regInfo.regClass; }
  //! Get a virtual type.
  ASMJIT_INLINE uint32_t getTypeId() const { return _typeId; }

  //! Get the virtual-register  priority, used by compiler to decide which variable to spill.
  ASMJIT_INLINE uint32_t getPriority() const { return _priority; }
  //! Set the virtual-register  priority.
  ASMJIT_INLINE void setPriority(uint32_t priority) {
    ASMJIT_ASSERT(priority <= 0xFF);
    _priority = static_cast<uint8_t>(priority);
  }

  //! Get variable state, only used by `RAPipeline`.
  ASMJIT_INLINE uint32_t getState() const { return _state; }
  //! Set variable state, only used by `RAPipeline`.
  ASMJIT_INLINE void setState(uint32_t state) {
    ASMJIT_ASSERT(state <= 0xFF);
    _state = static_cast<uint8_t>(state);
  }

  //! Get register index.
  ASMJIT_INLINE uint32_t getPhysId() const { return _physId; }
  //! Set register index.
  ASMJIT_INLINE void setPhysId(uint32_t physId) {
    ASMJIT_ASSERT(physId <= kInvalidReg);
    _physId = static_cast<uint8_t>(physId);
  }
  //! Reset register index.
  ASMJIT_INLINE void resetPhysId() {
    _physId = static_cast<uint8_t>(kInvalidReg);
  }

  //! Get home registers mask.
  ASMJIT_INLINE uint32_t getHomeMask() const { return _homeMask; }
  //! Add a home register index to the home registers mask.
  ASMJIT_INLINE void addHomeId(uint32_t physId) { _homeMask |= Utils::mask(physId); }

  //! Get whether the VirtReg is only memory allocated on the stack.
  ASMJIT_INLINE bool isStack() const { return static_cast<bool>(_isStack); }
  //! Get whether the variable is a function argument passed through memory.
  ASMJIT_INLINE bool isMemArg() const { return static_cast<bool>(_isMemArg); }

  //! Get variable content can be calculated by a simple instruction.
  ASMJIT_INLINE bool isCalculated() const { return static_cast<bool>(_isCalculated); }
  //! Get whether to save variable when it's unused (spill).
  ASMJIT_INLINE bool saveOnUnuse() const { return static_cast<bool>(_saveOnUnuse); }

  //! Get whether the variable was changed.
  ASMJIT_INLINE bool isModified() const { return static_cast<bool>(_modified); }
  //! Set whether the variable was changed.
  ASMJIT_INLINE void setModified(bool modified) { _modified = modified; }

  //! Get home memory offset.
  ASMJIT_INLINE int32_t getMemOffset() const { return _memOffset; }
  //! Set home memory offset.
  ASMJIT_INLINE void setMemOffset(int32_t offset) { _memOffset = offset; }

  //! Get home memory cell.
  ASMJIT_INLINE RACell* getMemCell() const { return _memCell; }
  //! Set home memory cell.
  ASMJIT_INLINE void setMemCell(RACell* cell) { _memCell = cell; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _id;                          //!< Virtual-register id.
  uint32_t _localId;                     //!< Virtual-register local-id (used by RA).
  const char* _name;                     //!< Virtual-register name.

  Operand::RegInfo _regInfo;             //!< Register info & signature.

  uint32_t _typeId;                      //!< Virtual type-id.
  uint8_t _priority;                     //!< Allocation priority.

  uint8_t _state;                        //!< Variable state (connected with actual `RAState)`.
  uint8_t _physId;                       //!< Actual register index (only used by `RAPipeline)`, during translate.

  uint8_t _isStack : 1;                  //!< Whether the variable is only used as memory allocated on the stack.
  uint8_t _isMemArg : 1;                 //!< Whether the variable is a function argument passed through memory.

  //! Whether variable content can be calculated by a simple instruction.
  //!
  //! This is used mainly by MMX and SSE2 code. This flag indicates that
  //! register allocator should never reserve memory for this variable, because
  //! the content can be generated by a single instruction (for example PXOR).
  uint8_t _isCalculated : 1;
  uint8_t _saveOnUnuse : 1;              //!< Save on unuse (at end of the variable scope).
  uint8_t _modified : 1;                 //!< Whether variable was changed (connected with actual `RAState)`.
  uint8_t _reserved0 : 3;                //!< \internal
  uint8_t _alignment;                    //!< Variable's natural alignment.

  uint32_t _size;                        //!< Variable size.
  uint32_t _homeMask;                    //!< Mask of all registers variable has been allocated to.

  int32_t _memOffset;                    //!< Home memory offset.
  RACell* _memCell;                      //!< Home memory cell, used by `RAPipeline` (initially nullptr).

  // --------------------------------------------------------------------------
  // [Members - Temporary Usage]
  // --------------------------------------------------------------------------

  // These variables are only used during register allocation. They are
  // initialized by init() phase and reset by cleanup() phase.

  //! Temporary link to TiedReg* used by the `RAPipeline` used in
  //! various phases, but always set back to nullptr when finished.
  //!
  //! This temporary data is designed to be used by algorithms that need to
  //! store some data into variables themselves during compilation. But it's
  //! expected that after variable is compiled & translated the data is set
  //! back to zero/null. Initial value is nullptr.
  TiedReg* _tied;
};

// ============================================================================
// [asmjit::TiedReg]
// ============================================================================

//! Tied register (CodeCompiler)
//!
//! Tied register is used to describe one ore more register operands that share
//! the same virtual register. Tied register contains all the data that is
//! essential for register allocation.
struct TiedReg {
  //! Flags.
  ASMJIT_ENUM(Flags) {
    kRReg        = 0x00000001U,          //!< Register read.
    kWReg        = 0x00000002U,          //!< Register write.
    kXReg        = 0x00000003U,          //!< Register read-write.

    kRMem        = 0x00000004U,          //!< Memory read.
    kWMem        = 0x00000008U,          //!< Memory write.
    kXMem        = 0x0000000CU,          //!< Memory read-write.

    kRDecide     = 0x00000010U,          //!< RA can decide between reg/mem read.
    kWDecide     = 0x00000020U,          //!< RA can decide between reg/mem write.
    kXDecide     = 0x00000030U,          //!< RA can decide between reg/mem read-write.

    kRConv       = 0x00000040U,          //!< Variable is converted to other type/class on the input.
    kWConv       = 0x00000080U,          //!< Variable is converted from other type/class on the output.
    kXConv       = 0x000000C0U,          //!< Combination of `kRConv` and `kWConv`.

    kRFunc       = 0x00000100U,          //!< Function argument passed in register.
    kWFunc       = 0x00000200U,          //!< Function return value passed into register.
    kXFunc       = 0x00000300U,          //!< Function argument and return value.
    kRCall       = 0x00000400U,          //!< Function call operand.

    kSpill       = 0x00000800U,          //!< Variable should be spilled.
    kUnuse       = 0x00001000U,          //!< Variable should be unused at the end of the instruction/node.

    kRAll        = kRReg | kRMem | kRDecide | kRFunc | kRCall, //!< All in-flags.
    kWAll        = kWReg | kWMem | kWDecide | kWFunc,          //!< All out-flags.

    kRDone       = 0x00400000U,          //!< Already allocated on the input.
    kWDone       = 0x00800000U,          //!< Already allocated on the output.

    kX86GpbLo    = 0x10000000U,
    kX86GpbHi    = 0x20000000U,
    kX86Fld4     = 0x40000000U,
    kX86Fld8     = 0x80000000U
  };

  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setup(VirtReg* vreg, uint32_t flags = 0, uint32_t inRegs = 0, uint32_t allocableRegs = 0) noexcept {
    this->vreg = vreg;
    this->flags = flags;
    this->refCount = 0;
    this->inPhysId = kInvalidReg;
    this->outPhysId = kInvalidReg;
    this->reserved = 0;
    this->inRegs = inRegs;
    this->allocableRegs = allocableRegs;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get whether the variable has to be allocated in a specific input register.
  ASMJIT_INLINE uint32_t hasInPhysId() const { return inPhysId != kInvalidReg; }
  //! Get whether the variable has to be allocated in a specific output register.
  ASMJIT_INLINE uint32_t hasOutPhysId() const { return outPhysId != kInvalidReg; }

  //! Set the input register index.
  ASMJIT_INLINE void setInPhysId(uint32_t index) { inPhysId = static_cast<uint8_t>(index); }
  //! Set the output register index.
  ASMJIT_INLINE void setOutPhysId(uint32_t index) { outPhysId = static_cast<uint8_t>(index); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE TiedReg& operator=(const TiedReg& other) {
    ::memcpy(this, &other, sizeof(TiedReg));
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Pointer to the associated \ref VirtReg.
  VirtReg* vreg;
  //! Linked register flags.
  uint32_t flags;

  union {
    struct {
      //! How many times the variable is used by the instruction/node.
      uint8_t refCount;
      //! Input register index or `kInvalidReg` if it's not given.
      //!
      //! Even if the input register index is not given (i.e. it may by any
      //! register), register allocator should assign an index that will be
      //! used to persist a variable into this specific index. It's helpful
      //! in situations where one variable has to be allocated in multiple
      //! registers to determine the register which will be persistent.
      uint8_t inPhysId;
      //! Output register index or `kInvalidReg` if it's not given.
      //!
      //! Typically `kInvalidReg` if variable is only used on input.
      uint8_t outPhysId;
      //! \internal
      uint8_t reserved;
    };

    //! \internal
    //!
    //! Packed data #0.
    uint32_t packed;
  };

  //! Mandatory input registers.
  //!
  //! Mandatory input registers are required by the instruction even if
  //! there are duplicates. This schema allows us to allocate one variable
  //! in one or more register when needed. Required mostly by instructions
  //! that have implicit register operands (imul, cpuid, ...) and function
  //! call.
  uint32_t inRegs;

  //! Allocable input registers.
  //!
  //! Optional input registers is a mask of all allocable registers for a given
  //! variable where we have to pick one of them. This mask is usually not used
  //! when _inRegs is set. If both masks are used then the register
  //! allocator tries first to find an intersection between these and allocates
  //! an extra slot if not found.
  uint32_t allocableRegs;
};

// ============================================================================
// [asmjit::CCHint]
// ============================================================================

//! Hint for register allocator (CodeCompiler).
class CCHint : public CBNode {
public:
  ASMJIT_NO_COPY(CCHint)

  //! Hint type.
  ASMJIT_ENUM(Hint) {
    //! Alloc to physical reg.
    kHintAlloc = 0,
    //! Spill to memory.
    kHintSpill = 1,
    //! Save if modified.
    kHintSave = 2,
    //! Save if modified and mark it as unused.
    kHintSaveAndUnuse = 3,
    //! Mark as unused.
    kHintUnuse = 4
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCHint` instance.
  ASMJIT_INLINE CCHint(CodeBuilder* cb, VirtReg* vreg, uint32_t hint, uint32_t value) noexcept : CBNode(cb, kNodeHint) {
    orFlags(kFlagIsRemovable | kFlagIsInformative);
    _vreg = vreg;
    _hint = hint;
    _value = value;
  }

  //! Destroy the `CCHint` instance.
  ASMJIT_INLINE ~CCHint() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get variable.
  ASMJIT_INLINE VirtReg* getVReg() const noexcept { return _vreg; }

  //! Get hint it, see \ref Hint.
  ASMJIT_INLINE uint32_t getHint() const noexcept { return _hint; }
  //! Set hint it, see \ref Hint.
  ASMJIT_INLINE void setHint(uint32_t hint) noexcept { _hint = hint; }

  //! Get hint value.
  ASMJIT_INLINE uint32_t getValue() const noexcept { return _value; }
  //! Set hint value.
  ASMJIT_INLINE void setValue(uint32_t value) noexcept { _value = value; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Variable.
  VirtReg* _vreg;
  //! Hint id.
  uint32_t _hint;
  //! Value.
  uint32_t _value;
};

// ============================================================================
// [asmjit::CCFunc]
// ============================================================================

//! Function entry (CodeCompiler).
class CCFunc : public CBLabel {
public:
  ASMJIT_NO_COPY(CCFunc)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCFunc` instance.
  //!
  //! Always use `CodeCompiler::addFunc()` to create \ref CCFunc.
  ASMJIT_INLINE CCFunc(CodeBuilder* cb) noexcept
    : CBLabel(cb),
      _exitNode(nullptr),
      _decl(nullptr),
      _end(nullptr),
      _args(nullptr),
      _funcHints(Utils::mask(kFuncHintNaked)),
      _funcFlags(0),
      _naturalStackAlignment(0),
      _requiredStackAlignment(0),
      _redZoneSize(0),
      _spillZoneSize(0),
      _argStackSize(0),
      _memStackSize(0),
      _callStackSize(0) {
    _type = kNodeFunc;
  }

  //! Destroy the `CCFunc` instance.
  ASMJIT_INLINE ~CCFunc() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function exit `CBLabel`.
  ASMJIT_INLINE CBLabel* getExitNode() const noexcept { return _exitNode; }
  //! Get function exit label.
  ASMJIT_INLINE Label getExitLabel() const noexcept { return _exitNode->getLabel(); }

  //! Get the function end sentinel.
  ASMJIT_INLINE CBSentinel* getEnd() const noexcept { return _end; }
  //! Get function declaration.
  ASMJIT_INLINE FuncDecl* getDecl() const noexcept { return _decl; }

  //! Get arguments count.
  ASMJIT_INLINE uint32_t getNumArgs() const noexcept { return _decl->getNumArgs(); }
  //! Get arguments list.
  ASMJIT_INLINE VirtReg** getArgs() const noexcept { return _args; }

  //! Get argument at `i`.
  ASMJIT_INLINE VirtReg* getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < getNumArgs());
    return _args[i];
  }

  //! Set argument at `i`.
  ASMJIT_INLINE void setArg(uint32_t i, VirtReg* vreg) noexcept {
    ASMJIT_ASSERT(i < getNumArgs());
    _args[i] = vreg;
  }

  //! Reset argument at `i`.
  ASMJIT_INLINE void resetArg(uint32_t i) noexcept {
    ASMJIT_ASSERT(i < getNumArgs());
    _args[i] = nullptr;
  }

  //! Get function hints.
  ASMJIT_INLINE uint32_t getFuncHints() const noexcept { return _funcHints; }
  //! Get function flags.
  ASMJIT_INLINE uint32_t getFuncFlags() const noexcept { return _funcFlags; }

  //! Get whether the _funcFlags has `flag`
  ASMJIT_INLINE bool hasFuncFlag(uint32_t flag) const noexcept { return (_funcFlags & flag) != 0; }
  //! Set function `flag`.
  ASMJIT_INLINE void addFuncFlags(uint32_t flags) noexcept { _funcFlags |= flags; }
  //! Clear function `flag`.
  ASMJIT_INLINE void clearFuncFlags(uint32_t flags) noexcept { _funcFlags &= ~flags; }

  //! Get whether the function is naked.
  ASMJIT_INLINE bool isNaked() const noexcept { return hasFuncFlag(kFuncFlagIsNaked); }
  //! Get whether the function is also a caller.
  ASMJIT_INLINE bool isCaller() const noexcept { return hasFuncFlag(kFuncFlagIsCaller); }
  //! Get whether the required stack alignment is lower than expected one,
  //! thus it has to be aligned manually.
  ASMJIT_INLINE bool isStackMisaligned() const noexcept { return hasFuncFlag(kFuncFlagIsStackMisaligned); }
  //! Get whether the stack pointer is adjusted inside function prolog/epilog.
  ASMJIT_INLINE bool isStackAdjusted() const noexcept { return hasFuncFlag(kFuncFlagIsStackAdjusted); }

  //! Get whether the function is finished.
  ASMJIT_INLINE bool isFinished() const noexcept { return hasFuncFlag(kFuncFlagIsFinished); }

  //! Get expected stack alignment.
  ASMJIT_INLINE uint32_t getNaturalStackAlignment() const noexcept { return _naturalStackAlignment; }
  //! Set expected stack alignment.
  ASMJIT_INLINE void setNaturalStackAlignment(uint32_t alignment) noexcept { _naturalStackAlignment = alignment; }

  //! Get required stack alignment.
  ASMJIT_INLINE uint32_t getRequiredStackAlignment() const noexcept { return _requiredStackAlignment; }
  //! Set required stack alignment.
  ASMJIT_INLINE void setRequiredStackAlignment(uint32_t alignment) noexcept { _requiredStackAlignment = alignment; }

  //! Update required stack alignment so it's not lower than expected
  //! stack alignment.
  ASMJIT_INLINE void updateRequiredStackAlignment() noexcept {
    if (_requiredStackAlignment <= _naturalStackAlignment) {
      _requiredStackAlignment = _naturalStackAlignment;
      clearFuncFlags(kFuncFlagIsStackMisaligned);
    }
    else {
      addFuncFlags(kFuncFlagIsStackMisaligned);
    }
  }

  //! Get red-zone size.
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! set red-zone size.
  ASMJIT_INLINE void setRedZoneSize(uint32_t s) noexcept { _redZoneSize = static_cast<uint16_t>(s); }

  //! Get spill-zone size.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }
  //! Set spill-zone size.
  ASMJIT_INLINE void setSpillZoneSize(uint32_t s) noexcept { _spillZoneSize = static_cast<uint16_t>(s); }

  //! Get stack size used by function arguments.
  ASMJIT_INLINE uint32_t getArgStackSize() const noexcept { return _argStackSize; }
  //! Get stack size used by variables and memory allocated on the stack.
  ASMJIT_INLINE uint32_t getMemStackSize() const noexcept { return _memStackSize; }

  //! Get stack size used by function calls.
  ASMJIT_INLINE uint32_t getCallStackSize() const noexcept { return _callStackSize; }
  //! Merge stack size used by function call with `s`.
  ASMJIT_INLINE void mergeCallStackSize(uint32_t s) noexcept { if (_callStackSize < s) _callStackSize = s; }

  // --------------------------------------------------------------------------
  // [Hints]
  // --------------------------------------------------------------------------

  //! Set function hint.
  ASMJIT_INLINE void setHint(uint32_t hint, uint32_t value) noexcept {
    ASMJIT_ASSERT(hint <= 31);
    ASMJIT_ASSERT(value <= 1);

    _funcHints &= ~(1     << hint);
    _funcHints |=  (value << hint);
  }

  //! Get function hint.
  ASMJIT_INLINE uint32_t getHint(uint32_t hint) const noexcept {
    ASMJIT_ASSERT(hint <= 31);
    return (_funcHints >> hint) & 0x1;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FuncDecl* _decl;                       //!< Function declaration.
  CBLabel* _exitNode;                   //!< Function exit.
  CBSentinel* _end;                     //!< Function end.

  VirtReg** _args;                       //!< Arguments array as `VirtReg`.

  uint32_t _funcHints;                   //!< Function hints;
  uint32_t _funcFlags;                   //!< Function flags.

  uint32_t _naturalStackAlignment;       //!< Natural stack alignment (OS/ABI).
  uint32_t _requiredStackAlignment;      //!< Required stack alignment.

  uint16_t _redZoneSize;                 //!< Red-zone size (AMD64-ABI).
  uint16_t _spillZoneSize;               //!< Spill-zone size (WIN64-ABI).

  uint32_t _argStackSize;                //!< Stack size needed for function arguments.
  uint32_t _memStackSize;                //!< Stack size needed for all variables and memory allocated on the stack.
  uint32_t _callStackSize;               //!< Stack size needed to call other functions.
};

// ============================================================================
// [asmjit::CCFuncRet]
// ============================================================================

//! AIR function return.
class CCFuncRet : public CBNode {
public:
  ASMJIT_NO_COPY(CCFuncRet)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCFuncRet` instance.
  ASMJIT_INLINE CCFuncRet(CodeBuilder* cb, const Operand_& o0, const Operand_& o1) noexcept : CBNode(cb, kNodeFuncExit) {
    orFlags(kFlagIsRet);
    _ret[0].copyFrom(o0);
    _ret[1].copyFrom(o1);
  }

  //! Destroy the `CCFuncRet` instance.
  ASMJIT_INLINE ~CCFuncRet() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the first return operand.
  ASMJIT_INLINE Operand& getFirst() noexcept { return static_cast<Operand&>(_ret[0]); }
  //! \overload
  ASMJIT_INLINE const Operand& getFirst() const noexcept { return static_cast<const Operand&>(_ret[0]); }

  //! Get the second return operand.
  ASMJIT_INLINE Operand& getSecond() noexcept { return static_cast<Operand&>(_ret[1]); }
   //! \overload
  ASMJIT_INLINE const Operand& getSecond() const noexcept { return static_cast<const Operand&>(_ret[1]); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Return operands.
  Operand_ _ret[2];
};

// ============================================================================
// [asmjit::CCCall]
// ============================================================================

//! Function call (CodeCompiler).
class CCCall : public CBInst {
public:
  ASMJIT_NO_COPY(CCCall)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCCall` instance.
  ASMJIT_INLINE CCCall(CodeBuilder* cb, uint32_t instId, uint32_t options, Operand* opArray, uint32_t opCount) noexcept
    : CBInst(cb, instId, options, opArray, opCount),
      _decl(nullptr),
      _args(nullptr) {

    _type = kNodeCall;
    _ret[0].reset();
    _ret[1].reset();
    orFlags(kFlagIsRemovable);
  }

  //! Destroy the `CCCall` instance.
  ASMJIT_INLINE ~CCCall() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get function declaration.
  ASMJIT_INLINE FuncDecl* getDecl() const noexcept { return _decl; }

  //! Get target operand.
  ASMJIT_INLINE Operand& getTarget() noexcept { return static_cast<Operand&>(_opArray[0]); }
  //! \overload
  ASMJIT_INLINE const Operand& getTarget() const noexcept { return static_cast<const Operand&>(_opArray[0]); }

  //! Get return at `i`.
  ASMJIT_INLINE Operand& getRet(uint32_t i = 0) noexcept {
    ASMJIT_ASSERT(i < 2);
    return static_cast<Operand&>(_ret[i]);
  }
  //! \overload
  ASMJIT_INLINE const Operand& getRet(uint32_t i = 0) const noexcept {
    ASMJIT_ASSERT(i < 2);
    return static_cast<const Operand&>(_ret[i]);
  }

  //! Get argument at `i`.
  ASMJIT_INLINE Operand& getArg(uint32_t i) noexcept {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return static_cast<Operand&>(_args[i]);
  }
  //! \overload
  ASMJIT_INLINE const Operand& getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < kFuncArgCountLoHi);
    return static_cast<const Operand&>(_args[i]);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  FuncDecl* _decl;                       //!< Function declaration.
  Operand_ _ret[2];                      //!< Return.
  Operand_* _args;                       //!< Arguments.
};

// ============================================================================
// [asmjit::CCPushArg]
// ============================================================================

//! Push argument before a function call (CodeCompiler).
class CCPushArg : public CBNode {
public:
  ASMJIT_NO_COPY(CCPushArg)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CCPushArg` instance.
  ASMJIT_INLINE CCPushArg(CodeBuilder* cb, CCCall* call, VirtReg* src, VirtReg* cvt) noexcept
    : CBNode(cb, kNodePushArg),
      _call(call),
      _src(src),
      _cvt(cvt),
      _args(0) {
    orFlags(kFlagIsRemovable);
  }

  //! Destroy the `CCPushArg` instance.
  ASMJIT_INLINE ~CCPushArg() noexcept {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the associated function-call.
  ASMJIT_INLINE CCCall* getCall() const noexcept { return _call; }
  //! Get source variable.
  ASMJIT_INLINE VirtReg* getSrcReg() const noexcept { return _src; }
  //! Get conversion variable.
  ASMJIT_INLINE VirtReg* getCvtReg() const noexcept { return _cvt; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CCCall* _call;                         //!< Associated `CCCall`.
  VirtReg* _src;                         //!< Source variable.
  VirtReg* _cvt;                         //!< Temporary variable used for conversion (or null).
  uint32_t _args;                        //!< Affected arguments bit-array.
};

// ============================================================================
// [asmjit::CodeCompiler]
// ============================================================================

//! Code emitter that uses virtual registers and performs register allocation.
//!
//! Compiler is a high-level code-generation tool that provides register
//! allocation and automatic handling of function calling conventions. It was
//! primarily designed for merging multiple parts of code into a function
//! without worrying about registers and function calling conventions.
//!
//! CodeCompiler can be used, with a minimum effort, to handle 32-bit and 64-bit
//! code at the same time.
//!
//! CodeCompiler is based on CodeBuilder and contains all the features it
//! provides. It means that the code it stores can be modified (removed, added,
//! injected) and analyzed. When the code is finalized the compiler can emit
//! the code into an Assembler to translate the abstract representation into a
//! machine code.
class ASMJIT_VIRTAPI CodeCompiler : public CodeBuilder {
public:
  ASMJIT_NO_COPY(CodeCompiler)
  typedef CodeBuilder Base;

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new `CodeCompiler` instance.
  ASMJIT_API CodeCompiler() noexcept;
  //! Destroy the `CodeCompiler` instance.
  ASMJIT_API virtual ~CodeCompiler() noexcept;

  // --------------------------------------------------------------------------
  // [Events]
  // --------------------------------------------------------------------------

  ASMJIT_API virtual Error onAttach(CodeHolder* code) noexcept override;
  ASMJIT_API virtual Error onDetach(CodeHolder* code) noexcept override;

  // --------------------------------------------------------------------------
  // [Compiler Features]
  // --------------------------------------------------------------------------

  //! Get maximum look ahead.
  ASMJIT_INLINE uint32_t getMaxLookAhead() const noexcept {
    return _maxLookAhead;
  }
  //! Set maximum look ahead to `val`.
  ASMJIT_INLINE void setMaxLookAhead(uint32_t val) noexcept {
    _maxLookAhead = val;
  }

  // --------------------------------------------------------------------------
  // [Node-Factory]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Create a new `CCHint`.
  ASMJIT_API CCHint* newHintNode(Reg& reg, uint32_t hint, uint32_t value) noexcept;

  // --------------------------------------------------------------------------
  // [Func]
  // --------------------------------------------------------------------------

  //! Add a function `node` to the stream.
  ASMJIT_API CCFunc* addFunc(CCFunc* func);
  //! Get current function.
  ASMJIT_INLINE CCFunc* getFunc() const noexcept { return _func; }

  // --------------------------------------------------------------------------
  // [Hint]
  // --------------------------------------------------------------------------

  //! Emit a new hint (purely informational node).
  ASMJIT_API Error _hint(Reg& reg, uint32_t hint, uint32_t value);

  // --------------------------------------------------------------------------
  // [Vars]
  // --------------------------------------------------------------------------

  //! Create a new virtual register.
  ASMJIT_API VirtReg* newVirtReg(const VirtType& typeInfo, const char* name) noexcept;

  //! Get whether the virtual register `r` is valid.
  ASMJIT_INLINE bool isVirtRegValid(const Reg& reg) const noexcept {
    return isVirtRegValid(reg.getId());
  }
  //! \overload
  ASMJIT_INLINE bool isVirtRegValid(uint32_t id) const noexcept {
    size_t index = Operand::unpackId(id);
    return index < _vRegArray.getLength();
  }

  //! Get \ref VirtReg associated with the given `r`.
  ASMJIT_INLINE VirtReg* getVirtReg(const Reg& reg) const noexcept { return getVirtRegById(reg.getId()); }
  //! Get \ref VirtReg associated with the given `id`.
  ASMJIT_INLINE VirtReg* getVirtRegById(uint32_t id) const noexcept {
    ASMJIT_ASSERT(id != kInvalidValue);
    size_t index = Operand::unpackId(id);

    ASMJIT_ASSERT(index < _vRegArray.getLength());
    return _vRegArray[index];
  }

  //! Get an array of all virtual registers managed by CodeCompiler.
  ASMJIT_INLINE const PodVector<VirtReg*>& getVirtRegArray() const noexcept { return _vRegArray; }

  //! Alloc a virtual register `reg`.
  ASMJIT_API Error alloc(Reg& reg);
  //! Alloc a virtual register `reg` using `physId` as a register id.
  ASMJIT_API Error alloc(Reg& reg, uint32_t physId);
  //! Alloc a virtual register `reg` using `ref` as a register operand.
  ASMJIT_API Error alloc(Reg& reg, const Reg& ref);
  //! Spill a virtual register `reg`.
  ASMJIT_API Error spill(Reg& reg);
  //! Save a virtual register `reg` if the status is `modified` at this point.
  ASMJIT_API Error save(Reg& reg);
  //! Unuse a virtual register `reg`.
  ASMJIT_API Error unuse(Reg& reg);

  //! Get priority of a virtual register `reg`.
  ASMJIT_API uint32_t getPriority(Reg& reg) const;
  //! Set priority of variable `reg` to `priority`.
  ASMJIT_API void setPriority(Reg& reg, uint32_t priority);

  //! Get save-on-unuse `reg` property.
  ASMJIT_API bool getSaveOnUnuse(Reg& reg) const;
  //! Set save-on-unuse `reg` property to `value`.
  ASMJIT_API void setSaveOnUnuse(Reg& reg, bool value);

  //! Rename variable `reg` to `name`.
  //!
  //! NOTE: Only new name will appear in the logger.
  ASMJIT_API void rename(Reg& reg, const char* fmt, ...);

  // --------------------------------------------------------------------------
  // [Stack]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Create a new memory chunk allocated on the current function's stack.
  virtual Error _newStack(Mem& m, uint32_t size, uint32_t alignment, const char* name) = 0;

  // --------------------------------------------------------------------------
  // [Const]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Put data to a constant-pool and get a memory reference to it.
  virtual Error _newConst(Mem& m, uint32_t scope, const void* data, size_t size) = 0;

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  const uint32_t* _typeIdMap;            //!< Mapping between arch-independent type-id and backend specific one.
  uint32_t _maxLookAhead;                //!< Maximum look-ahead of RA.

  CCFunc* _func;                         //!< Current function.

  Zone _vRegAllocator;                   //!< Allocates \ref VirtReg objects.
  PodVector<VirtReg*> _vRegArray;        //!< Stores array of \ref VirtReg pointers.

  CBConstPool* _localConstPool;          //!< Local constant pool, flushed at the end of each function.
  CBConstPool* _globalConstPool;         //!< Global constant pool, flushed at the end of the compilation.
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // !ASMJIT_DISABLE_COMPILER
#endif // _ASMJIT_BASE_CODECOMPILER_H
