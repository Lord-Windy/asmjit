// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/assembler.h"
#include "../base/cpuinfo.h"
#include "../base/runtime.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

static ASMJIT_INLINE void hostFlushInstructionCache(void* p, size_t size) noexcept {
  // Only useful on non-x86 architectures.
#if !ASMJIT_ARCH_X86 && !ASMJIT_ARCH_X64
# if ASMJIT_OS_WINDOWS
  // Windows has a built-in support in kernel32.dll.
  ::FlushInstructionCache(_memMgr.getProcessHandle(), p, size);
# endif // ASMJIT_OS_WINDOWS
#else
  ASMJIT_UNUSED(p);
  ASMJIT_UNUSED(size);
#endif // !ASMJIT_ARCH_X86 && !ASMJIT_ARCH_X64
}

// ============================================================================
// [asmjit::Runtime - Construction / Destruction]
// ============================================================================

Runtime::Runtime() noexcept
  : _archInfo(),
    _runtimeType(kRuntimeNone),
    _allocType(kVMemAllocFreeable),
    _cdeclConv(kCallConvNone),
    _stdCallConv(kCallConvNone) {}
Runtime::~Runtime() noexcept {}

// ============================================================================
// [asmjit::HostRuntime - Construction / Destruction]
// ============================================================================

HostRuntime::HostRuntime() noexcept {
  _runtimeType = kRuntimeJit;

  _archInfo = CpuInfo::getHost().getArchInfo();
  _cdeclConv = kCallConvHostCDecl;
  _stdCallConv = kCallConvHostStdCall;
}
HostRuntime::~HostRuntime() noexcept {}

// ============================================================================
// [asmjit::HostRuntime - Interface]
// ============================================================================

void HostRuntime::flush(void* p, size_t size) noexcept {
  hostFlushInstructionCache(p, size);
}

// ============================================================================
// [asmjit::JitRuntime - Construction / Destruction]
// ============================================================================

JitRuntime::JitRuntime() noexcept {}
JitRuntime::~JitRuntime() noexcept {}

// ============================================================================
// [asmjit::JitRuntime - Interface]
// ============================================================================

Error JitRuntime::add(void** dst, CodeHolder* holder) noexcept {
  size_t codeSize = holder->getCodeSize();
  if (ASMJIT_UNLIKELY(codeSize == 0)) {
    *dst = nullptr;
    return DebugUtils::errored(kErrorNoCodeGenerated);
  }

  void* p = _memMgr.alloc(codeSize, getAllocType());
  if (ASMJIT_UNLIKELY(!p)) {
    *dst = nullptr;
    return DebugUtils::errored(kErrorNoVirtualMemory);
  }

  // Relocate the code and release the unused memory back to `VMemMgr`.
  size_t relocSize = holder->relocate(p);
  if (ASMJIT_UNLIKELY(relocSize == 0)) {
    *dst = nullptr;
    _memMgr.release(p);
    return DebugUtils::errored(kErrorInvalidState);
  }

  if (relocSize < codeSize)
    _memMgr.shrink(p, relocSize);

  flush(p, relocSize);
  *dst = p;

  return kErrorOk;
}

Error JitRuntime::release(void* p) noexcept {
  return _memMgr.release(p);
}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
