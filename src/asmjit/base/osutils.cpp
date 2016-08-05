// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Export]
#define ASMJIT_EXPORTS

// [Dependencies]
#include "../base/osutils.h"
#include "../base/utils.h"

#if ASMJIT_OS_POSIX
# include <sys/types.h>
# include <sys/mman.h>
# include <time.h>
# include <unistd.h>
#endif // ASMJIT_OS_POSIX

#if ASMJIT_OS_MAC
# include <mach/mach_time.h>
#endif // ASMJIT_OS_MAC

#if ASMJIT_OS_WINDOWS
# if defined(_MSC_VER) && _MSC_VER >= 1400
#  include <intrin.h>
# else
#  define _InterlockedCompareExchange InterlockedCompareExchange
# endif // _MSC_VER
#endif // ASMJIT_OS_WINDOWS

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [asmjit::OSUtils - Virtual Memory - Windows]
// ============================================================================

// Windows specific implementation using `VirtualAllocEx` and `VirtualFree`.
#if ASMJIT_OS_WINDOWS
static ASMJIT_NOINLINE const VMemInfo& OSUtils_GetVMemInfo() noexcept {
  static VMemInfo vmi;

  if (ASMJIT_UNLIKELY(!vmi.hCurrentProcess)) {
    SYSTEM_INFO info;
    ::GetSystemInfo(&info);

    vmi.pageSize = Utils::alignToPowerOf2<uint32_t>(info.dwPageSize);
    vmi.pageGranularity = info.dwAllocationGranularity;
    vmi.hCurrentProcess = ::GetCurrentProcess();
  }

  return vmi;
};

VMemInfo OSUtils::getVirtualMemoryInfo() noexcept { return OSUtils_GetVMemInfo(); }

void* OSUtils::allocVirtualMemory(size_t size, size_t* allocated, uint32_t flags) noexcept {
  return allocProcessMemory(static_cast<HANDLE>(0), size, allocated, flags);
}

Error OSUtils::releaseVirtualMemory(void* p, size_t size) noexcept {
  return releaseProcessMemory(static_cast<HANDLE>(0), p, size);
}

void* OSUtils::allocProcessMemory(HANDLE hProcess, size_t size, size_t* allocated, uint32_t flags) noexcept {
  if (size == 0)
    return nullptr;

  const VMemInfo& vmi = OSUtils_GetVMemInfo();
  if (!hProcess) hProcess = vmi.hCurrentProcess;

  // VirtualAllocEx rounds the allocated size to a page size automatically,
  // but we need the `alignedSize` so we can store the real allocated size
  // into `allocated` output.
  size_t alignedSize = Utils::alignTo(size, vmi.pageSize);

  // Windows XP SP2 / Vista+ allow data-execution-prevention (DEP).
  DWORD protectFlags = 0;

  if (flags & kVMExecutable)
    protectFlags |= (flags & kVMWritable) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
  else
    protectFlags |= (flags & kVMWritable) ? PAGE_READWRITE : PAGE_READONLY;

  LPVOID mBase = ::VirtualAllocEx(hProcess, nullptr, alignedSize, MEM_COMMIT | MEM_RESERVE, protectFlags);
  if (ASMJIT_UNLIKELY(!mBase)) return nullptr;

  ASMJIT_ASSERT(Utils::isAligned<size_t>(reinterpret_cast<size_t>(mBase), vmi.pageSize));
  if (allocated) *allocated = alignedSize;
  return mBase;
}

Error OSUtils::releaseProcessMemory(HANDLE hProcess, void* p, size_t size) noexcept {
  const VMemInfo& vmi = OSUtils_GetVMemInfo();
  if (!hProcess) hProcess = vmi.hCurrentProcess;

  if (ASMJIT_UNLIKELY(!::VirtualFreeEx(hProcess, p, 0, MEM_RELEASE)))
    return DebugUtils::errored(kErrorInvalidState);

  return kErrorOk;
}
#endif // ASMJIT_OS_WINDOWS

// ============================================================================
// [asmjit::OSUtils - Virtual Memory - Posix]
// ============================================================================

// Posix specific implementation using `mmap` and `munmap`.
#if ASMJIT_OS_POSIX

// Mac uses MAP_ANON instead of MAP_ANONYMOUS.
#if !defined(MAP_ANONYMOUS)
# define MAP_ANONYMOUS MAP_ANON
#endif // MAP_ANONYMOUS

