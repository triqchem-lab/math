// vavx3_s2_kernels.cpp — VAVX3 SIMD内核 (C++23, AVX2, OpenMP)
//
// 宪法声明 — 每函数标注层级+范畴+模:
//   [层0] [模2]    x86-64二进制硬件, CPU指令 (AVX2, ADC, OpenMP)
//   [层1] [GF(3)]  有限域, 3≡0, 逐trit运算, (a+b)%3
//   [层2] [Z/3¹¹Z] 3-adic截断环, 3≠0, 位权3^k, 逢三进一
//   [桥]  [LCM]    (acc×177147)>>16, 层1↔层2唯一合法通道
//
// 编译: g++ -std=c++23 -mavx2 -O3 -fopenmp -fPIC -shared -o vavx3_s2_kernels.so vavx3_s2_kernels.cpp
#include <stdint.h>
#include <immintrin.h>
#include <string.h>
#include <omp.h>
#include "trit_address_bus.h"
#include "../include/sovereign_lcm.h"

extern "C" {

// ═══════════════════════════════════════════════════════════════════════
// [层1] GF(3) 有限域运算 — 3≡0, 逐trit独立, 模3
// ═══════════════════════════════════════════════════════════════════════

// [层1] [GF(3)模3] AVX2逐trit加法: (a+b)%3, 无进位
void gf3_add_batch(const uint8_t* a, const uint8_t* b, int n, uint8_t* result) {
    int i = 0;
    // AVX2: 32 bytes per iteration
    for (; i + 31 < n; i += 32) {
        __m256i va = _mm256_loadu_si256((__m256i*)(a + i));
        __m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
        __m256i vsum = _mm256_add_epi8(va, vb);
        // GF(3) mod: if val >= 3, subtract 3
        __m256i v3 = _mm256_set1_epi8(3);
        __m256i vge3 = _mm256_cmpgt_epi8(vsum, _mm256_set1_epi8(2));
        __m256i vsub = _mm256_and_si256(vge3, v3);
        _mm256_storeu_si256((__m256i*)(result + i), _mm256_sub_epi8(vsum, vsub));
    }
    for (; i < n; i++) {
        uint8_t s = a[i] + b[i];
        result[i] = (s >= 3) ? (s - 3) : s;
    }
}

// [层1] [GF(3)模3] 逐trit减法: (a-b+3)%3
void gf3_sub_batch(const uint8_t* a, const uint8_t* b, int n, uint8_t* result) {
    for (int i = 0; i < n; i++) {
        int d = (int)a[i] - (int)b[i];
        result[i] = (uint8_t)((d + 3) % 3);
    }
}

// [层1] [GF(3)模3] 逐trit乘法: (a×b)%3
void gf3_mul_batch(const uint8_t* a, const uint8_t* b, int n, uint8_t* result) {
    for (int i = 0; i < n; i++) {
        result[i] = (a[i] * b[i]) % 3;
    }
}

// [层1] [GF(3)模3] trit求和后归约到模3
int gf3_sum_mod3(const uint8_t* trits, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) sum += trits[i];
    return sum % 3;
}

// ═══════════════════════════════════════════════════════════════════════
// [层0] [模2编码] 5 trit ↔ 1 byte 打包/解包
// ═══════════════════════════════════════════════════════════════════════
int pack_trits_5(const uint8_t* trits, int n, uint8_t* packed) {
    int out = 0;
    for (int i = 0; i < n; i += 5) {
        uint8_t val = 0;
        for (int k = 0; k < 5; k++) {
            val *= 3;
            if (i + k < n) val += trits[i + k];
        }
        packed[out++] = val;
    }
    return out;
}

int unpack_trits_5(const uint8_t* packed, int num_packed, uint8_t* trits, int max_trits) {
    int divs[5] = {81, 27, 9, 3, 1};
    int idx = 0;
    for (int i = 0; i < num_packed && idx < max_trits; i++) {
        uint8_t val = packed[i];
        for (int k = 0; k < 5 && idx < max_trits; k++) {
            trits[idx++] = (val / divs[k]) % 3;
        }
    }
    return idx;
}

// [层1↔层0] {0,1,2} GF(3) ↔ {-1,0,+1} 平衡三进制 (仅用于兼容接口)
void trits_012_to_balanced(const uint8_t* src, int n, int8_t* dst) {
    for (int i = 0; i < n; i++) dst[i] = (int8_t)src[i] - 1;
}

