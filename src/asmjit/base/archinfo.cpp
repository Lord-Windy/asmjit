// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/archinfo.h"
#include "../base/globals.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

static const ArchInfo archInfoData[] = {
//+-------------------+--+---+---+---+----------+
//| Architecture      |GP|NUM|USE|S.A| Reserved |
//+-------------------+--+---+---+---+----------+
  { ArchInfo::kIdNone , 0, 0 , 0 , 0 , 0, 0, 0 },
  { ArchInfo::kIdX86  , 4, 8 , 7 , 4 , 0, 0, 0 }, // 8  regs, 7 usable (except ESP).
  { ArchInfo::kIdX64  , 8, 16, 15, 16, 0, 0, 0 }, // 16 regs, 15 usable (except RSP).
  { ArchInfo::kIdX32  , 8, 16, 15, 16, 0, 0, 0 }, // 16 regs, 15 usable (except RSP).
  { ArchInfo::kIdArm32, 4, 16, 14, 8 , 0, 0, 0 }, // 16 regs, 14 usable (except R13{SP} and R15{PC}).
  { ArchInfo::kIdArm64, 8, 32, 31, 16, 0, 0, 0 }  // 32 regs, 31 usable (except R31{RIP|ZERO}).
};

void ArchInfo::setup(uint32_t archId) noexcept {
  uint32_t index = archId < ASMJIT_ARRAY_SIZE(archInfoData) ? archId : uint32_t(0);
  ::memcpy(this, archInfoData + index, sizeof(ArchInfo));

  // Make sure the `archInfoData` is correctly indexed.
  ASMJIT_ASSERT(_archId == index);

  // If the architecture is unknown we still set it to `archId`, but
  // there will be no information about it (defaults to ArchInfo::kIdNone).
  _archId = archId;
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
