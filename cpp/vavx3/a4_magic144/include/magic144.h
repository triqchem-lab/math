/* ============================================================================
 * 144-细胞魔方 - Magic 144 Cell Structure
 *
 * 数学结构：
 * - 12×12 = 144 个细胞排列
 * - 每个细胞包含: 三进制状态、五行相位、手性、ℤ₁₂ 相位
 * - A₄ 群作用: 对 144 个细胞进行置换
 *
 * 知识图谱映射：
 * - 根数学: 能量分布 (144 细胞)
 * - 结构学: 魔方几何 (12×12)
 * - 密度学: 意识态 (细胞状态)
 * - 耦合域: 动力学 (A₄ 作用)
 *
 * 与 4320 的关系:
 * 144 × 30 = 4320 (全息态空间)
 * ============================================================================ */

#ifndef MAGIC144_H
#define MAGIC144_H

#include "fixed_complex.h"
#include "a4_group.h"
#include "magic_square_144.h"
#include <cstdint>
#include <array>
#include <span>
#include <concepts>

/* ══════════════════════════════════════════════════════════════════════
 * 1. 常量定义
 * ══════════════════════════════════════════════════════════════════════ */

constexpr uint16_t MAGIC144_SIZE = 144;
constexpr uint8_t MAGIC144_DIM = 12;  // 12×12
constexpr uint16_t HOLOGRAPHIC_SIZE = 4320;  // 144 × 30

/* ══════════════════════════════════════════════════════════════════════
 * 2. 三进制状态编码 (2-bit per Trit)
 *
 * 编码: T₀=00, T₁=01, T₂=10, 11=error
 * 6 Trits = 12 bits = 729 状态
 * ══════════════════════════════════════════════════════════════════════ */

enum class TritState : uint8_t {
    T0 = 0,  // 00
    T1 = 1,  // 01
    T2 = 2,  // 10
    ERROR = 3  // 11 (非法)
};

/* 6 个 Trits 打包到 12-bit */
struct PackedTernary {
    uint16_t packed : 12;  // 6 Trits × 2 bits = 12 bits

    /* 从 Trit 数组构造 */
    static constexpr PackedTernary from_trits(const std::array<TritState, 6>& trits) {
        uint16_t value = 0;
        for (int i = 0; i < 6; ++i) {
            value |= (static_cast<uint16_t>(trits[i]) & 0x3) << (i * 2);
        }
        return PackedTernary{.packed = value};
    }

    /* 提取第 i 个 Trit */
    constexpr TritState get_trit(int i) const {
        return static_cast<TritState>((packed >> (i * 2)) & 0x3);
    }

    /* 检查是否有错误 Trit (11 模式) */
    constexpr bool has_error() const {
        for (int i = 0; i < 6; ++i) {
            if (get_trit(i) == TritState::ERROR) return true;
        }
        return false;
    }

    /* 解码为整数 (0-728) */
    constexpr uint16_t to_index() const {
        if (has_error()) return 0;
        // 3进制展开: Σ trit[i] × 3^i
        uint16_t idx = 0;
        uint16_t power = 1;
        for (int i = 0; i < 6; ++i) {
            idx += static_cast<uint16_t>(get_trit(i)) * power;
            power *= 3;
        }
        return idx;
    }

    /* 从索引构造 (0-728) */
    static constexpr PackedTernary from_index(uint16_t idx) {
        if (idx >= 729) idx = 728;
        std::array<TritState, 6> trits{};
        for (int i = 0; i < 6; ++i) {
            trits[i] = static_cast<TritState>(idx % 3);
            idx /= 3;
        }
        return from_trits(trits);
    }
};

/* ══════════════════════════════════════════════════════════════════════
 * 3. 五行类型
 * ══════════════════════════════════════════════════════════════════════ */

enum class WuxingType : uint8_t {
    MU = 0,     // 木
    HUO = 1,    // 火
    TU = 2,     // 土
    JIN = 3,    // 金
    SHUI = 4,   // 水
};

