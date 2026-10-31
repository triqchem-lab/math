#ifndef GEOMETRIC_DYNAMICS_VORTEX_H
#define GEOMETRIC_DYNAMICS_VORTEX_H

#include <complex>
#include <math.h>
#include <numbers>   // std::numbers::pi_v<float>（严格标准下不提供 POSIX 的 M_PI）
#include "quantum_vortex_torus.h"

#define TETRAHEDRON_STABILITY 0.707106f 
#define HARMONIC_RESONANCE 36           
#define QUOTIENT_SPACE_DIM 3            

static inline float compute_geometric_stability(float triangle_flow) {
    float conjugate_spiral = sinf(triangle_flow) * TETRAHEDRON_STABILITY;
    return tanhf(conjugate_spiral);
}

static inline float get_torus_standing_wave(uint64_t addr, float spin_phase) {
    float normalized_addr = (float)(addr % HARMONIC_RESONANCE) / HARMONIC_RESONANCE;
    float k = 2.0f * std::numbers::pi_v<float> * QUOTIENT_SPACE_DIM;
    return sinf(k * normalized_addr) * cosf(spin_phase);
}

#endif
