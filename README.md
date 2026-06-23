# sov-math — Sovereign Law-Computation Math Library

**GF(3) pure integer domain, C++23, zero floating-point.** A constitutional mathematical framework implementing 8 layers from binary hardware through holographic observation.

```
  L8 全息观测 ───  f=432×2^b×3^a  log10标度
  L7 陈数守卫 ───  C=±2 拓扑不变死锁
  L6 仲吕倍频 ───  ×8 频率级联
  L5 纳音孤子 ───  C3周期=1500 ρ=0.38居里相变
  L4 T⁶环面  ───  144×46=6624 损益链
  L3 手性离合 ───  Z[ω] ω=e^{2πi/3} 五行振幅
  桥 LCM     ───  (acc×3¹¹)>>16 唯一合法通道
  L2 Z/3¹¹Z ───  3≠0 逢三进一 位权3^k
  L1 GF(3)  ───  3≡0 每trit独立 {0,1,2}
  L0 模2硬件 ───  x86-64 ADC uint384
```

## Constitutional Invariants

| Constant | Value | Meaning |
|----------|-------|---------|
| HUANGZHONG | 177147 | 3¹¹, LCM bridge multiplier |
| ZHONGLV_BOUNDARY | 65536 | 2¹⁶, binary truncation width |
| LCM_TOTAL | 11609505792 | 3¹¹ × 2¹⁶ |
| POLAR_WINDING | 144 | I_h (120) + chiral tetra (24) |
| TOROIDAL_WINDING | 46 | Holographic period |
| GRAND_PUMP | 6624 | 144 × 46, full sovereign breath |
| CHERN_TARGET | ±2 | Chern number topological invariant |
| DELTA | √3 | Phase transition energy gap |

## Architecture

```
include/          — 33 header files (C++23, headers-only + optional .so)
  ├── lcm_constants.h          L0-L8 constitutional constants
  ├── gf3_types.h              Core types: Trit, TryteValue, SovBlock128
  ├── gf3_field.h              L1 GF(3) field — C3 rotation, mul, norm
  ├── gf3_layer1.h             L1 packing/unpacking, vector operations
  ├── gf3_operators.h          Operator hierarchy (AdcAdder→Layer1Ops→LcmBridge→Z3ROps)
  ├── adc_limb.h               L0 uint384 ADC carry chain
  ├── adc_carry_chain.h        L0-L8 error analysis template
  ├── z3r_ring.h               L2 Z/3¹¹Z RingElement (11-digit base-3)
  ├── z3r_layer2.h             L2 RingElement vector/matrix ops
  ├── gf3_layer2.h             L2 carry propagation, matmul, RMSNorm
  ├── chiral_geometry.h        L3 chiral conjugacy, coupling states
  ├── fixed_complex.h          L3 Q16.16 fixed-point, ω=e^{2πi/3}
  ├── lcm_bridge.h             LCM bridge state machine
  ├── loss_gain.h              L4 twelve-tone temperament chain
  ├── nayin_soliton_l5.h       L5 C3 soliton dynamics
  ├── zhonglv_multiplier_l6.h  L6 frequency cascade
  ├── chern_guard_l7.h         L7 Chern guard, topology protection
  ├── holographic_limit_l8.h   L8 holographic state computation
  ├── digital_root.h           Cross-layer digital root mathematics
  ├── sovereign_assert.h       Compile-time constitutional validators
  ├── sov_format.h             SOV v2.6 file format
  ├── sov_io.h                 File I/O engine
  └── ...                      (advanced: solver, tetration, time crystal)

vavx3/            — Optional AVX2 C kernels (shared library)
tests/            — 🔴🟡🟢 traffic-light test suite (9 tests)
```

## Building

### Requirements
- **Compiler:** GCC ≥ 14 (or Clang ≥ 18)
- **CMake:** ≥ 3.25
- **CPU:** x86-64 with AVX2
- **Optional:** OpenMP (for VAVX3 kernels), GMP (for golden verification)

### Build
```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
ctest
```

### Individual Test Compilation
```bash
# Constitutional-level
g++ -std=c++23 -mavx2 -O3 -I./include -o build/test_red_constitution tests/test_red_constitution.cpp

# Engineering-level
g++ -std=c++23 -mavx2 -O3 -I./include -o build/test_amber_ring tests/test_amber_ring.cpp

# Informational-level
g++ -std=c++23 -mavx2 -O3 -I./include -o build/test_green_api tests/test_green_api.cpp
```

### Usage
```cpp
#include "gf3_types.h"
#include "z3r_ring.h"

using namespace sov::math;

// GF(3) arithmetic
uint8_t a = 1, b = 2;
uint8_t sum = gf3::c3_cw(a);        // C3 rotation
uint8_t prod = TRIT_MUL_LUT[a][b];  // GF(3) multiply: 1×2=2

// LCM bridge
LcmBridge bridge;
bridge.forward_bridge();             // Layer1 → Layer2

// Z/3¹¹Z ring
z3r::RingElement x = z3r::RingElement::T1();
z3r::RingElement y = z3r::RingElement::T2();
z3r::RingElement z = x * y;         // T3 (positional carry propagation)
```

## Test Suite: 🔴🟡🟢 Traffic Lights

| Level | Tests | Meaning |
|-------|-------|---------|
| 🔴 **Red** | 3 tests | Constitutional invariants — 违宪即阻断 |
| 🟡 **Amber** | 3 tests | Engineering precision/stability/boundaries |
| 🟢 **Green** | 1 test | API consistency, format compliance |
| ⬜ **Base** | 2 tests | Bridge error analysis, L5-L8 verification |

```
$ ctest
100% tests passed, 0 tests failed out of 9
```

## Anti-Optimization Guards

The library includes three levels of GCC optimization defense:

| Macro | Purpose |
|-------|---------|
| `SOV_COMPILER_BARRIER()` | Full memory fence — blocks instruction reordering |
| `SOV_KEEP(x)` | Read-only anchor — prevents dead code elimination |
| `SOV_ANCHOR(x)` | Read-write anchor — prevents constant folding of invariants |

These exist because **GCC -O3 will aggressively constant-fold, reorder, and eliminate
sovereign operations** — treating them as pure functions when they carry observable
side effects in the LCM accumulator and Chern guard.

## Permanently Abolished

| Practice | Reason |
|----------|--------|
| AdamW optimizer | Cannot produce trit=2 discrete jumps |
| Q·K^T float softmax | Electrical civilization illegal projection |
| `-ffast-math` | Introduces floating-point approximation |
| `{-1,0,1}` encoding | Only {0,1,2} is legal |
| Direct cross-layer operations | Must pass through LCM bridge |

## License

MIT License — see [LICENSE](LICENSE)