void trits_balanced_to_012(const int8_t* src, int n, uint8_t* dst) {
    for (int i = 0; i < n; i++) dst[i] = (uint8_t)(src[i] + 1);
}

// ═══════════════════════════════════════════════════════════════════════
// [层4] [拓扑] Christoffel平行移动: S²胞腔间tryte运输
// [层1] GF(3)运算: result = ((query-proto)+shift)*chiral_gate %3
// ═══════════════════════════════════════════════════════════════════════
void christoffel_transport_batch(
    const uint8_t* proto,    // [H, B, 6]
    const uint8_t* query,    // [B, N, H, 6]
    const uint8_t* shifts,   // [B]
    int H, int B, int N,
    uint8_t* result
) {
    // chiral_gate: {0→0, 1→1, 2→1} — T0 absorbs, T1/T2 pass through
    for (int b = 0; b < B; b++) {
        for (int n = 0; n < N; n++) {
            for (int h = 0; h < H; h++) {
                int shift = shifts[b];
                const uint8_t* proto_hb = proto + (h * B + b) * 6;
                const uint8_t* query_bnh = query + ((b * N + n) * H + h) * 6;
                uint8_t* result_bnh = result + ((b * N + n) * H + h) * 6;
                for (int t = 0; t < 6; t++) {
                    // delta = (query - proto) % 3
                    int delta = ((int)query_bnh[t] - (int)proto_hb[t] + 3) % 3;
                    // rotated = (delta + shift) % 3
                    int rotated = (delta + shift) % 3;
                    // chiral gate: {0→0, 1→1, 2→1} — T0 absorbs, T1/T2 pass through as T1
                    result_bnh[t] = (rotated == 0) ? 0 : 1;
                }
            }
        }
    }
}

// [层4] [拓扑] Christoffel混合: 多头transport → trit_sum × x
// transported: [BN, H, 6], x: [BN, D=H*HD], result: [BN, D]
void christoffel_mix_batch(
    const uint8_t* transported, // [BN * H * 6]
    const uint8_t* x,           // [BN * D]
    int BN, int H, int HD,
    uint8_t* result
) {
    int D = H * HD;
    // mix_lut[nz][trit]: precomputed int(trit * nz/6 + 0.5) % 3
    static const uint8_t mix_lut[7][3] = {
        {0, 0, 0},  // nz=0: all zero
        {0, 0, 0},  // nz=1: trit*1/6 → 0
        {0, 0, 1},  // nz=2: 0→0, 1→0, 2→1
        {0, 1, 1},  // nz=3: 0→0, 1→1, 2→1
        {0, 1, 1},  // nz=4: 0→0, 1→1, 2→1
        {0, 1, 2},  // nz=5: 0→0, 1→1, 2→2
        {0, 1, 2},  // nz=6: all pass through
    };
    #pragma omp parallel for
    for (int pos = 0; pos < BN; pos++) {
        for (int h = 0; h < H; h++) {
            int nz = 0;
            const uint8_t* tp = transported + (pos * H + h) * 6;
            for (int j = 0; j < 6; j++) {
                if (tp[j] != 0) nz++;
            }
            const uint8_t* lut_row = mix_lut[nz];
            int h_start = h * HD;
            const uint8_t* xp = x + pos * D + h_start;
            uint8_t* rp = result + pos * D + h_start;
            for (int j = 0; j < HD; j++) {
                rp[j] = lut_row[xp[j] % 3];
            }
        }
    }
}

// [层1] [GF(3)] Gate展开+逐元素乘: result[i] = gate[i/dim] * x[i] % 3
void gf3_gated_mul_batch(
    const uint8_t* gate,  // [N]
    const uint8_t* x,     // [N * dim]
    int N, int dim,
    uint8_t* result
) {
    int total = N * dim;
    #pragma omp parallel for
    for (int i = 0; i < total; i++) {
        uint8_t g = gate[i / dim] % 3;
        result[i] = (g * x[i]) % 3;
    }
}