/* 五行相生循环: 木→火→土→金→水→木 */
constexpr WuxingType wuxing_sheng_cycle(WuxingType w) {
    return static_cast<WuxingType>((static_cast<uint8_t>(w) + 1) % 5);
}

/* 五行相克关系 */
constexpr WuxingType wuxing_ke_relation(WuxingType w) {
    // 木克土, 火克金, 土克水, 金克木, 水克火
    constexpr std::array<uint8_t, 5> ke_map = {2, 3, 4, 0, 1};
    return static_cast<WuxingType>(ke_map[static_cast<uint8_t>(w)]);
}

/* ══════════════════════════════════════════════════════════════════════
 * 4. 手性
 * ══════════════════════════════════════════════════════════════════════ */

enum class Chirality : uint8_t {
    LEFT = 0,    // 左手性 (阴)
    RIGHT = 1,   // 右手性 (阳)
    NEUTRAL = 2, // 中性
};

/* ══════════════════════════════════════════════════════════════════════
 * 5. 细胞状态 (CellState)
 *
 * 紧凑结构: 64 bits (8 字节)
 * - 三进制状态: 12 bits
 * - 五行类型: 3 bits
 * - 手性: 2 bits
 * - ℤ₁₂ 相位: 4 bits
 * - 振幅 (定点数): 32 bits
 * - 填充: 11 bits (对齐到 64 bits)
 * ══════════════════════════════════════════════════════════════════════ */

struct CellState {
    /* 紧凑存储 (用于序列化/缓存) */
    struct Compact {
        uint64_t
            ternary   : 12,  // PackedTernary
            wuxing    : 3,   // WuxingType
            chirality : 2,   // Chirality
            phase     : 4,   // ℤ₁₂ phase (0-11)
            amp_re    : 20,  // 振幅实部 (缩放)
            amp_im    : 20;  // 振幅虚部 (缩放)
    };

    /* 展开字段 (用于计算) */
    PackedTernary ternary;       // 三进制状态
    WuxingType wuxing;           // 五行类型
    Chirality chirality;         // 手性
    uint8_t phase;               // ℤ₁₂ 相位 (0-11)
    fixed_complex amplitude;     // 复振幅 (定点数)

    /* 构造函数 */
    constexpr CellState()
        : ternary{0}
        , wuxing(WuxingType::MU)
        , chirality(Chirality::NEUTRAL)
        , phase(0)
        , amplitude(0, 0)
    {}

    constexpr CellState(PackedTernary t, WuxingType w, Chirality c,
                       uint8_t p, fixed_complex amp)
        : ternary(t)
        , wuxing(w)
        , chirality(c)
        , phase(p % PHASE_MODULUS)
        , amplitude(amp)
    {}

    /* 默认细胞: 单位振幅，相位 0 */
    static constexpr CellState default_cell() {
        return CellState(
            PackedTernary{0},
            WuxingType::MU,
            Chirality::NEUTRAL,
            0,
            fixed_complex::one()
        );
    }

    /* 检查有效性 */
    constexpr bool is_valid() const {
        return !ternary.has_error() &&
               phase < PHASE_MODULUS &&
               static_cast<uint8_t>(wuxing) < 5 &&
               static_cast<uint8_t>(chirality) < 3;
    }

    /* 相位旋转 */
    constexpr CellState rotate_phase(uint8_t delta) const {
        return CellState(
            ternary, wuxing, chirality,
            phase_add(phase, delta),
            amplitude
        );
    }

    /* C₃ 循环置换 (三进制状态) */
    constexpr CellState c3_permute() const {
        std::array<TritState, 6> trits;
        for (int i = 0; i < 6; ++i) {
            trits[i] = ternary.get_trit(i);
        }
        // C₃: T₀→T₁, T₁→T₂, T₂→T₀
        for (auto& t : trits) {
            if (t == TritState::T0) t = TritState::T1;
            else if (t == TritState::T1) t = TritState::T2;
            else if (t == TritState::T2) t = TritState::T0;
        }
        return CellState(
            PackedTernary::from_trits(trits),
            wuxing, chirality, phase, amplitude
        );
    }

