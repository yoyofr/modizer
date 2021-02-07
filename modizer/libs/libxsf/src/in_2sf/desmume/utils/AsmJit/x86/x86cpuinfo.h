// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

#pragma once

// [Dependencies - AsmJit]
#include "../base/cpuinfo.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

// ============================================================================
// [Forward Declarations]
// ============================================================================

struct X86CpuInfo;

//! \addtogroup asmjit_x86_general
//! \{

// ============================================================================
// [asmjit::kX86CpuFeature]
// ============================================================================

//! X86 CPU features.
ASMJIT_ENUM(kX86CpuFeature) {
  //! Cpu has multithreading.
  kX86CpuFeatureMultithreading = 1,
  //! Cpu has execute disable bit.
  kX86CpuFeatureExecuteDisableBit,
  //! Cpu has RDTSC.
  kX86CpuFeatureRdtsc,
  //! Cpu has RDTSCP.
  kX86CpuFeatureRdtscp,
  //! Cpu has CMOV.
  kX86CpuFeatureCmov,
  //! Cpu has CMPXCHG8B.
  kX86CpuFeatureCmpXchg8B,
  //! Cpu has CMPXCHG16B (x64).
  kX86CpuFeatureCmpXchg16B,
  //! Cpu has CLFUSH.
  kX86CpuFeatureClflush,
  //! Cpu has PREFETCH.
  kX86CpuFeaturePrefetch,
  //! Cpu has LAHF/SAHF.
  kX86CpuFeatureLahfSahf,
  //! Cpu has FXSAVE/FXRSTOR.
  kX86CpuFeatureFxsr,
  //! Cpu has FXSAVE/FXRSTOR optimizations.
  kX86CpuFeatureFfxsr,
  //! Cpu has MMX.
  kX86CpuFeatureMmx,
  //! Cpu has extended MMX.
  kX86CpuFeatureMmxExt,
  //! Cpu has 3dNow!
  kX86CpuFeature3dNow,
  //! Cpu has enchanced 3dNow!
  kX86CpuFeature3dNowExt,
  //! Cpu has SSE.
  kX86CpuFeatureSse,
  //! Cpu has SSE2.
  kX86CpuFeatureSse2,
  //! Cpu has SSE3.
  kX86CpuFeatureSse3,
  //! Cpu has Supplemental SSE3 (SSSE3).
  kX86CpuFeatureSsse3,
  //! Cpu has SSE4.A.
  kX86CpuFeatureSse4A,
  //! Cpu has SSE4.1.
  kX86CpuFeatureSse41,
  //! Cpu has SSE4.2.
  kX86CpuFeatureSse42,
  //! Cpu has Misaligned SSE (MSSE).
  kX86CpuFeatureMsse,
  //! Cpu has MONITOR and MWAIT.
  kX86CpuFeatureMonitorMWait,
  //! Cpu has MOVBE.
  kX86CpuFeatureMovbe,
  //! Cpu has POPCNT.
  kX86CpuFeaturePopcnt,
  //! Cpu has LZCNT.
  kX86CpuFeatureLzcnt,
  //! Cpu has AESNI.
  kX86CpuFeatureAesni,
  //! Cpu has PCLMULQDQ.
  kX86CpuFeaturePclmulqdq,
  //! Cpu has RDRAND.
  kX86CpuFeatureRdrand,
  //! Cpu has AVX.
  kX86CpuFeatureAvx,
  //! Cpu has AVX2.
  kX86CpuFeatureAvx2,
  //! Cpu has F16C.
  kX86CpuFeatureF16C,
  //! Cpu has FMA3.
  kX86CpuFeatureFma3,
  //! Cpu has FMA4.
  kX86CpuFeatureFma4,
  //! Cpu has XOP.
  kX86CpuFeatureXop,
  //! Cpu has BMI.
  kX86CpuFeatureBmi,
  //! Cpu has BMI2.
  kX86CpuFeatureBmi2,
  //! Cpu has HLE.
  kX86CpuFeatureHle,
  //! Cpu has RTM.
  kX86CpuFeatureRtm,
  //! Cpu has FSGSBASE.
  kX86CpuFeatureFsGsBase,
  //! Cpu has enhanced REP MOVSB/STOSB.
  kX86CpuFeatureRepMovsbStosbExt,

  //! Count of X86/X64 Cpu features.
  kX86CpuFeatureCount
};

// ============================================================================
// [asmjit::X86CpuId]
// ============================================================================

//! X86/X64 CPUID output.
union X86CpuId {
  //! EAX/EBX/ECX/EDX output.
  uint32_t i[4];

  struct {
    //! EAX output.
    uint32_t eax;
    //! EBX output.
    uint32_t ebx;
    //! ECX output.
    uint32_t ecx;
    //! EDX output.
    uint32_t edx;
  };
};

// ============================================================================
// [asmjit::X86CpuUtil]
// ============================================================================

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
//! CPU utilities available only if the host processor is X86/X64.
struct X86CpuUtil {
  //! Get the result of calling CPUID instruction to `out`.
  ASMJIT_API static void callCpuId(uint32_t inEax, uint32_t inEcx, X86CpuId* out);

  //! Detect the Host CPU.
  ASMJIT_API static void detect(X86CpuInfo* cpuInfo);
};
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

// ============================================================================
// [asmjit::X86CpuInfo]
// ============================================================================

struct X86CpuInfo : public CpuInfo {
  ASMJIT_NO_COPY(X86CpuInfo)

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE X86CpuInfo(uint32_t size = sizeof(X86CpuInfo)) :
    CpuInfo(size) {}

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get processor type.
  ASMJIT_INLINE uint32_t getProcessorType() const {
    return _processorType;
  }

  //! Get brand index.
  ASMJIT_INLINE uint32_t getBrandIndex() const {
    return _brandIndex;
  }

  //! Get flush cache line size.
  ASMJIT_INLINE uint32_t getFlushCacheLineSize() const {
    return _flushCacheLineSize;
  }

  //! Get maximum logical processors count.
  ASMJIT_INLINE uint32_t getMaxLogicalProcessors() const {
    return _maxLogicalProcessors;
  }

  // --------------------------------------------------------------------------
  // [Statics]
  // --------------------------------------------------------------------------

#if defined(ASMJIT_HOST_X86) || defined(ASMJIT_HOST_X64)
  //! Get global instance of `X86CpuInfo`.
  static ASMJIT_INLINE const X86CpuInfo* getHost() {
    return static_cast<const X86CpuInfo*>(CpuInfo::getHost());
  }
#endif // ASMJIT_HOST_X86 || ASMJIT_HOST_X64

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! Processor type.
  uint32_t _processorType;
  //! Brand index.
  uint32_t _brandIndex;
  //! Flush cache line size in bytes.
  uint32_t _flushCacheLineSize;
  //! Maximum number of addressable IDs for logical processors.
  uint32_t _maxLogicalProcessors;
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"