// [层2] [Z/3¹¹Z] Tryte评估: label = Σ trit_i × 3^i (位权展开→模2十进制)
// [层1] GF(3)损失计算: 手性+五行+非法tryte惩罚
void tryte_eval_batch(
    const uint8_t* trits,  // [K, 6]
    int K,
    int32_t* labels,       // [K]
    float* losses          // [K]
) {
    #pragma omp parallel for
    for (int k = 0; k < K; k++) {
        const uint8_t* t = trits + k * 6;
        // label = Σ t_i × 3^i
        int label = 0;
        int ones = 0, twos = 0;
        for (int i = 0; i < 6; i++) {
            label += (int)t[i] * (i == 0 ? 1 : i == 1 ? 3 : i == 2 ? 9 : i == 3 ? 27 : i == 4 ? 81 : 243);
            if (t[i] == 1) ones++;
            if (t[i] == 2) twos++;
        }
        labels[k] = label;

        int total = ones + twos;
        if (total == 0) { losses[k] = 0.0f; continue; }
        float chiral = (float)(abs(ones - twos)) / (float)total;
        float wuxing = (ones > 0 && twos > 0) ? 0.0f : 1.0f;
        int a1_bad = (t[0]==t[1] && t[1]==t[2] && (t[0]==1 || t[0]==2)) ? 1 : 0;
        int a2_bad = (t[3]==t[4] && t[4]==t[5] && (t[3]==1 || t[3]==2)) ? 1 : 0;
        losses[k] = 0.4f * chiral + 0.4f * wuxing + 0.2f * (float)(a1_bad + a2_bad);
    }
}

// [层2] [A4群] 批量翻转: 层0 float容器中提取trit→层1 GF(3)运算→层2 tryte评估
// 注意: 当前仍在float32容器中操作 (PyTorch残留), 待迁移到SovBlock128
void a4_batch_flip(
    const float* weight,     // [out_dim * in_dim]
    int out_dim, int in_dim,
    const int* rows,         // [K]
    const int* a4_indices,   // [K]
    const int* ops,          // [K]  0=C3CW, 1=C3CCW, 2=Auto, 3=ChiralExch
    int K,
    int* accepted,           // [K] output
    int32_t* old_labels,     // [K] output
    int32_t* new_labels,     // [K] output
    float* old_losses,       // [K] output
    float* new_losses        // [K] output
) {
    int n_a4 = in_dim / 3;  // TRITS_PER_A4 = 3
    int TRYTE = 6;          // TRITS_PER_TRYTE = 6

    #pragma omp parallel for schedule(dynamic, 1)
    for (int k = 0; k < K; k++) {
        int row = rows[k];
        int a4_idx = a4_indices[k];
        int op = ops[k];
        int col_start = a4_idx * 3;  // TRITS_PER_A4 = 3
        int tryte_start = (col_start / TRYTE) * TRYTE;
        int tryte_end = tryte_start + TRYTE;
        if (tryte_end > in_dim) tryte_end = in_dim;

        // --- Extract old tryte ---
        uint8_t old_t[6] = {0};
        float* w_row = (float*)weight + row * in_dim;
        int nt = tryte_end - tryte_start;
        for (int j = 0; j < nt; j++) {
            float v = w_row[tryte_start + j];
            int trit = (int)(v + 0.5f);
            if (trit < 0) trit = 0;
            if (trit > 2) trit = 2;
            old_t[j] = (uint8_t)trit;
        }

        // --- Eval old ---
        int old_ones = 0, old_twos = 0;
        int old_label = 0;
        int pow3[6] = {1, 3, 9, 27, 81, 243};
        for (int j = 0; j < nt; j++) {
            old_label += (int)old_t[j] * pow3[j];
            if (old_t[j] == 1) old_ones++;
            if (old_t[j] == 2) old_twos++;
        }
        int old_total = old_ones + old_twos;
        float old_loss = 0.0f;
        if (old_total > 0) {
            float chiral = (float)abs(old_ones - old_twos) / (float)old_total;
            float wuxing = (old_ones > 0 && old_twos > 0) ? 0.0f : 1.0f;
            int a1_bad = (old_t[0]==old_t[1] && old_t[1]==old_t[2] && (old_t[0]==1||old_t[0]==2)) ? 1 : 0;
            int a2_bad = (nt>=6 && old_t[3]==old_t[4] && old_t[4]==old_t[5] && (old_t[3]==1||old_t[3]==2)) ? 1 : 0;
            old_loss = 0.4f * chiral + 0.4f * wuxing + 0.2f * (float)(a1_bad + a2_bad);
        }
        old_labels[k] = old_label;
        old_losses[k] = old_loss;

        // --- Apply A4 operation ---
        int cs = col_start;
        uint8_t a4[3];
        for (int j = 0; j < 3 && (cs+j) < in_dim; j++) {
            float v = w_row[cs + j];
            int trit = (int)(v + 0.5f);
            if (trit < 0) trit = 0;
            if (trit > 2) trit = 2;
            a4[j] = (uint8_t)trit;
        }

        // Apply GF(3) operation (0=C3CW: +1, 1=C3CCW: +2, 2=Auto: (t+1)%3, 3=ChiralExch: swap pairs)
        if (op == 0) {       // C3_CLOCKWISE: (t+1)%3
            for (int j = 0; j < 3; j++) { a4[j] = (a4[j] + 1) % 3; }
        } else if (op == 1) { // C3_COUNTERCLOCK: (t+2)%3
            for (int j = 0; j < 3; j++) { a4[j] = (a4[j] + 2) % 3; }
        } else if (op == 2) { // AUTOMORPHISM: (2-t)%3
            for (int j = 0; j < 3; j++) { a4[j] = (uint8_t)((6 - a4[j]) % 3); }
        }
        // Chiral exchange (op==3) is handled at Python level (rare, involves paired indices)

        for (int j = 0; j < 3 && (cs+j) < in_dim; j++) {
            w_row[cs + j] = (float)a4[j];
        }

        // --- Extract new tryte ---
        uint8_t new_t[6] = {0};
        for (int j = 0; j < nt; j++) {
            float v = w_row[tryte_start + j];
            int trit = (int)(v + 0.5f);
            if (trit < 0) trit = 0;
            if (trit > 2) trit = 2;
            new_t[j] = (uint8_t)trit;
        }

        // --- Eval new ---
        int new_ones = 0, new_twos = 0;
        int new_label = 0;
        for (int j = 0; j < nt; j++) {
            new_label += (int)new_t[j] * pow3[j];
            if (new_t[j] == 1) new_ones++;
            if (new_t[j] == 2) new_twos++;
        }
        int new_total = new_ones + new_twos;
        float new_loss = 0.0f;
        if (new_total > 0) {
            float chiral = (float)abs(new_ones - new_twos) / (float)new_total;
            float wuxing = (new_ones > 0 && new_twos > 0) ? 0.0f : 1.0f;
            int a1_bad = (new_t[0]==new_t[1] && new_t[1]==new_t[2] && (new_t[0]==1||new_t[0]==2)) ? 1 : 0;
            int a2_bad = (nt>=6 && new_t[3]==new_t[4] && new_t[4]==new_t[5] && (new_t[3]==1||new_t[3]==2)) ? 1 : 0;
            new_loss = 0.4f * chiral + 0.4f * wuxing + 0.2f * (float)(a1_bad + a2_bad);
        }
        new_labels[k] = new_label;
        new_losses[k] = new_loss;

        // --- Decide ---
        accepted[k] = (new_loss <= old_loss + 0.01f) ? 1 : 0;

        // Rollback if not accepted
        if (!accepted[k] && op != 3) {
            // recover original a4 values from old_t
            int a4_offset = cs - tryte_start;
            for (int j = 0; j < 3 && (cs+j) < in_dim; j++) {
                w_row[cs + j] = (float)old_t[a4_offset + j];
            }
        }
    }
}

