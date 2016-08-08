// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/utils.h"
#include "../base/zone.h"
#include <stdarg.h>

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! Zero size block used by `Zone` that doesn't have any memory allocated.
static const Zone::Block Zone_zeroBlock = { nullptr, nullptr, 0, { 0 } };

static ASMJIT_INLINE uint32_t Zone_getAlignmentOffsetFromAlignment(uint32_t x) noexcept {
  switch (x) {
    case 0 : return 0;
    case 1 : return 1;
    case 2 : return 2;
    case 4 : return 3;
    case 8 : return 4;
    case 16: return 5;
    case 32: return 6;
    case 64: return 7;
    default: return 0;
  }
}

// ============================================================================
// [asmjit::Zone - Construction / Destruction]
// ============================================================================

Zone::Zone(uint32_t blockSize, uint32_t blockAlignment) noexcept
  : _ptr(nullptr),
    _end(nullptr),
    _block(const_cast<Zone::Block*>(&Zone_zeroBlock)),
    _blockSize(blockSize),
    _blockAlignmentShift(Zone_getAlignmentOffsetFromAlignment(blockAlignment)) {}

Zone::~Zone() noexcept {
  reset(true);
}

// ============================================================================
// [asmjit::Zone - Reset]
// ============================================================================

void Zone::reset(bool releaseMemory) noexcept {
  Block* cur = _block;

  // Can't be altered.
  if (cur == &Zone_zeroBlock)
    return;

  if (releaseMemory) {
    // Since cur can be in the middle of the double-linked list, we have to
    // traverse to both directions `prev` and `next` separately.
    Block* next = cur->next;
    do {
      Block* prev = cur->prev;
      ASMJIT_FREE(cur);
      cur = prev;
    } while (cur);

    cur = next;
    while (cur) {
      next = cur->next;
      ASMJIT_FREE(cur);
      cur = next;
    }

    _ptr = nullptr;
    _end = nullptr;
    _block = const_cast<Zone::Block*>(&Zone_zeroBlock);
  }
  else {
    while (cur->prev)
      cur = cur->prev;

    _ptr = cur->data;
    _end = _ptr + cur->size;
    _block = cur;
  }
}

// ============================================================================
// [asmjit::Zone - Alloc]
// ============================================================================

void* Zone::_alloc(size_t size) noexcept {
  Block* curBlock = _block;
  uint8_t* p;

  size_t blockSize = Utils::iMax<size_t>(_blockSize, size);
  size_t blockAlignment = getBlockAlignment();

  // The `_alloc()` method can only be called if there is not enough space
  // in the current block, see `alloc()` implementation for more details.
  ASMJIT_ASSERT(curBlock == &Zone_zeroBlock || getRemainingSize() < size);

  // If the `Zone` has been cleared the current block doesn't have to be the
  // last one. Check if there is a block that can be used instead of allocating
  // a new one. If there is a `next` block it's completely unused, we don't have
  // to check for remaining bytes.
  Block* next = curBlock->next;
  if (next && next->size >= size) {
    p = Utils::alignTo(next->data, blockAlignment);

    _block = next;
    _ptr = p + size;
    _end = next->data + next->size;

    return static_cast<void*>(p);
  }

  // Prevent arithmetic overflow.
  if (blockSize > (~static_cast<size_t>(0) - sizeof(Block) - blockAlignment))
    return nullptr;

  Block* newBlock = static_cast<Block*>(ASMJIT_ALLOC(sizeof(Block) - sizeof(void*) + blockSize + blockAlignment));
  if (ASMJIT_UNLIKELY(!newBlock)) return nullptr;

  // Align the pointer to `blockAlignment` and adjust the size of this block
  // accordingly. It's the same as using `blockAlignment - Utils::alignDiff()`,
  // just written differently.
  p = Utils::alignTo(newBlock->data, blockAlignment);
  newBlock->size = blockSize + blockAlignment - (size_t)(p - newBlock->data);

  newBlock->prev = nullptr;
  newBlock->next = nullptr;

  if (curBlock != &Zone_zeroBlock) {
    newBlock->prev = curBlock;
    curBlock->next = newBlock;

    // Does only happen if there is a next block, but the requested memory
    // can't fit into it. In this case a new buffer is allocated and inserted
    // between the current block and the next one.
    if (next) {
      newBlock->next = next;
      next->prev = newBlock;
    }
  }

  _block = newBlock;
  _ptr = p + size;
  _end = newBlock->data + blockSize + blockAlignment;

  return static_cast<void*>(p);
}

void* Zone::allocZeroed(size_t size) noexcept {
  void* p = alloc(size);
  if (ASMJIT_LIKELY(p))
    ::memset(p, 0, size);
  return p;
}

void* Zone::dup(const void* data, size_t size) noexcept {
  if (!data || size == 0) return nullptr;

  void* m = alloc(size);
  if (ASMJIT_UNLIKELY(!m)) return nullptr;

  ::memcpy(m, data, size);
  return m;
}

char* Zone::sdup(const char* str) noexcept {
  size_t len;
  if (!str || (len = ::strlen(str)) == 0) return nullptr;

  // Include null terminator and limit the string length.
  if (++len > 256) len = 256;

  char* m = static_cast<char*>(alloc(len));
  if (ASMJIT_UNLIKELY(!m)) return nullptr;

  ::memcpy(m, str, len);
  m[len - 1] = '\0';
  return m;
}

char* Zone::sformat(const char* fmt, ...) noexcept {
  if (ASMJIT_UNLIKELY(!fmt)) return nullptr;

  char buf[512];
  size_t len;

  va_list ap;
  va_start(ap, fmt);

  len = vsnprintf(buf, ASMJIT_ARRAY_SIZE(buf) - 1, fmt, ap);
  buf[len++] = 0;

  va_end(ap);
  return static_cast<char*>(dup(buf, len));
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
