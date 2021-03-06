//===-- msan.h --------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of MemorySanitizer.
//
// Private MSan header.
//===----------------------------------------------------------------------===//

#ifndef MSAN_H
#define MSAN_H

#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_internal_defs.h"
#include "sanitizer_common/sanitizer_stacktrace.h"
#include "msan_interface_internal.h"
#include "msan_flags.h"
#include "ubsan/ubsan_platform.h"

#ifndef MSAN_REPLACE_OPERATORS_NEW_AND_DELETE
# define MSAN_REPLACE_OPERATORS_NEW_AND_DELETE 1
#endif

#ifndef MSAN_CONTAINS_UBSAN
# define MSAN_CONTAINS_UBSAN CAN_SANITIZE_UB
#endif

struct MappingDesc {
  uptr start;
  uptr end;
  enum Type {
    INVALID, APP, SHADOW, ORIGIN
  } type;
  const char *name;
};


#if SANITIZER_LINUX && defined(__mips64)

// Everything is above 0x00e000000000.
const MappingDesc kMemoryLayout[] = {
    {0x000000000000ULL, 0x00a000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x00a000000000ULL, 0x00c000000000ULL, MappingDesc::SHADOW, "shadow"},
    {0x00c000000000ULL, 0x00e000000000ULL, MappingDesc::ORIGIN, "origin"},
    {0x00e000000000ULL, 0x010000000000ULL, MappingDesc::APP, "app"}};

#define MEM_TO_SHADOW(mem) (((uptr)(mem)) & ~0x4000000000ULL)
#define SHADOW_TO_ORIGIN(shadow) (((uptr)(shadow)) + 0x002000000000)

#elif SANITIZER_LINUX && defined(__aarch64__)

// The mapping describes both 39-bits and 42-bits.  AArch64 maps:
// - 0x00000000000-0x00010000000: 39/42-bits program own segments
// - 0x05500000000-0x05600000000: 39-bits PIE program segments
// - 0x07f80000000-0x07fffffffff: 39-bits libraries segments
// - 0x2aa00000000-0x2ab00000000: 42-bits PIE program segments
// - 0x3ff00000000-0x3ffffffffff: 42-bits libraries segments
// It is fragmented in multiples segments to increase the memory available
// on 42-bits (12.21% of total VMA available for 42-bits and 13.28 for
// 39 bits).
const MappingDesc kMemoryLayout[] = {
    {0x00000000000ULL, 0x01000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x01000000000ULL, 0x02000000000ULL, MappingDesc::SHADOW, "shadow-2"},
    {0x02000000000ULL, 0x03000000000ULL, MappingDesc::ORIGIN, "origin-2"},
    {0x03000000000ULL, 0x04000000000ULL, MappingDesc::SHADOW, "shadow-1"},
    {0x04000000000ULL, 0x05000000000ULL, MappingDesc::ORIGIN, "origin-1"},
    {0x05000000000ULL, 0x06000000000ULL, MappingDesc::APP, "app-1"},
    {0x06000000000ULL, 0x07000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x07000000000ULL, 0x08000000000ULL, MappingDesc::APP, "app-2"},
    {0x08000000000ULL, 0x09000000000ULL, MappingDesc::INVALID, "invalid"},
    // The mappings below are used only for 42-bits VMA.
    {0x09000000000ULL, 0x0A000000000ULL, MappingDesc::SHADOW, "shadow-3"},
    {0x0A000000000ULL, 0x0B000000000ULL, MappingDesc::ORIGIN, "origin-3"},
    {0x0B000000000ULL, 0x0F000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x0F000000000ULL, 0x10000000000ULL, MappingDesc::APP, "app-3"},
    {0x10000000000ULL, 0x11000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x11000000000ULL, 0x12000000000ULL, MappingDesc::APP, "app-4"},
    {0x12000000000ULL, 0x17000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x17000000000ULL, 0x18000000000ULL, MappingDesc::SHADOW, "shadow-4"},
    {0x18000000000ULL, 0x19000000000ULL, MappingDesc::ORIGIN, "origin-4"},
    {0x19000000000ULL, 0x20000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x20000000000ULL, 0x21000000000ULL, MappingDesc::APP, "app-5"},
    {0x21000000000ULL, 0x26000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x26000000000ULL, 0x27000000000ULL, MappingDesc::SHADOW, "shadow-5"},
    {0x27000000000ULL, 0x28000000000ULL, MappingDesc::ORIGIN, "origin-5"},
    {0x28000000000ULL, 0x29000000000ULL, MappingDesc::SHADOW, "shadow-7"},
    {0x29000000000ULL, 0x2A000000000ULL, MappingDesc::ORIGIN, "origin-7"},
    {0x2A000000000ULL, 0x2B000000000ULL, MappingDesc::APP, "app-6"},
    {0x2B000000000ULL, 0x2C000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x2C000000000ULL, 0x2D000000000ULL, MappingDesc::SHADOW, "shadow-6"},
    {0x2D000000000ULL, 0x2E000000000ULL, MappingDesc::ORIGIN, "origin-6"},
    {0x2E000000000ULL, 0x2F000000000ULL, MappingDesc::APP, "app-7"},
    {0x2F000000000ULL, 0x39000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x39000000000ULL, 0x3A000000000ULL, MappingDesc::SHADOW, "shadow-9"},
    {0x3A000000000ULL, 0x3B000000000ULL, MappingDesc::ORIGIN, "origin-9"},
    {0x3B000000000ULL, 0x3C000000000ULL, MappingDesc::APP, "app-8"},
    {0x3C000000000ULL, 0x3D000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x3D000000000ULL, 0x3E000000000ULL, MappingDesc::SHADOW, "shadow-8"},
    {0x3E000000000ULL, 0x3F000000000ULL, MappingDesc::ORIGIN, "origin-8"},
    {0x3F000000000ULL, 0x40000000000ULL, MappingDesc::APP, "app-9"},
};
# define MEM_TO_SHADOW(mem) ((uptr)mem ^ 0x6000000000ULL)
# define SHADOW_TO_ORIGIN(shadow) (((uptr)(shadow)) + 0x1000000000ULL)

