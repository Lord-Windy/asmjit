// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/containers.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::PodVectorBase - NullData]
// ============================================================================

const PodVectorBase::Data PodVectorBase::_nullData = { 0, 0 };

static ASMJIT_INLINE bool isDataStatic(PodVectorBase* self, PodVectorBase::Data* d) noexcept {
  return (void*)(self + 1) == (void*)d;
}

// ============================================================================
// [asmjit::PodVectorBase - Reset]
// ============================================================================

//! Clear vector data and free internal buffer.
void PodVectorBase::reset(bool releaseMemory) noexcept {
  Data* d = _d;
  if (d == &_nullData)
    return;

  if (releaseMemory && !isDataStatic(this, d)) {
    ASMJIT_FREE(d);
    _d = const_cast<Data*>(&_nullData);
    return;
  }

  d->length = 0;
}

// ============================================================================
// [asmjit::PodVectorBase - Helpers]
// ============================================================================

Error PodVectorBase::_grow(size_t n, size_t sizeOfT) noexcept {
  Data* d = _d;

  size_t threshold = kMemAllocGrowMax / sizeOfT;
  size_t capacity = d->capacity;
  size_t after = d->length;

  if (IntTraits<size_t>::maxValue() - n < after)
    return DebugUtils::errored(kErrorNoHeapMemory);

  after += n;

  if (capacity >= after)
    return kErrorOk;

  // PodVector is used as an array to hold short-lived data structures used
  // during code generation. The purpose of this aggressive growing strategy
  // is to minimize memory reallocations.
  if (capacity < 32)
    capacity = 32;
  else if (capacity < 128)
    capacity = 128;
  else if (capacity < 512)
    capacity = 512;

  while (capacity < after) {
    if (capacity < threshold)
      capacity *= 2;
    else
      capacity += threshold;
  }

  return _reserve(capacity, sizeOfT);
}

Error PodVectorBase::_reserve(size_t n, size_t sizeOfT) noexcept {
  Data* d = _d;

  if (d->capacity >= n)
    return kErrorOk;

  size_t nBytes = sizeof(Data) + n * sizeOfT;
  if (nBytes < n) return DebugUtils::errored(kErrorNoHeapMemory);

  if (d == &_nullData) {
    d = static_cast<Data*>(ASMJIT_ALLOC(nBytes));
    if (!d) return DebugUtils::errored(kErrorNoHeapMemory);
    d->length = 0;
  }
  else {
    if (isDataStatic(this, d)) {
      Data* oldD = d;

      d = static_cast<Data*>(ASMJIT_ALLOC(nBytes));
      if (!d) return DebugUtils::errored(kErrorNoHeapMemory);

      size_t len = oldD->length;
      d->length = len;
      ::memcpy(d->getData(), oldD->getData(), len * sizeOfT);
    }
    else {
      d = static_cast<Data*>(ASMJIT_REALLOC(d, nBytes));
      if (!d) return DebugUtils::errored(kErrorNoHeapMemory);
    }
  }

  d->capacity = n;
  _d = d;

  return kErrorOk;
}

Error PodVectorBase::_resize(size_t n, size_t sizeOfT) noexcept {
  ASMJIT_PROPAGATE(_reserve(n, sizeOfT));
  size_t len = _d->length;

  if (n > len) ::memset(static_cast<uint8_t*>(_d->getData()) + len * sizeOfT, 0, (n - len) * sizeOfT);
  if (n != len) _d->length = n;

  return kErrorOk;
}

// ============================================================================
// [asmjit::PodHashBase - Utilities]
// ============================================================================

static uint32_t PodHashGetClosestPrime(uint32_t x) noexcept {
  static const uint32_t primeTable[] = {
    53, 193, 389, 769, 1543, 3079, 6151, 12289, 24593
  };

  size_t i = 0;
  uint32_t p;

  do {
    if ((p = primeTable[i]) > x)
      break;
  } while (++i < ASMJIT_ARRAY_SIZE(primeTable));

  return p;
}

// ============================================================================
// [asmjit::PodHashBase - Reset]
// ============================================================================

void PodHashBase::reset(bool releaseMemory) noexcept {
  _size = 0;

  if (releaseMemory) {
    if (_data != _embedded)
      ASMJIT_FREE(_data);

    _bucketsCount = 1;
    _bucketsGrow = 1;
    _data = _embedded;
    _embedded[0] = nullptr;
  }
  else {
    ::memset(_data, 0, _bucketsCount * sizeof(void*));
  }
}

// ============================================================================
// [asmjit::PodHashBase - Rehash]
// ============================================================================

void PodHashBase::_rehash(uint32_t newCount) noexcept {
  PodHashNode** oldData = _data;
  PodHashNode** newData = reinterpret_cast<PodHashNode**>(
    ASMJIT_ALLOC(static_cast<size_t>(newCount) * sizeof(PodHashNode*)));

  // We can still store nodes into the table, but it will degrade.
  if (ASMJIT_UNLIKELY(newData == nullptr))
    return;

  uint32_t i;
  uint32_t oldCount = _bucketsCount;

  for (i = 0; i < oldCount; i++) {
    PodHashNode* node = oldData[i];
    while (node) {
      PodHashNode* next = node->_hashNext;
      uint32_t hMod = node->_hVal % newCount;

      node->_hashNext = newData[hMod];
      newData[hMod] = node;

      node = next;
    }
  }

  // 90% is the maximum occupancy, can't overflow since the maximum capacity
  // is limited to the last prime number stored in the prime table.
  _bucketsCount = newCount;
  _bucketsGrow = newCount * 9 / 10;

  _data = newData;
  if (oldData != _embedded) ASMJIT_FREE(oldData);
}

// ============================================================================
// [asmjit::PodHashBase - Ops]
// ============================================================================

PodHashNode* PodHashBase::_put(PodHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;
  PodHashNode* next = _data[hMod];

  node->_hashNext = next;
  _data[hMod] = node;

  if (++_size >= _bucketsGrow && next) {
    uint32_t newCapacity = PodHashGetClosestPrime(_bucketsCount);
    if (newCapacity != _bucketsCount)
      _rehash(newCapacity);
  }

  return node;
}

PodHashNode* PodHashBase::_del(PodHashNode* node) noexcept {
  uint32_t hMod = node->_hVal % _bucketsCount;

  PodHashNode** pPrev = &_data[hMod];
  PodHashNode* p = *pPrev;

  while (p) {
    if (p == node) {
      *pPrev = p->_hashNext;
      return node;
    }

    pPrev = &p->_hashNext;
    p = *pPrev;
  }

  return nullptr;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