// [层2] [A4群] TritAddressBus版A4翻转: 通过涡旋测地线地址访问权重
void a4_batch_flip_tritbus(
    void* weight_ptr,        // 权重基址 (float*)
    int out_dim, int in_dim,
    const int* rows, const int* a4_indices, const int* ops, int K,
    int* accepted, int32_t* old_labels, int32_t* new_labels,
    float* old_losses, float* new_losses,
    int vortex_steps           // 涡旋演化步数 (用于地址旋转)
) {
    // 创建主权指针
    TritPointer wp = trit_ptr_wrap(weight_ptr, 0, TRIT_SPACE_WEIGHT);

    // 涡旋演化: 推进地址总线
    if (vortex_steps > 0) {
        trit_ptr_vortex_advance(&wp, vortex_steps);
    }

    float* w_base = (float*)wp.raw_ptr;
    int n_a4 = in_dim / 3;
    int TRYTE = 6;

    #pragma omp parallel for schedule(dynamic, 1)
    for (int k = 0; k < K; k++) {
        int row = rows[k], a4_idx = a4_indices[k], op = ops[k];
        int col_start = a4_idx * 3;
        int tryte_start = (col_start / TRYTE) * TRYTE;
        int tryte_end = tryte_start + TRYTE;
        if (tryte_end > in_dim) tryte_end = in_dim;

        float* w_row = w_base + row * in_dim;

        // Extract old tryte
        uint8_t old_t[6] = {0};
        int nt = tryte_end - tryte_start;
        for (int j = 0; j < nt; j++) {
            float v = w_row[tryte_start + j];
            int trit = (int)(v + 0.5f);
            if (trit < 0) trit = 0; if (trit > 2) trit = 2;
            old_t[j] = (uint8_t)trit;
        }

        // Eval old
        int ones = 0, twos = 0, old_label = 0;
        int pow3[6] = {1,3,9,27,81,243};
        for (int j = 0; j < nt; j++) {
            old_label += (int)old_t[j] * pow3[j];
            if (old_t[j] == 1) ones++; if (old_t[j] == 2) twos++;
        }
        int old_total = ones + twos;
        float old_loss = 0.0f;
        if (old_total > 0) {
            float ch = (float)abs(ones - twos) / (float)old_total;
            float wx = (ones > 0 && twos > 0) ? 0.0f : 1.0f;
            int a1b = (old_t[0]==old_t[1]&&old_t[1]==old_t[2]&&(old_t[0]==1||old_t[0]==2))?1:0;
            int a2b = (nt>=6&&old_t[3]==old_t[4]&&old_t[4]==old_t[5]&&(old_t[3]==1||old_t[3]==2))?1:0;
            old_loss = 0.4f*ch + 0.4f*wx + 0.2f*(float)(a1b+a2b);
        }
        old_labels[k] = old_label; old_losses[k] = old_loss;

        // Apply A4 op
        int cs = col_start;
        uint8_t a4[3];
        for (int j = 0; j < 3 && (cs+j) < in_dim; j++) {
            float v = w_row[cs+j]; int trit = (int)(v+0.5f);
            if (trit<0)trit=0; if(trit>2)trit=2;
            a4[j] = (uint8_t)trit;
        }
        if (op == 0) for(int j=0;j<3;j++) a4[j]=(a4[j]+1)%3;
        else if(op==1) for(int j=0;j<3;j++) a4[j]=(a4[j]+2)%3;
        else if(op==2) for(int j=0;j<3;j++) a4[j]=(uint8_t)((6-a4[j])%3);
        for (int j=0;j<3&&(cs+j)<in_dim;j++) w_row[cs+j]=(float)a4[j];

        // Eval new
        uint8_t new_t[6]={0};
        for(int j=0;j<nt;j++){float v=w_row[tryte_start+j];int trit=(int)(v+0.5f);if(trit<0)trit=0;if(trit>2)trit=2;new_t[j]=(uint8_t)trit;}
        int n1=0,n2=0,nl=0;
        for(int j=0;j<nt;j++){nl+=(int)new_t[j]*pow3[j];if(new_t[j]==1)n1++;if(new_t[j]==2)n2++;}
        int nt2=n1+n2; float nl2=0.0f;
        if(nt2>0){float ch=(float)abs(n1-n2)/(float)nt2;float wx=(n1>0&&n2>0)?0.0f:1.0f;
            int a1b=(new_t[0]==new_t[1]&&new_t[1]==new_t[2]&&(new_t[0]==1||new_t[0]==2))?1:0;
            int a2b=(nt>=6&&new_t[3]==new_t[4]&&new_t[4]==new_t[5]&&(new_t[3]==1||new_t[3]==2))?1:0;
            nl2=0.4f*ch+0.4f*wx+0.2f*(float)(a1b+a2b);}
        new_labels[k]=nl; new_losses[k]=nl2;
        accepted[k]=(nl2<=old_loss+0.01f)?1:0;
        if(!accepted[k]&&op!=3){int ao=cs-tryte_start;for(int j=0;j<3&&(cs+j)<in_dim;j++)w_row[cs+j]=(float)old_t[ao+j];}
    }
}