#elif SANITIZER_LINUX && SANITIZER_PPC64

const MappingDesc kMemoryLayout[] = {
    {0x000000000000ULL, 0x000100000000ULL, MappingDesc::APP, "low memory"},
    {0x000100000000ULL, 0x080000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x080000000000ULL, 0x180100000000ULL, MappingDesc::SHADOW, "shadow"},
    {0x180100000000ULL, 0x1C0000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x1C0000000000ULL, 0x2C0100000000ULL, MappingDesc::ORIGIN, "origin"},
    {0x2C0100000000ULL, 0x300000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x300000000000ULL, 0x400000000000ULL, MappingDesc::APP, "high memory"}};

// Maps low and high app ranges to contiguous space with zero base:
//   Low:  0000 0000 0000 - 0000 ffff ffff  ->  1000 0000 0000 - 1000 ffff ffff
//   High: 3000 0000 0000 - 3fff ffff ffff  ->  0000 0000 0000 - 0fff ffff ffff
#define LINEARIZE_MEM(mem) \
  (((uptr)(mem) & ~0x200000000000ULL) ^ 0x100000000000ULL)
#define MEM_TO_SHADOW(mem) (LINEARIZE_MEM((mem)) + 0x080000000000ULL)
#define SHADOW_TO_ORIGIN(shadow) (((uptr)(shadow)) + 0x140000000000ULL)

#elif SANITIZER_FREEBSD && SANITIZER_WORDSIZE == 64

// Low memory: main binary, MAP_32BIT mappings and modules
// High memory: heap, modules and main thread stack
const MappingDesc kMemoryLayout[] = {
    {0x000000000000ULL, 0x010000000000ULL, MappingDesc::APP, "low memory"},
    {0x010000000000ULL, 0x100000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x100000000000ULL, 0x310000000000ULL, MappingDesc::SHADOW, "shadow"},
    {0x310000000000ULL, 0x380000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x380000000000ULL, 0x590000000000ULL, MappingDesc::ORIGIN, "origin"},
    {0x590000000000ULL, 0x600000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x600000000000ULL, 0x800000000000ULL, MappingDesc::APP, "high memory"}};

// Maps low and high app ranges to contiguous space with zero base:
//   Low:  0000 0000 0000 - 00ff ffff ffff  ->  2000 0000 0000 - 20ff ffff ffff
//   High: 6000 0000 0000 - 7fff ffff ffff  ->  0000 0000 0000 - 1fff ffff ffff
#define LINEARIZE_MEM(mem) \
  (((uptr)(mem) & ~0xc00000000000ULL) ^ 0x200000000000ULL)