    /* 验证 C₃³ = identity */
    constexpr bool verify_c3_cube() const {
        auto s1 = c3_permute();
        auto s2 = s1.c3_permute();
        auto s3 = s2.c3_permute();
        return s3.ternary.packed == ternary.packed;
    }
};

/* ══════════════════════════════════════════════════════════════════════
 * 6. 144-细胞魔方 (12×12 数组)
 * ══════════════════════════════════════════════════════════════════════ */

struct Magic144 {
    /* 12×12 细胞数组 (行优先) */
    std::array<CellState, MAGIC144_SIZE> cells;

    /* 构造函数: 全部初始化为默认状态 */
    Magic144() {
        for (uint16_t i = 0; i < MAGIC144_SIZE; ++i) {
            cells[i] = CellState::default_cell();
        }
    }

    /* 2D 索引访问 */
    constexpr CellState& at(uint8_t row, uint8_t col) {
        return cells[row * MAGIC144_DIM + col];
    }
    constexpr const CellState& at(uint8_t row, uint8_t col) const {
        return cells[row * MAGIC144_DIM + col];
    }

    /* 1D 索引访问 */
    constexpr CellState& operator[](uint16_t idx) {
        return cells[idx];
    }
    constexpr const CellState& operator[](uint16_t idx) const {
        return cells[idx];
    }

    /* 获取细胞索引 (row, col) → 0-143 */
    static constexpr uint16_t cell_index(uint8_t row, uint8_t col) {
        return row * MAGIC144_DIM + col;
    }

    /* 从 1D 索引获取 (row, col) */
    static constexpr std::pair<uint8_t, uint8_t> cell_coords(uint16_t idx) {
        return {static_cast<uint8_t>(idx / MAGIC144_DIM),
                static_cast<uint8_t>(idx % MAGIC144_DIM)};
    }
};

/* ══════════════════════════════════════════════════════════════════════
 * 7. A₄ 在 144-细胞上的作用
 *
 * 映射策略:
 * - 144 = 12 × 12, 将 12×12 网格看作 A₄ × A₄ 的笛卡尔积
 * - A₄ 有 12 个元素，可以作用在行或列索引上
 *
 * 作用方式:
 * - 行作用: g·(row, col) = (g∘row, col), 其中 row 被解释为 A₄ 元素索引
 * - 列作用: g·(row, col) = (row, g∘col)
 * - 对角作用: g·(row, col) = (g∘row, g∘col)
 * ══════════════════════════════════════════════════════════════════════ */

enum class A4ActionType : uint8_t {
    ROW,     // 行作用
    COL,     // 列作用
    DIAGONAL // 对角作用
};

/* 应用 A₄ 群元素到 144-细胞
 *
 * 返回新的 Magic144 状态 (不修改原状态)
 */
Magic144 a4_apply_to_magic144(const Magic144& src, uint8_t a4_elem,
                               A4ActionType action);

/* 原地应用 A₄ (使用缓冲区) */
void a4_apply_to_magic144_inplace(Magic144& m, uint8_t a4_elem,
                                   A4ActionType action);

/* 验证 A₄ 作用的群同态性质:
 * (gh)·m = g·(h·m)
 */
bool a4_verify_action_homomorphism(const Magic144& m, uint8_t g, uint8_t h);

/* ══════════════════════════════════════════════════════════════════════
 * 8. 初始化函数
 * ══════════════════════════════════════════════════════════════════════ */

/* 用 144 阶幻方相位偏置表初始化 144-细胞 (C=2 缠绕) */
void magic144_init_with_wuxing(Magic144& m);

/* 用 ℤ₁₂ 相位图案初始化 */
void magic144_init_with_phase_pattern(Magic144& m);

/* 用三进制格雷码初始化 */
void magic144_init_with_ternary_gray(Magic144& m);

/* ══════════════════════════════════════════════════════════════════════
 * 9. 查询和验证 — 拓扑不变量 (非概率)
 * ══════════════════════════════════════════════════════════════════════ */