// [层3] [五行+手性] 融合正则化: 海豚情感+楞严+白鱀豚 6模块→1次C调用
// 注意: 当前使用float32, 待迁移到Q16定点整数
void reg_fused_6in1(
    // 海豚情感: wuxing_weights [5], acc_real [B], acc_imag [B], phase [B], trit_state [B]
    const float* wuxing_w, int B,
    const float* acc_real, const float* acc_imag,
    const int32_t* dolphin_phase, const int32_t* trit_state,
    const int32_t* emotional_pol, const float* chiral_beta,
    const int32_t* zhonglv_count, const int32_t* theta_sync,
    int target_phase,
    // 输出: 6个损失值
    float* dolphin_loss,
    float* shurangama_loss,
    float* baiji_loss,
    float* resonance_sim,
    float* anchor_loss,
    float* spectrum_dr369
) {
    float dl = 0.0f, sl = 0.0f, bl = 0.0f, rs = 0.5f, al = 0.0f, dr = 0.0f;

    #pragma omp parallel sections reduction(+:dl,sl,bl)
    {
        // === 海豚情感 ===
        #pragma omp section
        {
            // wuxing weights alignment
            float wx_target[5] = {0};
            wx_target[target_phase % 5] = 1.0f;
            float wx_diff = 0.0f;
            for (int i = 0; i < 5; i++) {
                float d = wuxing_w[i] - wx_target[i];
                wx_diff += d * d;
            }
            dl += 0.1f * wx_diff / 5.0f;

            // trit alignment
            int target_trit = (target_phase / 5) % 3;
            float trit_sum = 0.0f;
            for (int b = 0; b < B; b++) trit_sum += (float)trit_state[b];
            float trit_avg = trit_sum / (float)B;
            float trit_target = (float)target_trit;
            dl += 0.02f * (trit_avg - trit_target) * (trit_avg - trit_target);

            // chiral flip alignment
            float beta_sum = 0.0f;
            for (int b = 0; b < B; b++) beta_sum += chiral_beta[b];
            dl += 0.01f * (beta_sum / (float)B - 0.5f) * (beta_sum / (float)B - 0.5f);

            // theta sync
            if (zhonglv_count[0] > 0) {
                float theta_ratio = (float)theta_sync[0] / (float)zhonglv_count[0];
                dl += 0.05f * (theta_ratio - 0.33f) * (theta_ratio - 0.33f);
            }
        }

        // === 楞严七大 (简化: 仅计算主要项) ===
        #pragma omp section
        {
            // 地大: 虚实比
            float ratio = acc_imag[0] / (acc_real[0] + 1e-6f);
            sl += 0.1f * fabsf(ratio - 1.0f);

            // 水大: 五行相生闭环 (reward)
            float gen_cycle = wuxing_w[0]*wuxing_w[1] + wuxing_w[1]*wuxing_w[2] +
                              wuxing_w[2]*wuxing_w[3] + wuxing_w[3]*wuxing_w[4] + wuxing_w[4]*wuxing_w[0];
            sl -= 0.05f * gen_cycle;

            // 见大: 斐波那契节点 (简化: check dolphin_phase mod 18)
            int dp = dolphin_phase[0] % 18;
            int fib_nodes[] = {1,2,3,5,8,13};
            for (int i = 0; i < 6; i++) {
                if (dp == fib_nodes[i]) { sl -= 0.02f; break; }
            }
        }

        // === 白鱀豚脑 ===
        #pragma omp section
        {
            // theta ratio
            if (zhonglv_count[0] > 0) {
                float tr = (float)theta_sync[0] / (float)zhonglv_count[0];
                bl += 0.15f * fabsf(tr - 0.3f);
            }
            // water weight penalty
            bl += 0.05f * wuxing_w[4];
            // chiral beta
            float cb = chiral_beta[0];
            bl += 0.08f * fabsf(cb - 0.65f);
            // ratio lock
            float r2 = acc_imag[0] / (acc_real[0] + 1e-6f);
            bl += 0.10f * fabsf(r2 - 1.0f);
        }
    }

    *dolphin_loss = dl;
    *shurangama_loss = sl;
    *baiji_loss = bl;
    *resonance_sim = rs;
    *anchor_loss = al;
    *spectrum_dr369 = dr;
}

