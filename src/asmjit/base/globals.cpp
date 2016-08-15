// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::Arch]
// ============================================================================

static const uint32_t archSignatureTable[] = {
  //               +-----------------+-------------------+-------+
  //               |Type             | SubType           | GPInfo|
  //               +-----------------+-------------------+-------+
  ASMJIT_PACK32_4x8(Arch::kTypeNone  , Arch::kSubTypeNone, 0,  0),
  ASMJIT_PACK32_4x8(Arch::kTypeX86   , Arch::kSubTypeNone, 4,  8),
  ASMJIT_PACK32_4x8(Arch::kTypeX64   , Arch::kSubTypeNone, 8, 16),
  ASMJIT_PACK32_4x8(Arch::kTypeX32   , Arch::kSubTypeNone, 8, 16),
  ASMJIT_PACK32_4x8(Arch::kTypeArm32 , Arch::kSubTypeNone, 4, 16),
  ASMJIT_PACK32_4x8(Arch::kTypeArm64 , Arch::kSubTypeNone, 8, 32)
};

void Arch::init(uint32_t type, uint32_t subType) noexcept {
  uint32_t index = type < ASMJIT_ARRAY_SIZE(archSignatureTable) ? type : uint32_t(0);

  // Make sure the `archSignatureTable` array is correctly indexed.
  _signature = archSignatureTable[index];
  ASMJIT_ASSERT(_type == index);

  // Even if the architecture is not knows we setup its type and mode, however,
  // the information `Arch` has would be basically useless.
  _type = type;
  _subType = subType;
}

// ============================================================================
// [asmjit::DebugUtils]
// ============================================================================

#if !defined(ASMJIT_DISABLE_TEXT)
static const char errorMessages[] =
  "Ok\0"
  "No heap memory\0"
  "No virtual memory\0"
  "Invalid argument\0"
  "Invalid state\0"
  "Invalid architecture\0"
  "Not initialized\0"
  "Already initialized\0"
  "Slot occupied\0"
  "No code generated\0"
  "Code too large\0"
  "Invalid label\0"
  "Label index overflow\0"
  "Label already bound\0"
  "Label already defined\0"
  "Label name too long\0"
  "Invalid label name\0"
  "Invalid parent label\0"
  "Non-local label can't have parent\0"
  "Invalid instruction\0"
  "Invalid register type\0"
  "Invalid register's physical id\0"
  "Invalid register's virtual id\0"
  "Invalid broadcast\0"
  "Invalid {sae} or {rc} option\0"
  "Invalid address\0"
  "Invalid displacement\0"
  "Invalid type-info\0"
  "Invalid use of a low 8-bit GPB register\0"
  "Invalid use of a 64-bit GPQ register in 32-bit mode\0"
  "Invalid use of an 80-bit float\0"
  "Overlapped arguments\0"
  "Unknown error\0";

static const char* findPackedString(const char* p, uint32_t id, uint32_t maxId) noexcept {
  uint32_t i = 0;

  if (id > maxId)
    id = maxId;

  while (i < id) {
    while (p[0])
      p++;

    p++;
    i++;
  }

  return p;
}
#endif // ASMJIT_DISABLE_TEXT

const char* DebugUtils::errorAsString(Error err) noexcept {
#if !defined(ASMJIT_DISABLE_TEXT)
  return findPackedString(errorMessages, err, kErrorCount);
#else
  static const char noMessage[] = "";
  return noMessage;
#endif
}

void DebugUtils::debugOutput(const char* str) noexcept {
#if ASMJIT_OS_WINDOWS
  ::OutputDebugStringA(str);
#else
  ::fputs(str, stderr);
#endif
}

void DebugUtils::assertionFailed(const char* file, int line, const char* msg) noexcept {
  char str[1024];

  snprintf(str, 1024,
    "[asmjit] Assertion failed at %s (line %d):\n"
    "[asmjit] %s\n", file, line, msg);

  // Support buggy `snprintf` implementations.
  str[1023] = '\0';

  debugOutput(str);
  ::abort();
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