#define MEM_TO_SHADOW(mem) (LINEARIZE_MEM((mem)) + 0x100000000000ULL)
#define SHADOW_TO_ORIGIN(shadow) (((uptr)(shadow)) + 0x280000000000)

#elif SANITIZER_LINUX && SANITIZER_WORDSIZE == 64

#ifdef MSAN_LINUX_X86_64_OLD_MAPPING
// Requries PIE binary and ASLR enabled.
// Main thread stack and DSOs at 0x7f0000000000 (sometimes 0x7e0000000000).
// Heap at 0x600000000000.
const MappingDesc kMemoryLayout[] = {
    {0x000000000000ULL, 0x200000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x200000000000ULL, 0x400000000000ULL, MappingDesc::SHADOW, "shadow"},
    {0x400000000000ULL, 0x600000000000ULL, MappingDesc::ORIGIN, "origin"},
    {0x600000000000ULL, 0x800000000000ULL, MappingDesc::APP, "app"}};

#define MEM_TO_SHADOW(mem) (((uptr)(mem)) & ~0x400000000000ULL)
#define SHADOW_TO_ORIGIN(mem) (((uptr)(mem)) + 0x200000000000ULL)
#else  // MSAN_LINUX_X86_64_OLD_MAPPING
// All of the following configurations are supported.
// ASLR disabled: main executable and DSOs at 0x555550000000
// PIE and ASLR: main executable and DSOs at 0x7f0000000000
// non-PIE: main executable below 0x100000000, DSOs at 0x7f0000000000
// Heap at 0x700000000000.
const MappingDesc kMemoryLayout[] = {
    {0x000000000000ULL, 0x010000000000ULL, MappingDesc::APP, "app-1"},
    {0x010000000000ULL, 0x100000000000ULL, MappingDesc::SHADOW, "shadow-2"},
    {0x100000000000ULL, 0x110000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x110000000000ULL, 0x200000000000ULL, MappingDesc::ORIGIN, "origin-2"},
    {0x200000000000ULL, 0x300000000000ULL, MappingDesc::SHADOW, "shadow-3"},
    {0x300000000000ULL, 0x400000000000ULL, MappingDesc::ORIGIN, "origin-3"},
    {0x400000000000ULL, 0x500000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x500000000000ULL, 0x510000000000ULL, MappingDesc::SHADOW, "shadow-1"},
    {0x510000000000ULL, 0x600000000000ULL, MappingDesc::APP, "app-2"},
    {0x600000000000ULL, 0x610000000000ULL, MappingDesc::ORIGIN, "origin-1"},
    {0x610000000000ULL, 0x700000000000ULL, MappingDesc::INVALID, "invalid"},
    {0x700000000000ULL, 0x800000000000ULL, MappingDesc::APP, "app-3"}};
#define MEM_TO_SHADOW(mem) (((uptr)(mem)) ^ 0x500000000000ULL)
#define SHADOW_TO_ORIGIN(mem) (((uptr)(mem)) + 0x100000000000ULL)
#endif  // MSAN_LINUX_X86_64_OLD_MAPPING

#else
#error "Unsupported platform"
#endif

const uptr kMemoryLayoutSize = sizeof(kMemoryLayout) / sizeof(kMemoryLayout[0]);

#define MEM_TO_ORIGIN(mem) (SHADOW_TO_ORIGIN(MEM_TO_SHADOW((mem))))

#ifndef __clang__
__attribute__((optimize("unroll-loops")))
#endif
inline bool addr_is_type(uptr addr, MappingDesc::Type mapping_type) {
// It is critical for performance that this loop is unrolled (because then it is
// simplified into just a few constant comparisons).
#ifdef __clang__
#pragma unroll
#endif
  for (unsigned i = 0; i < kMemoryLayoutSize; ++i)
    if (kMemoryLayout[i].type == mapping_type &&
        addr >= kMemoryLayout[i].start && addr < kMemoryLayout[i].end)
      return true;
  return false;
}

#define MEM_IS_APP(mem) addr_is_type((uptr)(mem), MappingDesc::APP)
#define MEM_IS_SHADOW(mem) addr_is_type((uptr)(mem), MappingDesc::SHADOW)
#define MEM_IS_ORIGIN(mem) addr_is_type((uptr)(mem), MappingDesc::ORIGIN)

