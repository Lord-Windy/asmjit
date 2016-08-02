// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_OPERAND_H
#define _ASMJIT_BASE_OPERAND_H

// [Dependencies]
#include "../base/archinfo.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::Operand_]
// ============================================================================

//! Constructor-less \ref Operand.
//!
//! Contains no initialization code and can be used safely to define an array
//! of operands that won't be initialized. This is a \ref Operand compatible
//! data structure designed to be statically initialized or `static const`.
struct Operand_ {
  // --------------------------------------------------------------------------
  // [Operand Type]
  // --------------------------------------------------------------------------

  //! Operand types that can be encoded in \ref Operand.
  ASMJIT_ENUM(OpType) {
    kOpNone         = 0,                 //!< Not an operand or not initialized.
    kOpReg          = 1,                 //!< Operand is a register.
    kOpMem          = 2,                 //!< Operand is a memory.
    kOpImm          = 3,                 //!< Operand is an immediate value.
    kOpLabel        = 4                  //!< Operand is a label.
  };

  // --------------------------------------------------------------------------
  // [Operand Id]
  // --------------------------------------------------------------------------

  //! Operand id helpers useful for id <-> index translation.
  ASMJIT_ENUM(PackedId) {
    kPackedIdMin    = 0x00000100U,                //!< Minimum valid packed-id.
    kPackedIdMax    = 0xFFFFFFFEU,                //!< Maximum valid packed-id.
    kPackedIdCount  = kPackedIdMax - kPackedIdMin //!< Count of valid packed-ids.
  };

  //! Get if the given `id` is a valid packed-id that can be used by Operand.
  //! Packed ids are those equal or greater than `kPackedIdMin` and lesser or
  //! equal to `kPackedIdMax`. This concept was created to support virtual
  //! registers and to make them distinguishable from physical ones. It allows
  //! a single uint32_t to contain either physical register id or virtual
  //! register id represented as `packed-id`. This concept is used also for
  //! labels to make the API consistent.
  static ASMJIT_INLINE bool isPackedId(uint32_t id) noexcept { return id - kPackedIdMin <= kPackedIdCount; }
  //! Convert a real-id into a packed-id that can be stored in Operand.
  static ASMJIT_INLINE uint32_t packId(uint32_t id) noexcept { return id + kPackedIdMin; }
  //! Convert a packed-id back to real-id.
  static ASMJIT_INLINE uint32_t unpackId(uint32_t id) noexcept { return id - kPackedIdMin; }

  // --------------------------------------------------------------------------
  // [Operand Utils]
  // --------------------------------------------------------------------------

  static ASMJIT_INLINE uint32_t makeSignature(uint32_t opType, uint32_t subType, uint32_t payload, uint32_t size) noexcept {
    return Utils::pack32_4x8(opType, subType, payload, size);
  }

  static ASMJIT_INLINE uint32_t makeRegSignature(uint32_t regType, uint32_t regClass, uint32_t regSize) noexcept {
    return Utils::pack32_4x8(kOpReg, regType, regClass, regSize);
  }

  // --------------------------------------------------------------------------
  // [Operand Data]
  // --------------------------------------------------------------------------

  //! Any operand.
  struct AnyData {
    uint8_t op;                          //!< Type of the operand (see \ref OpType).
    uint8_t subType;                     //!< Subtype - depends on `op`.
    uint8_t payload;                     //!< Payload - depends on `op`.
    uint8_t size;                        //!< Size of the operand (register, address, immediate).
    uint32_t id;                         //!< Operand id or `kInvalidValue`.
    uint32_t reserved8_4;                //!< \internal
    uint32_t reserved12_4;               //!< \internal
  };

  //! Register operand data.
  struct RegData {
    uint8_t op;                          //!< Type of the operand (always \ref kOpReg).
    uint8_t regType;                     //!< Register type.
    uint8_t regClass;                    //!< Register class.
    uint8_t size;                        //!< Size of the register.
    uint32_t id;                         //!< Physical or virtual register id.
    uint32_t typeId;                     //!< Virtual type (optional).
    uint32_t reserved12_4;               //!< \internal
  };

  //! Structure that contains register signature only, used internally.
  union RegInfo {
    struct {
      uint8_t op;                        //!< Type of the operand (always \ref kOpReg).
      uint8_t regType;                   //!< Register type.
      uint8_t regClass;                  //!< Register class.
      uint8_t size;                      //!< Size of the register.
    };
    uint32_t signature;
  };

  //! Memory operand data.
  struct MemData {
    uint8_t op;                          //!< Type of the operand (always \ref kOpMem).
    uint8_t baseIndexType;               //!< Type of BASE and INDEX registers.
    uint8_t flags;                       //!< Architecture dependent flags.
    uint8_t size;                        //!< Size of the memory operand.
    uint32_t index;                      //!< INDEX register id or `kInvalidValue`.

    // [BASE + OFF32] vs just [OFF64].
    union {
      uint64_t offset64;                 //!< 64-bit offset, combining low and high 32-bit parts.
      struct {
#if ASMJIT_ARCH_LE
        uint32_t offsetLo32;             //!< 32-bit low offset part.
        uint32_t base;                   //!< 32-bit high offset part or BASE.
#else
        uint32_t base;                   //!< 32-bit high offset part or BASE.
        uint32_t offsetLo32;             //!< 32-bit low offset part.
#endif
      };
    };
  };

  //! Immediate operand data.
  struct ImmData {
    uint8_t op;                          //!< Type of the operand (always \ref kOpImm).
    uint8_t reserved_1_1;                //!< Must be zero.
    uint8_t reserved_2_1;                //!< Must be zero.
    uint8_t size;                        //!< Size of the immediate (or 0 to autodetect).
    uint32_t id;                         //!< Immediate id (always `kInvalidValue`).
    union {
      int8_t   _i8[8];                   //!< 8x8-bit signed integers.
      uint8_t  _u8[8];                   //!< 8x8-bit unsigned integers.
      int16_t  _i16[4];                  //!< 4x16-bit signed integers.
      uint16_t _u16[4];                  //!< 4x16-bit unsigned integers.
      int32_t  _i32[2];                  //!< 2x32-bit signed integers.
      uint32_t _u32[2];                  //!< 2x32-bit unsigned integers.
      int64_t  _i64[1];                  //!< 1x64-bit signed integers
      uint64_t _u64[1];                  //!< 1x64-bit unsigned integers.
      float    _f32[2];                  //!< 2x32-bit floating points.
      double   _f64[1];                  //!< 1x64-bit floating points.
    } value;
  };