static const VMemInfo& OSUtils_GetVMemInfo() noexcept {
  static VMemInfo vmi;

  if (ASMJIT_UNLIKELY(!vmi.pageSize)) {
    size_t pageSize = ::getpagesize();
    vmi.pageSize = pageSize;
    vmi.pageGranularity = Utils::iMax<size_t>(pageSize, 65536);
  }

  return vmi;
};

VMemInfo OSUtils::getVirtualMemoryInfo() noexcept { return OSUtils_GetVMemInfo(); }

void* OSUtils::allocVirtualMemory(size_t size, size_t* allocated, uint32_t flags) noexcept {
  const VMemInfo& vmi = OSUtils_GetVMemInfo();

  size_t alignedSize = Utils::alignTo<size_t>(size, vmi.pageSize);
  int protection = PROT_READ;

  if (flags & kVMWritable  ) protection |= PROT_WRITE;
  if (flags & kVMExecutable) protection |= PROT_EXEC;

  void* mbase = ::mmap(nullptr, alignedSize, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ASMJIT_UNLIKELY(mbase == MAP_FAILED)) return nullptr;

  if (allocated) *allocated = alignedSize;
  return mbase;
}

Error OSUtils::releaseVirtualMemory(void* p, size_t size) noexcept {
  if (ASMJIT_UNLIKELY(::munmap(p, size) != 0))
    return DebugUtils::errored(kErrorInvalidState);

  return kErrorOk;
}
#endif // ASMJIT_OS_POSIX

// ============================================================================
// [asmjit::OSUtils - GetTickCount - Windows]
// ============================================================================

#if ASMJIT_OS_WINDOWS
static volatile uint32_t OSUtils_hiResTicks;
static volatile double OSUtils_hiResFreq;

uint32_t OSUtils::getTickCount() noexcept {
  do {
    uint32_t hiResOk = OSUtils_hiResTicks;

    if (hiResOk == 1) {
      LARGE_INTEGER now;
      if (!::QueryPerformanceCounter(&now))
        break;
      return (int64_t)(double(now.QuadPart) / OSUtils_hiResFreq);
    }

    if (hiResOk == 0) {
      LARGE_INTEGER qpf;
      if (!::QueryPerformanceFrequency(&qpf)) {
        _InterlockedCompareExchange((LONG*)&OSUtils_hiResTicks, 0xFFFFFFFF, 0);
        break;
      }

      LARGE_INTEGER now;
      if (!::QueryPerformanceCounter(&now)) {
        _InterlockedCompareExchange((LONG*)&OSUtils_hiResTicks, 0xFFFFFFFF, 0);
        break;
      }

      double freqDouble = double(qpf.QuadPart) / 1000.0;
      OSUtils_hiResFreq = freqDouble;
      _InterlockedCompareExchange((LONG*)&OSUtils_hiResTicks, 1, 0);

      return static_cast<uint32_t>(
        static_cast<int64_t>(double(now.QuadPart) / freqDouble) & 0xFFFFFFFF);
    }
  } while (0);

  // Bail to a less precise GetTickCount().
  return ::GetTickCount();
}

// ============================================================================
// [asmjit::OSUtils - GetTickCount - Mac]
// ============================================================================

#elif ASMJIT_OS_MAC
static mach_timebase_info_data_t OSUtils_machTime;

uint32_t OSUtils::getTickCount() noexcept {
  // Initialize the first time CpuTicks::now() is called (See Apple's QA1398).
  if (ASMJIT_UNLIKELY(OSUtils_machTime.denom == 0)) {
    if (mach_timebase_info(&OSUtils_machTime) != KERN_SUCCESS)
      return 0;
  }

  // mach_absolute_time() returns nanoseconds, we need just milliseconds.
  uint64_t t = mach_absolute_time() / 1000000;

  t = t * OSUtils_machTime.numer / OSUtils_machTime.denom;
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
}

// ============================================================================
// [asmjit::OSUtils - GetTickCount - Posix]
// ============================================================================

#else
uint32_t OSUtils::getTickCount() noexcept {
#if defined(_POSIX_MONOTONIC_CLOCK) && _POSIX_MONOTONIC_CLOCK >= 0
  struct timespec ts;

  if (ASMJIT_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
    return 0;

  uint64_t t = (uint64_t(ts.tv_sec ) * 1000) + (uint64_t(ts.tv_nsec) / 1000000);
  return static_cast<uint32_t>(t & 0xFFFFFFFFU);
#else  // _POSIX_MONOTONIC_CLOCK
#error "[asmjit] OSUtils::getTickCount() is not implemented for your target OS."
  return 0;
#endif  // _POSIX_MONOTONIC_CLOCK
}
#endif // ASMJIT_OS

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
