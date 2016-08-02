// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_BITUTILS_H
#define _ASMJIT_BASE_BITUTILS_H

// [Dependencies]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::BitArray]
// ============================================================================

//! Fixed size bit-array.
//!
//! Used by variable liveness analysis.
struct BitArray {
  // --------------------------------------------------------------------------
  // [Enums]
  // --------------------------------------------------------------------------

  enum {
    kEntitySize = static_cast<int>(sizeof(uintptr_t)),
    kEntityBits = kEntitySize * 8
  };

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE uintptr_t getBit(uint32_t index) const noexcept {
    return (data[index / kEntityBits] >> (index % kEntityBits)) & 1;
  }

  ASMJIT_INLINE void setBit(uint32_t index) noexcept {
    data[index / kEntityBits] |= static_cast<uintptr_t>(1) << (index % kEntityBits);
  }

  ASMJIT_INLINE void delBit(uint32_t index) noexcept {
    data[index / kEntityBits] &= ~(static_cast<uintptr_t>(1) << (index % kEntityBits));
  }

  // --------------------------------------------------------------------------
  // [Interface]
  // --------------------------------------------------------------------------

  //! Copy bits from `s0`, returns `true` if at least one bit is set in `s0`.
  ASMJIT_INLINE bool copyBits(const BitArray* s0, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool addBits(const BitArray* s0, uint32_t len) noexcept {
    return addBits(this, s0, len);
  }

  ASMJIT_INLINE bool addBits(const BitArray* s0, const BitArray* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] | s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool andBits(const BitArray* s1, uint32_t len) noexcept {
    return andBits(this, s1, len);
  }

  ASMJIT_INLINE bool andBits(const BitArray* s0, const BitArray* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool delBits(const BitArray* s1, uint32_t len) noexcept {
    return delBits(this, s1, len);
  }

  ASMJIT_INLINE bool delBits(const BitArray* s0, const BitArray* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t t = s0->data[i] & ~s1->data[i];
      data[i] = t;
      r |= t;
    }
    return r != 0;
  }

  ASMJIT_INLINE bool _addBitsDelSource(BitArray* s1, uint32_t len) noexcept {
    return _addBitsDelSource(this, s1, len);
  }

  ASMJIT_INLINE bool _addBitsDelSource(const BitArray* s0, BitArray* s1, uint32_t len) noexcept {
    uintptr_t r = 0;
    for (uint32_t i = 0; i < len; i++) {
      uintptr_t a = s0->data[i];
      uintptr_t b = s1->data[i];

      this->data[i] = a | b;
      b &= ~a;

      s1->data[i] = b;
      r |= b;
    }
    return r != 0;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uintptr_t data[1];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_BITUTILS_H