  //! Label operand data.
  struct LabelData {
    uint8_t op;                          //!< Type of the operand (always \ref kOpLabel).
    uint8_t reserved_1_1;                //!< Must be zero.
    uint8_t reserved_2_1;                //!< Must be zero.
    uint8_t size;                        //!< Must be zero.
    uint32_t id;                         //!< Label id (`kInvalidValue` if not initialized).
    uint32_t reserved8_4;                //!< \internal
    uint32_t reserved12_4;               //!< \internal
  };

  // --------------------------------------------------------------------------
  // [Init & Copy]
  // --------------------------------------------------------------------------

  //! \internal
  //!
  //! Initialize the operand to `other` (used by constructors).
  ASMJIT_INLINE void _init(const Operand_& other) noexcept { ::memcpy(this, &other, sizeof(Operand_)); }

  //! \internal
  ASMJIT_INLINE void _initReg(uint32_t signature, uint32_t id) {
    _init_packed_d0_d1(signature, id);
    _init_packed_d2_d3(0, 0);
  }

  //! \internal
  ASMJIT_INLINE void _init_packed_op_b1_b2_sz_id(uint32_t op, uint32_t r0, uint32_t r1, uint32_t size, uint32_t id) noexcept {
    // This hack is not for performance, it's to decrease the size of the binary
    // generated when constructing AsmJit operands (mostly for third parties).
    // Some compilers are not able to join four BYTE writes to a single DWORD
    // write. Because the `op`, `r0`, `r1`, and `size` arguments are usually compile
    // time constants the compiler can do a really nice job if they are joined
    // by using bitwise operations.
    _packed[0].setPacked_2x32(makeSignature(op, r0, r1, size), id);
  }

  //! \internal
  ASMJIT_INLINE void _init_packed_d0_d1(uint32_t d0, uint32_t d1) noexcept { _packed[0].setPacked_2x32(d0, d1); }
  //! \internal
  ASMJIT_INLINE void _init_packed_d2_d3(uint32_t d2, uint32_t d3) noexcept { _packed[1].setPacked_2x32(d2, d3); }

  //! \internal
  //!
  //! Initialize the operand from `other` (used by operator overloads).
  ASMJIT_INLINE void copyFrom(const Operand_& other) noexcept { ::memcpy(this, &other, sizeof(Operand_)); }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get if the operand matches the given signature `sign`.
  ASMJIT_INLINE bool hasSignature(uint32_t signature) const noexcept { return _signature == signature; }

  //! Get if the operand matches a signature composed of `opType`, `subType`, `payload`, and `size`,
  ASMJIT_INLINE bool hasSignature(uint32_t opType, uint32_t subType, uint32_t payload, uint32_t size) const noexcept {
    return _signature == makeSignature(opType, subType, payload, size);
  }
  //! Get if the operand matches a signature of the `other` operand.
  ASMJIT_INLINE bool hasSignature(const Operand_& other) const noexcept {
    return _signature == other.getSignature();
  }

  //! Get a 32-bit operand signature.
  //!
  //! Signature is first 4 bytes of the operand data. It's used mostly for
  //! operand checking as it's much faster to check 4 bytes at once than having
  //! to check these bytes individually.
  ASMJIT_INLINE uint32_t getSignature() const noexcept { return _signature; }
  //! Set the operand signature (see \ref getSignature).
  //!
  //! Improper use of `setSignature()` can lead to hard-to-debug errors.
  ASMJIT_INLINE void setSignature(uint32_t signature) noexcept { _signature = signature; }
  //! \overload
  ASMJIT_INLINE void setSignature(uint32_t opType, uint32_t subType, uint32_t payload, uint32_t size) noexcept {
    _signature = makeSignature(opType, subType, payload, size);
  }

  //! Get type of the operand, see \ref OpType.
  ASMJIT_INLINE uint32_t getOp() const noexcept { return _any.op; }
  //! Get if the operand is none (\ref kOpNone).
  ASMJIT_INLINE bool isNone() const noexcept { return _any.op == kOpNone; }
  //! Get if the operand is a register (\ref kOpReg).
  ASMJIT_INLINE bool isReg() const noexcept { return _any.op == kOpReg; }
  //! Get if the operand is a memory location (\ref kOpMem).
  ASMJIT_INLINE bool isMem() const noexcept { return _any.op == kOpMem; }
  //! Get if the operand is an immediate (\ref kOpImm).
  ASMJIT_INLINE bool isImm() const noexcept { return _any.op == kOpImm; }
  //! Get if the operand is a label (\ref kOpLabel).
  ASMJIT_INLINE bool isLabel() const noexcept { return _any.op == kOpLabel; }

  //! Get if the operand is a physical register.
  ASMJIT_INLINE bool isPhysReg() const noexcept { return isReg() && _reg.id < kInvalidReg; }
  //! Get if the operand is a virtual register.
  ASMJIT_INLINE bool isVirtReg() const noexcept { return isReg() && isPackedId(_reg.id); }

  //! Get if the operand specifies a size (i.e. the size is not zero).
  ASMJIT_INLINE bool hasSize() const noexcept { return _any.size != 0; }
  //! Get if the size of the operand matches `size`.
  ASMJIT_INLINE bool hasSize(uint32_t size) const noexcept { return _any.size == size; }