// ═══════════════════════════════════════════════════════════════════════
// [层1] GF(3) 乘法表 — 3≡0, 每trit独立, (a×b)%3
// ═══════════════════════════════════════════════════════════════════════
static const uint8_t GF3_MUL[3][3] = {
    {0, 0, 0},  // [层1] 0 × * = 0
    {0, 1, 2},  // [层1] 1 × * = *
    {0, 2, 1},  // [层1] 2 × 2 = 1 (4≡1 mod 3)
};

// [桥] RMSNorm: 层1 GF(3)乘法 + Q16定点缩放 (LCM桥: 2^16定点基)
void gf3_rms_norm_fwd(
    const uint8_t* x,        // [层1] [N, dim] GF(3) trit
    const uint8_t* gamma,    // [层1] [dim] GF(3) gamma
    int N, int dim,          // [层0] 维度
    float eps,               // [层0] 编译期LUT参数
    uint8_t* result          // [层1] [N, dim] 输出
) {
    // 预计算 rsqrt LUT (Q16定点: × SOV_ZHONGLV_BOUNDARY = 65536)
    int32_t* rsqrt_q16 = (int32_t*)alloca((dim + 1) * sizeof(int32_t));
    for (int m = 0; m <= dim; m++) {
        double mean = (double)m / (double)dim;
        double val = 1.0 / sqrt(mean + (double)eps);
        rsqrt_q16[m] = (int32_t)(val * (double)SOV_ZHONGLV_BOUNDARY + 0.5);
    }

    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        const uint8_t* xi = x + i * dim;
        uint8_t* yi = result + i * dim;

        // GF(3)范数: |T0|²=0, |T1|²=1, |T2|²≡1 (2²=4≡1 mod 3)
        int non_zero = 0;
        for (int j = 0; j < dim; j++) {
            if (xi[j] != 0) non_zero++;  // T0→0, T1/T2→1
        }
        int32_t rsqrt = rsqrt_q16[non_zero];

        // 逐元素归一化 (GF(3)乘法 + Q16定点)
        for (int j = 0; j < dim; j++) {
            int prod = GF3_MUL[xi[j]][gamma[j]];  // GF(3)乘法: T2×T2=T1
            int32_t scaled = (prod * rsqrt + 32768) >> SOV_ZHONGLV_SHIFT;
            int y_val = scaled % 3;
            if (y_val < 0) y_val += 3;
            yi[j] = (uint8_t)y_val;
        }
    }
}