/* 计算陈数 C — 通过 plaquette Berry curvature 求和
 *
 * 宇宙全息物理学 v1.0:
 * - 禁止概率守恒 (Σ|ψ|² 是光锥退化规则)
 * - 使用拓扑不变量: 陈数 C=2
 *
 * 返回: 陈数 × FIXED_SCALE (定点数整数)
 *       期望值: 2 * FIXED_SCALE = 131072
 */
int64_t magic144_chern_number(const Magic144& m);

/* 验证拓扑不变量在 A₄ 作用下保持不变
 *
 * 测试: 陈数在 A₄ 群作用下不变
 */
bool magic144_verify_topological_invariant(const Magic144& m,
                                            int64_t expected_chern);

/* 归零公理验证: 1²+i²=0²
 *
 * 宇宙全息物理学 v1.0 核心公理:
 * - 在归零状态下，复振幅为零
 * - 触发相位重置到参考态
 *
 * 返回: 是否满足归零公理
 */
bool magic144_verify_zeroing(const Magic144& m);

/* 打印 144-细胞的相位分布 (12×12 网格) */
void magic144_print_phase_grid(const Magic144& m);

/* 打印三进制状态分布 */
void magic144_print_ternary_grid(const Magic144& m);

/* 统计五行类型分布 */
std::array<uint16_t, 5> magic144_count_wuxing(const Magic144& m);

/* 统计手性分布 */
std::array<uint16_t, 3> magic144_count_chirality(const Magic144& m);

/* 验证所有细胞的有效性 */
bool magic144_verify_all_cells_valid(const Magic144& m);

/* ══════════════════════════════════════════════════════════════════════
 * 10. Christoffel 联络矩阵 (离散版本)
 *
 * 宇宙全息物理学 v1.0:
 * - 五行生克关系 → 联络矩阵的非零元
 * - 联络 Γ 是 144×144 的稀疏矩阵
 * - 演化: ψ' = Γ·ψ (一步联络演化)
 *
 * 内存优化: 使用 CSR (Compressed Sparse Row) 格式
 * ══════════════════════════════════════════════════════════════════════ */

/* Christoffel 联络矩阵 — CSR 稀疏格式 */
struct ChristoffelConnection {
    static constexpr uint16_t MAX_NONZERO = 144 * 8;  // 每行最多 8 个非零元 (4邻接+4对角)

    /* CSR 格式 */
    fixed_complex values[MAX_NONZERO];   // 非零元素值 (五行相位)
    uint16_t col_indices[MAX_NONZERO];   // 列索引
    uint16_t row_ptr[145];               // 行指针 (144+1)
    uint16_t nnz;                        // 实际非零元素数

    /* 构造函数: 零初始化 */
    ChristoffelConnection() : nnz(0) {
        for (uint16_t i = 0; i < 145; ++i) row_ptr[i] = 0;
    }
};

/* 初始化 Christoffel 联络矩阵
 *
 * 五行生克关系定义非零模式:
 * - 相生: 相邻细胞之间有连接 (振幅 = WuxingPhase::SHENG)
 * - 相克: 跨细胞之间有连接 (振幅 = WuxingPhase::KE)
 * - 背克: 反向连接 (振幅 = WuxingPhase::BEI_KE)
 */
void christoffel_init(ChristoffelConnection& conn);

/* 一步联络演化: ψ' = Γ·ψ
 *
 * 使用 CSR 矩阵-向量乘法 (定点数)
 */
void christoffel_evolve(const ChristoffelConnection& conn,
                        const Magic144& src, Magic144& dst);

/* 验证联络矩阵的五行对称性
 *
 * 测试: Γ(i,j) ≠ 0 ⟺ Γ(j,i) ≠ 0 (对称连接)
 */
bool christoffel_verify_wuxing_symmetry(const ChristoffelConnection& conn);

/* 验证演化后拓扑不变量保持
 *
 * 测试: 演化后陈数不变 (C=2 保持)
 */
bool christoffel_verify_topological_preservation(const ChristoffelConnection& conn,
                                                   const Magic144& src);

#endif /* MAGIC144_H */