  //! Get size of the operand (in bytes).
  //!
  //! The value returned depends on the operand type:
  //!   * None  - Should always return zero size.
  //!   * Reg   - Should always return the size of the register. If the register
  //!             size depends on architecture (like \ref X86CReg and \ref X86DReg)
  //!             the size returned should be the greatest possible (so it should
  //!             return 64-bit size in such case).
  //!   * Mem   - Size is optional and will be in most cases zero.
  //!   * Imm   - Should always return zero size.
  //!   * Label - Should always return zero size.
  ASMJIT_INLINE uint32_t getSize() const noexcept { return _any.size; }

  //! Get the operand id.
  //!
  //! The value returned should be interpreted accordingly to the operand type:
  //!   * None  - Should be `kInvalidValue`.
  //!   * Reg   - Physical or virtual register id.
  //!   * Mem   - Multiple meanings - BASE address (register or label id), or
  //!             high value of a 64-bit absolute address.
  //!   * Imm   - Should be `kInvalidValue`.
  //!   * Label - Label id if it was created by using `newLabel()` or `kInvalidValue`
  //!             if the label is invalid or uninitialized.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _any.id; }

  //! Get if the operand is 100% equal to `other`.
  ASMJIT_INLINE bool isEqual(const Operand_& other) const noexcept {
    return (_packed[0] == other._packed[0]) &
           (_packed[1] == other._packed[1]) ;
  }

  //! Get if the operand is a register matching `regType`.
  ASMJIT_INLINE bool isReg(uint32_t regType) const noexcept {
    const uint32_t kMsk = makeSignature(0xFF  , 0xFF   , 0, 0);
    const uint32_t kTag = makeSignature(kOpReg, regType, 0, 0);
    return (_signature & kMsk) == kTag;
  }

  //! Get whether the operand is register and of `type` and `id`.
  ASMJIT_INLINE bool isReg(uint32_t regType, uint32_t id) const noexcept {
    return isReg(regType) && getId() == id;
  }

  //! Get whether the operand is a register or memory.
  ASMJIT_INLINE bool isRegOrMem() const noexcept {
    ASMJIT_ASSERT(kOpMem - kOpReg == 1);
    return _any.op >= kOpReg && _any.op <= kOpMem;
  }