// [层1→层2] GF(3) √3门控: 层1 GF(3)范数 + 层0 Q16阈值比较 → 层2 gate
// |H|² = Σ(x_i != 0), gate = |H|² > threshold_q16 ? T1 : T0
// threshold_q16: Q16定点 (如 √3 ≈ 1.732 → 113506)
// x: [N, dim] input trits {0,1,2}
// gate: [N] output gate {0,1}
void gf3_sqrt3_gate(
    const uint8_t* x,       // [N, dim]
    int N, int dim,
    int threshold_q16,      // Q16定点阈值 (LCM-consistent)
    uint8_t* gate           // [N] output {0,1}
) {
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        const uint8_t* xi = x + i * dim;
        int norm_sq = 0;
        for (int j = 0; j < dim; j++) {
            if (xi[j] != 0) norm_sq++;  // x² ≡ (x≠0) in GF(3)
        }
        // 纯整数比较: norm_sq * 65536 > threshold_q16
        gate[i] = ((norm_sq << SOV_ZHONGLV_SHIFT) > threshold_q16) ? (uint8_t)1 : (uint8_t)0;
    }
}

// [层2] Z/3¹¹Z 矩阵乘: 层1 GF(3)逐对乘法 + 层0模2整数累加 → 层1模3归约
// x: [N, in_dim] input trits {T0,T1,T2}
// w: [out_dim, in_dim] weight trits {T0,T1,T2}
// result: [N, out_dim] output trits {T0,T1,T2}
//
// 点积: total = Σ_j GF3_MUL[x_j][w_ij]  (GF3乘法, 普通整数累加)
// 归约: result = total % 3  → {T0,T1,T2}
//
// GF3_MUL: T2×T2=T1 (2×2≡1 mod 3), 非传统 2×2=4
// 累加用 int32_t (in_dim ≤ 10000 可安全容纳)
void gf3_matmul(
    const uint8_t* x,        // [N, in_dim]
    const uint8_t* w,        // [out_dim, in_dim]
    int N, int out_dim, int in_dim,
    uint8_t* result          // [N, out_dim]
) {
    #pragma omp parallel for collapse(2)
    for (int n = 0; n < N; n++) {
        for (int i = 0; i < out_dim; i++) {
            const uint8_t* xn = x + n * in_dim;
            const uint8_t* wi = w + i * in_dim;

            int32_t total = 0;
            for (int j = 0; j < in_dim; j++) {
                total += GF3_MUL[xn[j]][wi[j]];  // GF(3)乘法: T2×T2=T1
            }
            result[n * out_dim + i] = (uint8_t)(total % 3);
        }
    }
}