// These constants must be kept in sync with the ones in MemorySanitizer.cc.
const int kMsanParamTlsSize = 800;
const int kMsanRetvalTlsSize = 800;

namespace __msan {
extern int msan_inited;
extern bool msan_init_is_running;
extern int msan_report_count;

bool ProtectRange(uptr beg, uptr end);
bool InitShadow(bool init_origins);
char *GetProcSelfMaps();
void InitializeInterceptors();

void MsanAllocatorInit();
void MsanAllocatorThreadFinish();
void *MsanCalloc(StackTrace *stack, uptr nmemb, uptr size);
void *MsanReallocate(StackTrace *stack, void *oldp, uptr size,
                     uptr alignment, bool zeroise);
void MsanDeallocate(StackTrace *stack, void *ptr);
void InstallTrapHandler();
void InstallAtExitHandler();

const char *GetStackOriginDescr(u32 id, uptr *pc);

void EnterSymbolizer();
void ExitSymbolizer();
bool IsInSymbolizer();

struct SymbolizerScope {
  SymbolizerScope() { EnterSymbolizer(); }
  ~SymbolizerScope() { ExitSymbolizer(); }
};

void PrintWarning(uptr pc, uptr bp);
void PrintWarningWithOrigin(uptr pc, uptr bp, u32 origin);

void GetStackTrace(BufferedStackTrace *stack, uptr max_s, uptr pc, uptr bp,
                   bool request_fast_unwind);

void ReportUMR(StackTrace *stack, u32 origin);
void ReportExpectedUMRNotFound(StackTrace *stack);
void ReportStats();
void ReportAtExitStatistics();
void DescribeMemoryRange(const void *x, uptr size);
void ReportUMRInsideAddressRange(const char *what, const void *start, uptr size,
                                 uptr offset);

// Unpoison first n function arguments.
void UnpoisonParam(uptr n);
void UnpoisonThreadLocalState();

// Returns a "chained" origin id, pointing to the given stack trace followed by
// the previous origin id.
u32 ChainOrigin(u32 id, StackTrace *stack);

const int STACK_TRACE_TAG_POISON = StackTrace::TAG_CUSTOM + 1;

#define GET_MALLOC_STACK_TRACE                                                 \
  BufferedStackTrace stack;                                                    \
  if (__msan_get_track_origins() && msan_inited)                               \
  GetStackTrace(&stack, common_flags()->malloc_context_size,                   \
                StackTrace::GetCurrentPc(), GET_CURRENT_FRAME(),               \
                common_flags()->fast_unwind_on_malloc)

#define GET_STORE_STACK_TRACE_PC_BP(pc, bp)                                    \
  BufferedStackTrace stack;                                                    \
  if (__msan_get_track_origins() > 1 && msan_inited)                           \
  GetStackTrace(&stack, flags()->store_context_size, pc, bp,                   \
                common_flags()->fast_unwind_on_malloc)

#define GET_FATAL_STACK_TRACE_PC_BP(pc, bp)                                    \
  BufferedStackTrace stack;                                                    \
  if (msan_inited)                                                             \
  GetStackTrace(&stack, kStackTraceMax, pc, bp,                                \
                common_flags()->fast_unwind_on_fatal)

#define GET_STORE_STACK_TRACE \
  GET_STORE_STACK_TRACE_PC_BP(StackTrace::GetCurrentPc(), GET_CURRENT_FRAME())

class ScopedThreadLocalStateBackup {
 public:
  ScopedThreadLocalStateBackup() { Backup(); }
  ~ScopedThreadLocalStateBackup() { Restore(); }
  void Backup();
  void Restore();
 private:
  u64 va_arg_overflow_size_tls;
};

void MsanTSDInit(void (*destructor)(void *tsd));
void *MsanTSDGet();
void MsanTSDSet(void *tsd);
void MsanTSDDtor(void *tsd);

}  // namespace __msan

#define MSAN_MALLOC_HOOK(ptr, size)     \
  if (&__sanitizer_malloc_hook) {       \
    UnpoisonParam(2);                   \
    __sanitizer_malloc_hook(ptr, size); \
  }
#define MSAN_FREE_HOOK(ptr)     \
  if (&__sanitizer_free_hook) { \
    UnpoisonParam(1);           \
    __sanitizer_free_hook(ptr); \
  }

#endif  // MSAN_H