  template<typename T>
  ASMJIT_INLINE T& getData() noexcept { return reinterpret_cast<T&>(_any); }
  template<typename T>
  ASMJIT_INLINE const T& getData() const noexcept { return reinterpret_cast<const T&>(_any); }

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset the `Operand` to none.
  //!
  //! None operand is defined the following way:
  //!   - Its signature is zero (kOpNone, and the rest zero as well).
  //!   - Its id is `kInvalidValue`.
  //!   - The reserved8_4 field is set to `kInvalidValue`.
  //!   - The reserved12_4 field is set to zero.
  //!
  //! Reset operand must match the Operand state right after its construction:
  //!
  //! ```
  //! using namespace asmjit;
  //!
  //! Operand a;
  //! Operand b;
  //! assert(a == b);
  //!
  //! b = x86::eax;
  //! assert(a != b);
  //!
  //! b.reset();
  //! assert(a == b);
  //! ```
  ASMJIT_INLINE void reset() noexcept {
    _init_packed_op_b1_b2_sz_id(kOpNone, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(0, 0);
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE bool operator==(const Operand_& other) const noexcept { return  isEqual(other); }
  ASMJIT_INLINE bool operator!=(const Operand_& other) const noexcept { return !isEqual(other); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    AnyData _any;                        //!< Base data.
    RegData _reg;                        //!< Register or variable data.
    MemData _mem;                        //!< Memory data.
    ImmData _imm;                        //!< Immediate data.
    LabelData _label;                    //!< Label data.

    uint32_t _signature;                 //!< Operand signature (first 32-bits).
    UInt64 _packed[2];                   //!< Operand packed into two 64-bit integers.
  };
};

// ============================================================================
// [asmjit::Operand]
// ============================================================================

//! Operand can contain register, memory location, immediate, or label.
class Operand : public Operand_ {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create an uninitialized operand.
  ASMJIT_INLINE Operand() noexcept { reset(); }
  //! Create a reference to `other` operand.
  ASMJIT_INLINE Operand(const Operand& other) noexcept { _init(other); }
  //! Create a reference to `other` operand.
  explicit ASMJIT_INLINE Operand(const Operand_& other) noexcept { _init(other); }
  //! Create a completely uninitialized operand (dangerous).
  explicit ASMJIT_INLINE Operand(const _NoInit&) noexcept {}

  // --------------------------------------------------------------------------
  // [Clone]
  // --------------------------------------------------------------------------

  //! Clone the `Operand`.
  ASMJIT_INLINE Operand clone() const noexcept { return Operand(*this); }

  ASMJIT_INLINE Operand& operator=(const Operand_& other) noexcept { copyFrom(other); return *this; }
};

// ============================================================================
// [asmjit::Label]
// ============================================================================

//! Label (jump target or data location).
//!
//! Label represents a location in code typically used as a jump target, but
//! may be also a reference to some data or a static variable. Label has to be
//! explicitly created by CodeEmitter.
//!
//! Example of using labels:
//!
//! ~~~
//! // Create a CodeEmitter (for example X86Assembler).
//! X86Assembler a;
//!
//! // Create Label instance.
//! Label L1 = a.newLabel();
//!
//! // ... your code ...
//!
//! // Using label.
//! a.jump(L1);
//!
//! // ... your code ...
//!
//! // Bind label to the current position, see `Assembler::bind()`.
//! a.bind(L1);
//! ~~~
class Label : public Operand {
public:
  enum {
    //! Label tag is used as a sub-type, forming a unique signature across all
    //! operand types as 0x1 is never associated with any register (reg-type).
    //! This means that a memory operand's BASE register can be constructed
    //! from virtually any operand (register vs. label) by just assigning its
    //! type (reg type or label-tag) and operand id.
    kLabelTag = 0x1
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create new, unassociated label.
  ASMJIT_INLINE Label() noexcept : Operand(NoInit) { reset(); }
  //! Create a reference to another label.
  ASMJIT_INLINE Label(const Label& other) noexcept : Operand(other) {}

  explicit ASMJIT_INLINE Label(uint32_t id) noexcept : Operand(NoInit) {
    _init_packed_op_b1_b2_sz_id(kOpLabel, 0, 0, 0, id);
    _init_packed_d2_d3(0, 0);
  }

  explicit ASMJIT_INLINE Label(const _NoInit&) noexcept : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  // TODO: I think that if operand is reset it shouldn't say it's a Label, it
  // should be none like all other operands.
  ASMJIT_INLINE void reset() noexcept {
    _init_packed_op_b1_b2_sz_id(kOpLabel, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(0, 0);
  }

  // --------------------------------------------------------------------------
  // [Label Specific]
  // --------------------------------------------------------------------------

  //! Get if the label was created by CodeEmitter and has an assigned id.
  ASMJIT_INLINE bool isValid() const noexcept { return _label.id != kInvalidValue; }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Label& operator=(const Label& other) noexcept { copyFrom(other); return *this; }
  ASMJIT_INLINE bool operator==(const Label& other) const noexcept { return _packed[0] == other._packed[0]; }
  ASMJIT_INLINE bool operator!=(const Label& other) const noexcept { return _packed[1] == other._packed[1]; }
};

// ============================================================================
// [asmjit::RegTraits]
// ============================================================================

//! Allows to resolve a signature of a register class `RegT` at compile-time.
//!
//! Must be provided by an architecture-specific implementation, ambiguous
//! registers like `Reg`, `X86Gp` and `X86Xyz` are not resolved by design.
template<typename RegT>
struct RegTraits {};

// ============================================================================
// [asmjit::Reg]
// ============================================================================

//! Physical/Virtual register operand.
class Reg : public Operand {
public:
  ASMJIT_ENUM(RegType) {
    kRegNone    = 0,                     //!< No register - unused, invalid, multiple meanings.
    _kRegStart  = 2,                     //!< Start of register types (must be honored).
    kRegRip     = _kRegStart             //!< Universal id of RIP register (if supported).
  };

  //! Register class.
  ASMJIT_ENUM(Class) {
    kClassGp = 0                         //!< GP register class, compatible with all architectures.
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a dummy register operand.
  ASMJIT_INLINE Reg() noexcept : Operand() {}
  //! Create a new register operand which is the same as `other` .
  ASMJIT_INLINE Reg(const Reg& other) noexcept : Operand(other) {}
  //! Create a new register operand compatible with `other`, but with a different `id`.
  ASMJIT_INLINE Reg(const Reg& other, uint32_t id) noexcept : Operand(NoInit) {
    _signature = other._signature;
    _reg.id = id;
    _packed[1] = other._packed[1];
  }

  //! Create a register initialized to `signature` and `id`.
  ASMJIT_INLINE Reg(const _Init&, uint32_t signature, uint32_t id) noexcept : Operand(NoInit) { _initReg(signature, id); }
  explicit ASMJIT_INLINE Reg(const _NoInit&) noexcept : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Reg Specific]
  // --------------------------------------------------------------------------

  //! Get if the register is valid (either virtual or physical).
  ASMJIT_INLINE bool isValid() const noexcept { return _reg.id != kInvalidValue; }
  //! Get if this is a physical register.
  ASMJIT_INLINE bool isPhysReg() const noexcept { return _reg.id < kInvalidReg; }
  //! Get if this is a virtual register (used by \ref CodeCompiler).
  ASMJIT_INLINE bool isVirtReg() const noexcept { return isPackedId(_reg.id); }

  using Operand_::isReg;

  //! Get if the register type matches `type`.
  ASMJIT_INLINE bool isReg(uint32_t regType) const noexcept { return _reg.regType == regType; }
  //! Get if the register type matches `type` and register id matches `id`.
  ASMJIT_INLINE bool isReg(uint32_t regType, uint32_t id) const noexcept { return isReg(regType) && getId() == id; }

  //! Get if this register is the same as `other`.
  //!
  //! NOTE: This is not the same as `isEqual(other)` as `isEqual()` checks
  //! the whole operand whereas `isSameReg()` only checks register signature
  //! and id. Registers can optionally contain a type annotation, which is
  //! not checked by `isSameReg()`.
  ASMJIT_INLINE bool isSameReg(const Reg& other) const noexcept { return _packed[0] == _packed[1]; }

  //! Get if this register has the same type as `other`.
  //!
  //! This is like `isSameReg`, but doesn't compare the register id.
  ASMJIT_INLINE bool isSameType(const Reg& other) const noexcept { return _signature == other._signature; }

  //! Get the register type.
  ASMJIT_INLINE uint32_t getRegType() const noexcept { return _reg.regType; }
  //! Get the register class.
  ASMJIT_INLINE uint32_t getRegClass() const noexcept { return _reg.regClass; }

  //! Get a virtual-type of the register.
  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _reg.typeId; }

  //! Cast this register to a non-ambiguous register type `T` keeping this register id and optional virtual type-id.
  //!
  //! NOTE: This can cast to an incompatible register, use with caution.
  template<typename RegT>
  ASMJIT_INLINE RegT as() const noexcept { return RegT(Init, RegTraits<RegT>::kSignature, getId()); }

  //! Cast this register to the same register as `other` keeping this register id and optional virtual type-id.
  //!
  //! NOTE: This can cast to an incompatible register, use with caution.
  template<typename RegT>
  ASMJIT_INLINE RegT as(const RegT& other) const noexcept { return RegT(Init, other.getSignature(), getId()); }

  //! Clone the register operand.
  ASMJIT_INLINE Reg clone() const noexcept { return Reg(*this); }

  //! Set the register id to `id`.
  ASMJIT_INLINE void setId(uint32_t id) noexcept { _reg.id = id; }
  //! Set the register type to `regType`.
  ASMJIT_INLINE void setTypeId(uint32_t typeId) noexcept { _reg.typeId = typeId; }

#define ASMJIT_DEFINE_ABSTRACT_REG(REG, BASE_REG)                              \
public:                                                                        \
  /*! Default constructor doesn't setup anything, it's like `Operand()`. */    \
  ASMJIT_INLINE REG() ASMJIT_NOEXCEPT                                          \
    : BASE_REG() {}                                                            \
                                                                               \
  /*! Copy the `other` REG register operand. */                                \
  ASMJIT_INLINE REG(const REG& other) ASMJIT_NOEXCEPT                          \
    : BASE_REG(other) {}                                                       \
                                                                               \
  /*! Copy the `other` REG register operand having its id set to `id` */       \
  ASMJIT_INLINE REG(const Reg& other, uint32_t id) ASMJIT_NOEXCEPT             \
    : BASE_REG(other, id) {}                                                   \
                                                                               \
  /*! Create a REG register operand based on `signature` and `id`. */          \
  ASMJIT_INLINE REG(const _Init& init, uint32_t signature, uint32_t id) ASMJIT_NOEXCEPT \
    : BASE_REG(init, signature, id) {}                                         \
                                                                               \
  /*! Create a completely uninitialized REG register operand (garbage). */     \
  explicit ASMJIT_INLINE REG(const _NoInit&) ASMJIT_NOEXCEPT                   \
    : BASE_REG(NoInit) {}                                                      \
                                                                               \
  /*! Clone the register operand. */                                           \
  ASMJIT_INLINE REG clone() const ASMJIT_NOEXCEPT { return REG(*this); }       \
                                                                               \
  ASMJIT_INLINE REG& operator=(const REG& other) ASMJIT_NOEXCEPT { copyFrom(other); return *this; } \
  ASMJIT_INLINE bool operator==(const REG& other) const ASMJIT_NOEXCEPT { return  isEqual(other); } \
  ASMJIT_INLINE bool operator!=(const REG& other) const ASMJIT_NOEXCEPT { return !isEqual(other); }

#define ASMJIT_DEFINE_FINAL_REG(REG, BASE_REG, TRAITS)                         \
  ASMJIT_DEFINE_ABSTRACT_REG(REG, BASE_REG)                                    \
                                                                               \
  /*! Create a REG register with `id`. */                                      \
  explicit ASMJIT_INLINE REG(uint32_t id) ASMJIT_NOEXCEPT                      \
    : BASE_REG(Init, kSignature, id) {}                                        \
                                                                               \
  enum {                                                                       \
    kThisType  = TRAITS::kType,                                                \
    kThisClass = TRAITS::kClass,                                               \
    kThisSize  = TRAITS::kSize,                                                \
    kSignature = TRAITS::kSignature                                            \
  };
};

// ============================================================================
// [asmjit::Mem]
// ============================================================================

//! Base class for all memory operands.
//!
//! NOTE: It's tricky to pack all possible cases that define a memory operand
//! into just 16 bytes. The `Mem` splits data into the following parts:
//!
//!   BASE - Base register or label - requires 36 bits total. 4 bits are used
//!     to encode the type of the BASE operand (label vs. register type) and
//!     the remaining 32 bits define the BASE id, which can be a physical or
//!     virtual register index. If BASE type is zero, which is never used as
//!     a register-type and label doesn't use it as well then BASE field
//!     contains a high DWORD of a possible 64-bit absolute address, which is
//!     possible on X64.
//!
//!   INDEX - Index register (or theoretically Label, which doesn't make sense).
//!     Encoding is similar to BASE - it also requires 36 bits and splits the
//!     encoding to INDEX type (4 bits defining the register type) and id (32-bits).
//!
//!   OFFSET - A relative offset of the address. Basically if BASE is specified
//!     the relative displacement adjusts BASE and an optional INDEX. if BASE is
//!     not specified then the OFFSET should be considered as ABSOLUTE address
//!     (at least on X86/X64). In that case its low 32 bits are stored in
//!     DISPLACEMENT field and the remaining high 32 bits are stored in BASE.
//!
//!   OTHER FIELDS - There is rest 8 bits that can be used for whatever purpose.
//!          The X86Mem operand uses these bits to store segment override
//!          prefix and index shift (scale).
class Mem : public Operand {
public:
  //! Memory operand flags.
  ASMJIT_ENUM(Flags) {
    //! Memory operand BASE register is virtual-register's home location, not
    //! the BASE register itself. This flag is designed for \ref CodeCompiler,
    //! which should lower such operands and clear the flag before it gets
    //! used by \ref Assembler.
    kFlagIsRegHome = 0x80
  };

  //! Helper constants to pack BASE and INDEX register types into `MemData::baseIndexType`.
  ASMJIT_ENUM(BaseIndexType) {
    kMemBaseTypeShift  = 0,
    kMemIndexTypeShift = 4
  };

  static ASMJIT_INLINE uint32_t encodeBaseIndex(uint32_t baseType, uint32_t indexType) noexcept {
    return (baseType << kMemBaseTypeShift) | (indexType << kMemIndexTypeShift);
  }

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Construct a default `Mem` operand, that points to [0].
  ASMJIT_INLINE Mem() noexcept : Operand(NoInit) { reset(); }
  ASMJIT_INLINE Mem(const Mem& other) noexcept : Operand(other) {}

  ASMJIT_INLINE Mem(const _Init&,
    uint32_t baseType, uint32_t baseId,
    uint32_t indexType, uint32_t indexId,
    int32_t off, uint32_t size, uint32_t flags) noexcept : Operand(NoInit) {

    uint32_t baseIndexType = encodeBaseIndex(baseType, indexType);
    _init_packed_op_b1_b2_sz_id(kOpMem, baseIndexType, flags, size, indexId);
    _mem.base = baseId;
    _mem.offsetLo32 = static_cast<uint32_t>(off);
  }
  explicit ASMJIT_INLINE Mem(const _NoInit&) noexcept : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Mem Internal]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t _unpackFromFlags(uint32_t shift, uint32_t bits) const noexcept {
    return (static_cast<uint32_t>(_mem.flags) >> shift) & bits;
  }

  ASMJIT_INLINE void _packToFlags(uint32_t value, uint32_t shift, uint32_t bits) noexcept {
    ASMJIT_ASSERT(value <= bits);
    _mem.flags = static_cast<uint8_t>(
      (static_cast<uint32_t>(_mem.flags) & ~(bits << shift)) | (value << shift));
  }

  // --------------------------------------------------------------------------
  // [Mem Specific]
  // --------------------------------------------------------------------------

  //! Clone `Mem` operand.
  ASMJIT_INLINE Mem clone() const noexcept { return Mem(*this); }

  //! Reset the memory operand - after reset the memory points to [0].
  ASMJIT_INLINE void reset() noexcept {
    _init_packed_op_b1_b2_sz_id(kOpMem, 0, 0, 0, kInvalidValue);
    _init_packed_d2_d3(0, 0);
  }

  //! Get if this `Mem` is a home of a variable or stack-allocated memory, used by \ref CodeCompiler.
  ASMJIT_INLINE bool isRegHome() const noexcept { return (_mem.flags & kFlagIsRegHome) != 0; }
  //! Clear the reg-home bit, called by the \ref CodeCompiler to finalize the operand.
  ASMJIT_INLINE void clearRegHome() noexcept { _mem.flags &= ~kFlagIsRegHome; }

  //! Get if the memory operand has a BASE register or label specified.
  ASMJIT_INLINE bool hasBase() const noexcept { return getBaseType() != 0; }
  //! Get if the memory operand has an INDEX register specified.
  ASMJIT_INLINE bool hasIndex() const noexcept { return getIndexType() != 0; }

  //! Get whether the memory operand has BASE and INDEX register.
  ASMJIT_INLINE bool hasBaseOrIndex() const noexcept { return _mem.baseIndexType != 0; }
  //! Get whether the memory operand has BASE and INDEX register.
  ASMJIT_INLINE bool hasBaseAndIndex() const noexcept { return (_mem.baseIndexType & 0xF0U) != 0 && (_mem.baseIndexType & 0x0FU) != 0; }

  //! Get if the BASE operand is a register.
  ASMJIT_INLINE bool hasBaseReg() const noexcept {
    // It's hacky, but if we know that 0 and 1 reg-type is never used by any
    // register we can assume that every reg-type will always have at least
    // one of three remaining bits (in a 4-bit nibble) set. This is a good
    // optimization as we don't have to unpack the BASE type from `baseIndexType`.
    return (_mem.baseIndexType & (0xE << kMemBaseTypeShift)) != 0;
  }
  //! Get if the BASE operand is a register.
  ASMJIT_INLINE bool hasIndexReg() const noexcept {
    return (_mem.baseIndexType & (0xE << kMemIndexTypeShift)) != 0;
  }

  //! Get if the BASE operand is a label.
  ASMJIT_INLINE bool hasBaseLabel() const noexcept { return getBaseType() == Label::kLabelTag; }

  //! Get type of a BASE register (0 if this memory operand doesn't use the BASE register).
  //!
  //! NOTE: If the returned type is one (a value never associated to a register
  //! type) the BASE is not register, but it's a label. One equals to `kLabelTag`.
  //! You should always check `hasBaseLabel()` before using `getBaseId()` result.
  ASMJIT_INLINE uint32_t getBaseType() const noexcept { return (_mem.baseIndexType >> kMemBaseTypeShift) & 0xF; }
  //! Get type of an INDEX register (0 if this memory operand doesn't use the INDEX register).
  ASMJIT_INLINE uint32_t getIndexType() const noexcept { return (_mem.baseIndexType >> kMemIndexTypeShift) & 0xF; }

  //! Get both BASE (3:0 bits) and INDEX (7:4 bits) types combined into a single integer.
  //!
  //! NOTE: This is the way these two are stored. You can use `encodeBaseIndex()`
  //! to encode any two types into the format and bit masking to extract the BASE
  //! and INDEX types from the packed integer.
  ASMJIT_INLINE uint32_t getBaseIndexType() const noexcept { return _mem.baseIndexType; }

  //! Get id of the BASE register or label (if the BASE was specified as label).
  ASMJIT_INLINE uint32_t getBaseId() const noexcept { return _mem.base; }

  //! Get id of the INDEX register.
  ASMJIT_INLINE uint32_t getIndexId() const noexcept { return _mem.index; }

  ASMJIT_INLINE void _setBase(uint32_t regType, uint32_t id) noexcept {
    _mem.baseIndexType = static_cast<uint8_t>(
      (_mem.baseIndexType & ~encodeBaseIndex(0xF, 0)) | encodeBaseIndex(regType, 0));
    _mem.base = id;
  }

  ASMJIT_INLINE void _setIndex(uint32_t regType, uint32_t id) noexcept {
    _mem.baseIndexType = static_cast<uint8_t>(
      (_mem.baseIndexType & ~encodeBaseIndex(0, 0xF)) | encodeBaseIndex(0, regType));
    _mem.index = id;
  }

  ASMJIT_INLINE void setBase(const Reg& base) noexcept { return _setBase(base.getRegType(), base.getId()); }
  ASMJIT_INLINE void setIndex(const Reg& index) noexcept { return _setIndex(index.getRegType(), index.getId()); }

  //! Reset the memory operand's BASE register / label.
  ASMJIT_INLINE void resetBase() noexcept { _setBase(0, 0); }
  //! Reset the memory operand's INDEX register.
  ASMJIT_INLINE void resetIndex() noexcept { _setIndex(0, kInvalidValue); }

  //! Set memory operand size.
  ASMJIT_INLINE void setSize(uint32_t size) noexcept { _mem.size = static_cast<uint8_t>(size); }

  //! Get if the memory operand has 64-bit offset or absolute address.
  //!
  //! If this is true then `hasBase()` must always report false.
  ASMJIT_INLINE bool has64BitOffset() const noexcept { return getBaseType() == 0; }

  //! Get a 64-bit offset or absolute address.
  ASMJIT_INLINE int64_t getOffset() const noexcept {
    return has64BitOffset()
      ? static_cast<int64_t>(_mem.offset64)
      : static_cast<int64_t>(static_cast<int32_t>(_mem.offsetLo32)); // Sign-Extend.
  }

  //! Get a lower part of a 64-bit offset or absolute address.
  ASMJIT_INLINE int32_t getOffsetLo32() const noexcept { return static_cast<int32_t>(_mem.offsetLo32); }
  //! Get a higher part of a 64-bit offset or absolute address.
  //!
  //! NOTE: This function is UNSAFE and returns garbage if `has64BitOffset()`
  //! returns false. Never use blindly without checking it.
  ASMJIT_INLINE int32_t getOffsetHi32() const noexcept { return static_cast<int32_t>(_mem.base); }

  //! Set a 64-bit offset or an absolute address to `offset`.
  //!
  //! NOTE: This functions attempts to set both high and low parts of a 64-bit
  //! offset, however, if the operand has a BASE register it will store only the
  //! low 32 bits of the offset / address as there is no way to store both BASE
  //! and 64-bit offset, and there is currently no architecture that has such
  //! capability targeted by AsmJit.
  ASMJIT_INLINE void setOffset(int64_t offset) noexcept {
    if (has64BitOffset())
      _mem.offset64 = static_cast<uint64_t>(offset);
    else
      _mem.offsetLo32 = static_cast<int32_t>(offset & 0xFFFFFFFF);
  }
  //! Adjust the offset by a 64-bit `off`.
  ASMJIT_INLINE void addOffset(int64_t off) noexcept {
    if (has64BitOffset())
      _mem.offset64 += static_cast<uint64_t>(off);
    else
      _mem.offsetLo32 += static_cast<uint32_t>(off & 0xFFFFFFFF);
  }
  //! Reset the memory offset to zero.
  ASMJIT_INLINE void resetOffset() noexcept { setOffset(0); }

  //! Set a low 32-bit offset to `off`.
  ASMJIT_INLINE void setOffsetLo32(int32_t off) noexcept {
    _mem.offsetLo32 = static_cast<uint32_t>(off);
  }
  //! Adjust the offset by `off`.
  //!
  //! NOTE: This is a fast function that doesn't use the HI 32-bits of a
  //! 64-bit offset. Use it only if you know that there is a BASE register
  //! and the offset is only 32 bits anyway.
  ASMJIT_INLINE void addOffsetLo32(int32_t off) noexcept {
    _mem.offsetLo32 += static_cast<uint32_t>(off);
  }
  //! Reset the memory offset to zero.
  ASMJIT_INLINE void resetOffsetLo32() noexcept { setOffsetLo32(0); }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Mem& operator=(const Mem& other) noexcept { copyFrom(other); return *this; }
  ASMJIT_INLINE bool operator==(const Mem& other) const noexcept { return  isEqual(other); }
  ASMJIT_INLINE bool operator!=(const Mem& other) const noexcept { return !isEqual(other); }
};

// ============================================================================
// [asmjit::Imm]
// ============================================================================

//! Immediate operand.
//!
//! Immediate operand is usually part of instruction itself. It's inlined after
//! or before the instruction opcode. Immediates can be only signed or unsigned
//! integers.
//!
//! To create immediate operand use `imm()` or `imm_u()` non-members or `Imm`
//! constructors.
class Imm : public Operand {
public:
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! Create a new immediate value (initial value is 0).
  Imm() noexcept : Operand(NoInit) {
    _init_packed_op_b1_b2_sz_id(kOpImm, 0, 0, 0, kInvalidValue);
    _imm.value._i64[0] = 0;
  }

  //! Create a new signed immediate value, assigning the value to `val`.
  explicit Imm(int64_t val) noexcept : Operand(NoInit) {
    _init_packed_op_b1_b2_sz_id(kOpImm, 0, 0, 0, kInvalidValue);
    _imm.value._i64[0] = val;
  }

  //! Create a new immediate value from `other`.
  ASMJIT_INLINE Imm(const Imm& other) noexcept : Operand(other) {}

  explicit ASMJIT_INLINE Imm(const _NoInit&) noexcept : Operand(NoInit) {}

  // --------------------------------------------------------------------------
  // [Immediate Specific]
  // --------------------------------------------------------------------------

  //! Clone `Imm` operand.
  ASMJIT_INLINE Imm clone() const noexcept { return Imm(*this); }

  //! Get whether the immediate can be casted to 8-bit signed integer.
  ASMJIT_INLINE bool isInt8() const noexcept { return Utils::isInt8(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 8-bit unsigned integer.
  ASMJIT_INLINE bool isUInt8() const noexcept { return Utils::isUInt8(_imm.value._i64[0]); }

  //! Get whether the immediate can be casted to 16-bit signed integer.
  ASMJIT_INLINE bool isInt16() const noexcept { return Utils::isInt16(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 16-bit unsigned integer.
  ASMJIT_INLINE bool isUInt16() const noexcept { return Utils::isUInt16(_imm.value._i64[0]); }

  //! Get whether the immediate can be casted to 32-bit signed integer.
  ASMJIT_INLINE bool isInt32() const noexcept { return Utils::isInt32(_imm.value._i64[0]); }
  //! Get whether the immediate can be casted to 32-bit unsigned integer.
  ASMJIT_INLINE bool isUInt32() const noexcept { return Utils::isUInt32(_imm.value._i64[0]); }

  //! Get immediate value as 8-bit signed integer.
  ASMJIT_INLINE int8_t getInt8() const noexcept { return _imm.value._i8[_ASMJIT_ARCH_INDEX(8, 0)]; }
  //! Get immediate value as 8-bit unsigned integer.
  ASMJIT_INLINE uint8_t getUInt8() const noexcept { return _imm.value._u8[_ASMJIT_ARCH_INDEX(8, 0)]; }
  //! Get immediate value as 16-bit signed integer.
  ASMJIT_INLINE int16_t getInt16() const noexcept { return _imm.value._i16[_ASMJIT_ARCH_INDEX(4, 0)]; }
  //! Get immediate value as 16-bit unsigned integer.
  ASMJIT_INLINE uint16_t getUInt16() const noexcept { return _imm.value._u16[_ASMJIT_ARCH_INDEX(4, 0)]; }
  //! Get immediate value as 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32() const noexcept { return _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 0)]; }
  //! Get immediate value as 32-bit unsigned integer.
  ASMJIT_INLINE uint32_t getUInt32() const noexcept { return _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)]; }
  //! Get immediate value as 64-bit signed integer.
  ASMJIT_INLINE int64_t getInt64() const noexcept { return _imm.value._i64[0]; }
  //! Get immediate value as 64-bit unsigned integer.
  ASMJIT_INLINE uint64_t getUInt64() const noexcept { return _imm.value._u64[0]; }

  //! Get immediate value as `intptr_t`.
  ASMJIT_INLINE intptr_t getIntPtr() const noexcept {
    if (sizeof(intptr_t) == sizeof(int64_t))
      return static_cast<intptr_t>(getInt64());
    else
      return static_cast<intptr_t>(getInt32());
  }

  //! Get immediate value as `uintptr_t`.
  ASMJIT_INLINE uintptr_t getUIntPtr() const noexcept {
    if (sizeof(uintptr_t) == sizeof(uint64_t))
      return static_cast<uintptr_t>(getUInt64());
    else
      return static_cast<uintptr_t>(getUInt32());
  }

  //! Get low 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32Lo() const noexcept { return _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 0)]; }
  //! Get low 32-bit signed integer.
  ASMJIT_INLINE uint32_t getUInt32Lo() const noexcept { return _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)]; }
  //! Get high 32-bit signed integer.
  ASMJIT_INLINE int32_t getInt32Hi() const noexcept { return _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 1)]; }
  //! Get high 32-bit signed integer.
  ASMJIT_INLINE uint32_t getUInt32Hi() const noexcept { return _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)]; }

  //! Set immediate value to 8-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt8(int8_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      int32_t val32 = static_cast<int32_t>(val);
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 0)] = val32;
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 1)] = val32 >> 31;
    }
    return *this;
  }

  //! Set immediate value to 8-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt8(uint8_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)] = static_cast<uint32_t>(val);
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 16-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt16(int16_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      int32_t val32 = static_cast<int32_t>(val);
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 0)] = val32;
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 1)] = val32 >> 31;
    }
    return *this;
  }

  //! Set immediate value to 16-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt16(uint16_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)] = static_cast<uint32_t>(val);
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 32-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt32(int32_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._i64[0] = static_cast<int64_t>(val);
    }
    else {
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 0)] = val;
      _imm.value._i32[_ASMJIT_ARCH_INDEX(2, 1)] = val >> 31;
    }
    return *this;
  }

  //! Set immediate value to 32-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt32(uint32_t val) noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._u64[0] = static_cast<uint64_t>(val);
    }
    else {
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)] = val;
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  //! Set immediate value to 64-bit signed integer `val`.
  ASMJIT_INLINE Imm& setInt64(int64_t val) noexcept {
    _imm.value._i64[0] = val;
    return *this;
  }

  //! Set immediate value to 64-bit unsigned integer `val`.
  ASMJIT_INLINE Imm& setUInt64(uint64_t val) noexcept {
    _imm.value._u64[0] = val;
    return *this;
  }

  //! Set immediate value to intptr_t `val`.
  ASMJIT_INLINE Imm& setIntPtr(intptr_t val) noexcept {
    _imm.value._i64[0] = static_cast<int64_t>(val);
    return *this;
  }

  //! Set immediate value to uintptr_t `val`.
  ASMJIT_INLINE Imm& setUIntPtr(uintptr_t val) noexcept {
    _imm.value._u64[0] = static_cast<uint64_t>(val);
    return *this;
  }

  //! Set immediate value as unsigned type to `val`.
  ASMJIT_INLINE Imm& setPtr(void* p) noexcept {
    return setIntPtr((intptr_t)p);
  }

  //! Set immediate value to `val`.
  template<typename T>
  ASMJIT_INLINE Imm& setValue(T val) noexcept {
    return setIntPtr((intptr_t)val);
  }

  // --------------------------------------------------------------------------
  // [Float]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Imm& setFloat(float f) noexcept {
    _imm.value._f32[_ASMJIT_ARCH_INDEX(2, 0)] = f;
    _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    return *this;
  }

  ASMJIT_INLINE Imm& setDouble(double d) noexcept {
    _imm.value._f64[0] = d;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Truncate]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE Imm& truncateTo8Bits() noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._u64[0] &= static_cast<uint64_t>(0x000000FFU);
    }
    else {
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)] &= 0x000000FFU;
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  ASMJIT_INLINE Imm& truncateTo16Bits() noexcept {
    if (ASMJIT_ARCH_64BIT) {
      _imm.value._u64[0] &= static_cast<uint64_t>(0x0000FFFFU);
    }
    else {
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 0)] &= 0x0000FFFFU;
      _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    }
    return *this;
  }

  ASMJIT_INLINE Imm& truncateTo32Bits() noexcept {
    _imm.value._u32[_ASMJIT_ARCH_INDEX(2, 1)] = 0;
    return *this;
  }

  // --------------------------------------------------------------------------
  // [Operator Overload]
  // --------------------------------------------------------------------------

  //! Assign `other` to the immediate operand.
  ASMJIT_INLINE Imm& operator=(const Imm& other) noexcept { copyFrom(other); return *this; }
  ASMJIT_INLINE bool operator==(const Imm& other) const noexcept { return  isEqual(other); }
  ASMJIT_INLINE bool operator!=(const Imm& other) const noexcept { return !isEqual(other); }
};

//! Create a signed immediate operand.
static ASMJIT_INLINE Imm imm(int64_t val) noexcept { return Imm(val); }
//! Create an unsigned immediate operand.
static ASMJIT_INLINE Imm imm_u(uint64_t val) noexcept { return Imm(static_cast<int64_t>(val)); }
//! Create an immediate operand from `p`.
template<typename T>
static ASMJIT_INLINE Imm imm_ptr(T p) noexcept { return Imm(static_cast<int64_t>((intptr_t)p)); }

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_OPERAND_H
