// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_X86_X86MISC_H
#define _ASMJIT_X86_X86MISC_H

// [Dependencies]
#include "../x86/x86operand.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_x86
//! \{

// ============================================================================
// [asmjit::X86RegCount]
// ============================================================================

//! \internal
//!
//! X86/X64 registers count.
//!
//! Since the number of registers changed across CPU generations `X86RegCount`
//! class is used by `X86Assembler` and `X86Compiler` to provide a way to get
//! number of available registers dynamically. 32-bit mode offers always only
//! 8 registers of all classes, however, 64-bit mode offers 16 GP registers and
//! 16 XMM/YMM/ZMM registers. AVX512 instruction set doubles the number of SIMD
//! registers (XMM/YMM/ZMM) to 32, this mode has to be explicitly enabled to
//! take effect as it changes some assumptions.
//!
//! `X86RegCount` is also used extensively by X86Compiler's register allocator
//! and data structures. FP registers were omitted as they are never mapped to
//! variables, thus, not needed to be managed.
//!
//! NOTE: At the moment `X86RegCount` can fit into 32-bits, having 8-bits for
//! each register class except FP. This can change in the future after a new
//! instruction set, which adds more registers, is introduced.
struct X86RegCount {
  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  //! Reset all counters to zero.
  ASMJIT_INLINE void reset() noexcept { _packed = 0; }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  //! Get register count by a register class `rc`.
  ASMJIT_INLINE uint32_t get(uint32_t rc) const noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    uint32_t shift = Utils::byteShiftOfDWordStruct(rc);
    return (_packed >> shift) & static_cast<uint32_t>(0xFF);
  }

  //! Get Gp count.
  ASMJIT_INLINE uint32_t getGp() const noexcept { return get(X86Reg::kClassGp); }
  //! Get Mm count.
  ASMJIT_INLINE uint32_t getMm() const noexcept { return get(X86Reg::kClassMm); }
  //! Get K count.
  ASMJIT_INLINE uint32_t getK() const noexcept { return get(X86Reg::kClassK); }
  //! Get XMM/YMM/ZMM count.
  ASMJIT_INLINE uint32_t getXyz() const noexcept { return get(X86Reg::kClassXyz); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  //! Set register count by a register class `rc`.
  ASMJIT_INLINE void set(uint32_t rc, uint32_t n) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);
    ASMJIT_ASSERT(n <= 0xFF);

    uint32_t shift = Utils::byteShiftOfDWordStruct(rc);
    _packed = (_packed & ~static_cast<uint32_t>(0xFF << shift)) + (n << shift);
  }

  //! Set Gp count.
  ASMJIT_INLINE void setGp(uint32_t n) noexcept { set(X86Reg::kClassGp, n); }
  //! Set Mm count.
  ASMJIT_INLINE void setMm(uint32_t n) noexcept { set(X86Reg::kClassMm, n); }
  //! Set K count.
  ASMJIT_INLINE void setK(uint32_t n) noexcept { set(X86Reg::kClassK, n); }
  //! Set XMM/YMM/ZMM count.
  ASMJIT_INLINE void setXyz(uint32_t n) noexcept { set(X86Reg::kClassXyz, n); }

  // --------------------------------------------------------------------------
  // [Add]
  // --------------------------------------------------------------------------

  //! Add register count by a register class `rc`.
  ASMJIT_INLINE void add(uint32_t rc, uint32_t n = 1) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);
    ASMJIT_ASSERT(0xFF - static_cast<uint32_t>(_regs[rc]) >= n);

    uint32_t shift = Utils::byteShiftOfDWordStruct(rc);
    _packed += n << shift;
  }

  //! Add GP count.
  ASMJIT_INLINE void addGp(uint32_t n) noexcept { add(X86Reg::kClassGp, n); }
  //! Add MMX count.
  ASMJIT_INLINE void addMm(uint32_t n) noexcept { add(X86Reg::kClassMm, n); }
  //! Add K count.
  ASMJIT_INLINE void addK(uint32_t n) noexcept { add(X86Reg::kClassK, n); }
  //! Add XMM/YMM/ZMM count.
  ASMJIT_INLINE void addXyz(uint32_t n) noexcept { add(X86Reg::kClassXyz, n); }

  // --------------------------------------------------------------------------
  // [Misc]
  // --------------------------------------------------------------------------

  //! Build register indexes based on the given `count` of registers.
  ASMJIT_INLINE void indexFromRegCount(const X86RegCount& count) noexcept {
    uint32_t x = static_cast<uint32_t>(count._regs[0]);
    uint32_t y = static_cast<uint32_t>(count._regs[1]) + x;
    uint32_t z = static_cast<uint32_t>(count._regs[2]) + y;

    ASMJIT_ASSERT(y <= 0xFF);
    ASMJIT_ASSERT(z <= 0xFF);
    _packed = Utils::pack32_4x8(0, x, y, z);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! Count of GP registers.
      uint8_t _gp;
      //! Count of MMX registers.
      uint8_t _mm;
      //! Count of K registers.
      uint8_t _k;
      //! Count of XMM|YMM|ZMM registers.
      uint8_t _xyz;
    };

    uint8_t _regs[4];
    uint32_t _packed;
  };
};