// ============ [层2] A4 批量翻转 uint8 (纯整数, 零float) ============
// 与 a4_batch_flip 相同逻辑, 但直接操作 uint8 trit 数组
// tolerance: Metropolis接受容差 (0.005=严格, 0.02=宽松)
// 返回: 实际接受的翻转次数
int a4_batch_flip_uint8(
    uint8_t* weight,        // [out_dim * in_dim] trit 数组
    int out_dim, int in_dim,
    int n_flip,              // 翻转次数
    float tolerance          // Metropolis接受容差
) {
    int n_a4 = in_dim / 3;
    int TRYTE = 6;
    int n_accepted = 0;

    for (int k = 0; k < n_flip; k++) {
        int row = rand() % out_dim;
        int a4_idx = rand() % n_a4;
        int op = rand() % 3;  // 0=CW, 1=CCW, 2=Auto
        int col_start = a4_idx * 3;
        int tryte_start = (col_start / TRYTE) * TRYTE;
        int tryte_end = tryte_start + TRYTE;
        if (tryte_end > in_dim) tryte_end = in_dim;
        int nt = tryte_end - tryte_start;

        uint8_t* w_row = weight + row * in_dim;

        // 提取旧 tryte
        uint8_t old_t[6] = {0};
        for (int j = 0; j < nt; j++)
            old_t[j] = w_row[tryte_start + j] % 3;

        // 旧 tryte 损失
        int o1 = 0, o2 = 0;
        for (int j = 0; j < nt; j++) {
            if (old_t[j] == 1) o1++;
            if (old_t[j] == 2) o2++;
        }
        int ot = o1 + o2;
        float old_loss = 0.0f;
        if (ot > 0) {
            float ch = (float)abs(o1 - o2) / (float)ot;
            float wx = (o1 > 0 && o2 > 0) ? 0.0f : 1.0f;
            int a1 = (old_t[0]==old_t[1]&&old_t[1]==old_t[2]&&(old_t[0]==1||old_t[0]==2)) ? 1 : 0;
            int a2 = (nt>=6&&old_t[3]==old_t[4]&&old_t[4]==old_t[5]&&(old_t[3]==1||old_t[3]==2)) ? 1 : 0;
            old_loss = 0.4f * ch + 0.4f * wx + 0.2f * (float)(a1 + a2);
        }

        // 应用 A4 翻转
        int cs = col_start;
        uint8_t a4[3];
        for (int j = 0; j < 3 && (cs+j) < in_dim; j++)
            a4[j] = w_row[cs + j] % 3;

        if (op == 0) for(int j=0;j<3;j++) a4[j] = (a4[j]+1)%3;
        else if(op==1) for(int j=0;j<3;j++) a4[j] = (a4[j]+2)%3;
        else for(int j=0;j<3;j++) a4[j] = (uint8_t)((6-a4[j])%3);

        for (int j = 0; j < 3 && (cs+j) < in_dim; j++)
            w_row[cs + j] = a4[j];

        // 新 tryte 损失
        uint8_t new_t[6] = {0};
        for (int j = 0; j < nt; j++)
            new_t[j] = w_row[tryte_start + j] % 3;

        int n1 = 0, n2 = 0;
        for (int j = 0; j < nt; j++) {
            if (new_t[j] == 1) n1++;
            if (new_t[j] == 2) n2++;
        }
        int nt2 = n1 + n2;
        float new_loss = 0.0f;
        if (nt2 > 0) {
            float ch = (float)abs(n1 - n2) / (float)nt2;
            float wx = (n1 > 0 && n2 > 0) ? 0.0f : 1.0f;
            int a1 = (new_t[0]==new_t[1]&&new_t[1]==new_t[2]&&(new_t[0]==1||new_t[0]==2)) ? 1 : 0;
            int a2 = (nt>=6&&new_t[3]==new_t[4]&&new_t[4]==new_t[5]&&(new_t[3]==1||new_t[3]==2)) ? 1 : 0;
            new_loss = 0.4f * ch + 0.4f * wx + 0.2f * (float)(a1 + a2);
        }

        // 接受/拒绝
        if (new_loss <= old_loss + tolerance) {
            n_accepted++;
        } else {
            // 回滚
            int ao = cs - tryte_start;
            for (int j = 0; j < 3 && (cs+j) < in_dim; j++)
                w_row[cs + j] = old_t[ao + j];
        }
    }
    return n_accepted;
}

} // extern "C"