// ============================================================================
// [asmjit::X86RegMask]
// ============================================================================

//! \internal
//!
//! X86/X64 registers mask.
struct X86RegMask {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  //! Reset all register masks to zero.
  ASMJIT_INLINE void reset() noexcept {
    _packed.reset();
  }

  // --------------------------------------------------------------------------
  // [IsEmpty / Has]
  // --------------------------------------------------------------------------

  //! Get whether all register masks are zero (empty).
  ASMJIT_INLINE bool isEmpty() const noexcept {
    return _packed.isZero();
  }

  ASMJIT_INLINE bool has(uint32_t rc, uint32_t mask = 0xFFFFFFFFU) const noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : return (static_cast<uint32_t>(_gp ) & mask) != 0;
      case X86Reg::kClassMm : return (static_cast<uint32_t>(_mm ) & mask) != 0;
      case X86Reg::kClassK  : return (static_cast<uint32_t>(_k  ) & mask) != 0;
      case X86Reg::kClassXyz: return (static_cast<uint32_t>(_xyz) & mask) != 0;
    }

    return false;
  }

  ASMJIT_INLINE bool hasGp(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE bool hasMm(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE bool hasK(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kClassK, mask); }
  ASMJIT_INLINE bool hasXyz(uint32_t mask = 0xFFFFFFFFU) const noexcept { return has(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [Get]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uint32_t get(uint32_t rc) const noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : return _gp;
      case X86Reg::kClassMm : return _mm;
      case X86Reg::kClassK  : return _k;
      case X86Reg::kClassXyz: return _xyz;
    }

    return 0;
  }

  ASMJIT_INLINE uint32_t getGp() const noexcept { return get(X86Reg::kClassGp); }
  ASMJIT_INLINE uint32_t getMm() const noexcept { return get(X86Reg::kClassMm); }
  ASMJIT_INLINE uint32_t getK() const noexcept { return get(X86Reg::kClassK); }
  ASMJIT_INLINE uint32_t getXyz() const noexcept { return get(X86Reg::kClassXyz); }

  // --------------------------------------------------------------------------
  // [Zero]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void zero(uint32_t rc) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : _gp  = 0; break;
      case X86Reg::kClassMm : _mm  = 0; break;
      case X86Reg::kClassK  : _k   = 0; break;
      case X86Reg::kClassXyz: _xyz = 0; break;
    }
  }

  ASMJIT_INLINE void zeroGp() noexcept { zero(X86Reg::kClassGp); }
  ASMJIT_INLINE void zeroMm() noexcept { zero(X86Reg::kClassMm); }
  ASMJIT_INLINE void zeroK() noexcept { zero(X86Reg::kClassK); }
  ASMJIT_INLINE void zeroXyz() noexcept { zero(X86Reg::kClassXyz); }

  // --------------------------------------------------------------------------
  // [Set]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void set(const X86RegMask& other) noexcept {
    _packed = other._packed;
  }

  ASMJIT_INLINE void set(uint32_t rc, uint32_t mask) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : _gp  = static_cast<uint16_t>(mask); break;
      case X86Reg::kClassMm : _mm  = static_cast<uint8_t >(mask); break;
      case X86Reg::kClassK  : _k   = static_cast<uint8_t >(mask); break;
      case X86Reg::kClassXyz: _xyz = static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void setGp(uint32_t mask) noexcept { return set(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE void setMm(uint32_t mask) noexcept { return set(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE void setK(uint32_t mask) noexcept { return set(X86Reg::kClassK, mask); }
  ASMJIT_INLINE void setXyz(uint32_t mask) noexcept { return set(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [And]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void and_(const X86RegMask& other) noexcept {
    _packed.and_(other._packed);
  }

  ASMJIT_INLINE void and_(uint32_t rc, uint32_t mask) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : _gp  &= static_cast<uint16_t>(mask); break;
      case X86Reg::kClassMm : _mm  &= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassK  : _k   &= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassXyz: _xyz &= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void andGp(uint32_t mask) noexcept { and_(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE void andMm(uint32_t mask) noexcept { and_(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE void andK(uint32_t mask) noexcept { and_(X86Reg::kClassK, mask); }
  ASMJIT_INLINE void andXyz(uint32_t mask) noexcept { and_(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [AndNot]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void andNot(const X86RegMask& other) noexcept {
    _packed.andNot(other._packed);
  }

  ASMJIT_INLINE void andNot(uint32_t rc, uint32_t mask) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : _gp  &= ~static_cast<uint16_t>(mask); break;
      case X86Reg::kClassMm : _mm  &= ~static_cast<uint8_t >(mask); break;
      case X86Reg::kClassK  : _k   &= ~static_cast<uint8_t >(mask); break;
      case X86Reg::kClassXyz: _xyz &= ~static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void andNotGp(uint32_t mask) noexcept { andNot(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE void andNotMm(uint32_t mask) noexcept { andNot(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE void andNotK(uint32_t mask) noexcept { andNot(X86Reg::kClassK, mask); }
  ASMJIT_INLINE void andNotXyz(uint32_t mask) noexcept { andNot(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [Or]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void or_(const X86RegMask& other) noexcept {
    _packed.or_(other._packed);
  }

  ASMJIT_INLINE void or_(uint32_t rc, uint32_t mask) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);
    switch (rc) {
      case X86Reg::kClassGp : _gp  |= static_cast<uint16_t>(mask); break;
      case X86Reg::kClassMm : _mm  |= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassK  : _k   |= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassXyz: _xyz |= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void orGp(uint32_t mask) noexcept { return or_(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE void orMm(uint32_t mask) noexcept { return or_(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE void orK(uint32_t mask) noexcept { return or_(X86Reg::kClassK, mask); }
  ASMJIT_INLINE void orXyz(uint32_t mask) noexcept { return or_(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [Xor]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void xor_(const X86RegMask& other) noexcept {
    _packed.xor_(other._packed);
  }

  ASMJIT_INLINE void xor_(uint32_t rc, uint32_t mask) noexcept {
    ASMJIT_ASSERT(rc < X86Reg::_kClassManagedCount);

    switch (rc) {
      case X86Reg::kClassGp : _gp  ^= static_cast<uint16_t>(mask); break;
      case X86Reg::kClassMm : _mm  ^= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassK  : _k   ^= static_cast<uint8_t >(mask); break;
      case X86Reg::kClassXyz: _xyz ^= static_cast<uint32_t>(mask); break;
    }
  }

  ASMJIT_INLINE void xorGp(uint32_t mask) noexcept { xor_(X86Reg::kClassGp, mask); }
  ASMJIT_INLINE void xorMm(uint32_t mask) noexcept { xor_(X86Reg::kClassMm, mask); }
  ASMJIT_INLINE void xorK(uint32_t mask) noexcept { xor_(X86Reg::kClassK, mask); }
  ASMJIT_INLINE void xorXyz(uint32_t mask) noexcept { xor_(X86Reg::kClassXyz, mask); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  union {
    struct {
      //! GP registers mask (16 bits).
      uint16_t _gp;
      //! MMX registers mask (8 bits).
      uint8_t _mm;
      //! K registers mask (8 bits).
      uint8_t _k;
      //! XMM|YMM|ZMM registers mask (32 bits).
      uint32_t _xyz;
    };

    //! Packed masks.
    UInt64 _packed;
  };
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_X86_X86MISC_H
