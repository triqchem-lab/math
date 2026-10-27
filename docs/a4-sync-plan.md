# A₄/2A₄ 族三语言同步 + C ABI 1.1 落点表（2026-10-28）

> **口径**：`hs/sov-math`（commit `cb3bcc1`）是当前最新数学状态，`/data/work/discrete-mathematics` 是唯一数学权威。
> 本表是**施工图**：每个 hs 模块给出 cpp 头、rust 模块、ABI 符号三处落点。
> 判据沿用既有一致性流程（`docs/abi.md` §四）：**三语言逐符号等价 + 同一向量逐行相同**。

## 0. 基线（本次动工前实测）

`bash hs/sov-math/engineering/abi_check.sh` → **全绿**：71 符号 / 23 头 / 1126 行三语言一致 / 22 项闸门通过 / 与 v1.0 基线零差异。

缺口（`hs` 有、`cpp`/`rust` 无）：

| 缺口 | 模块 | 行数 | cpp | rust |
|---|---|---:|---|---|
| A₄/2A₄ 族 | 13 个（见 §1） | 1690 | ❌ | ❌ |
| Validation 族 | `Zomega` `ChiInner` `GF9Diag` `GF4Magic` `Magic` `EM` | 726 | ❌ | ✅（源） |
| 内核件 | NSE / TimeCrystal / Soliton | — | ✅ | ⚠ 仅 FFI 内 / 缺 / `f64` |

## 1. A₄/2A₄ 族逐模块落点

命名冲突声明：hs 的 `Sov.Math.Field` 是**关联丛截面**，与 `cpp/include/gf3_field.h`（GF(3) 域）**不是同一对象**；
本表统一改名 `section`（`section_field.h` / `section.rs`），hs 侧名字保留不动（避免破坏已被引用的模块路径）。

| # | hs 模块 | 行 | cpp 头（`namespace sov::math`） | rust（`sov-a4` crate） | 依赖 |
|---|---|---:|---|---|---|
| 1 | `A4.hs` | 121 | `a4_group.h` — `A4{Id,Rot(i,j),Flip(k)}`、`all_a4/perm/index/of_perm/mul/inv`、`gen_g1/gen_g2/generated` | `a4.rs` | — |
| 2 | `SL23.hs` | 109 | `sl23.h` — `M2{a,b,c,d}`、`all_sl23/mat_id/mul/det/inv/pow/order`、`sigma1/sigma2/neg_i/w_element`、`elems_of_order4`、`act_vec/nonzero_vectors/orbit_of_vec` | `sl23.rs` | `a4_group.h` |
| 3 | `Cover2A4.hs` | 94 | `cover_2a4.h` — `line_reps/canon_line/line_index/perm_of_m/compose_p/is_even_p/image_perms/kernel_of_cover/cover_ok` | `cover.rs` | `a4_group.h`, `sl23.h` |
| 4 | `Complex2A4.hs` | 226 | `complex_2a4.h` — `M2E`（Eis 矩阵）、`mul_m/id_m/inv_m/det_m/trace_m/pow_m/order_of_m`、`i_gen/j_gen/u_gen/group24`、`conj_class_m/classes_m/class_info_m`、`char_by_size_order/spinor_char_from_model`、`m_index/one_dim_char/tensor_rep/tensor_faithful/tensor_hom/model_ok` | `complex2a4.rs` | `eisenstein.h` |
| 5 | `RepA4.hs` | 167 | `rep_a4.h` — `Irrep{R1,R1p,R1pp,R3}`、`ChiClass{CE,CV4,CA,CB}`、`class_of/chi/chi_orth/regular_character/regular_multiplicity_num/col_orth_at_id/one_dim_reps_non_faithful` | `rep_a4.rs` | `a4_group.h`, `eisenstein.h` |
| 6 | `Rep2A4.hs` | 275 | `rep_2a4.h` — 7 共轭类（计算序）、`Spinor{S2,S2p,S2pp}`、`one_dim_homs/one_dim_chars/three_dim_char/all_chars`、`char_pairing_l/all_chars_orthogonal/all_chars_distinct`、`regular_char/regular_multiplicity/regular_multiplicity_ok`、`predicted_dims/dims_identity` | `rep_2a4.rs` | `complex_2a4.h`, `sl23.h` |
| 7 | `CarrierA4.hs` | 65 | `carrier_a4.h` — `Carrier = std::array<Eis,12>`、`dim_carrier/zero_c/basis_vec/dot_c/norm_c2/plancherel_sum/reg_act` | `carrier_a4.rs` | `rep_a4.h` |
| 8 | `Carrier2A4.hs` | 202 | `carrier_2a4.h` — `Field = std::array<Eis,24>`（按 `all_sl23` 索引）、`dim_field/zero_f/basis_at/dot_f/norm_f2/eq_f/reg_act2/matrix_coeff`、`coefficient_gram/gram_hermitian/gram_det/coefficients_independent`、`epsilon/bilinear_gram/bilinear_schur/frobenius_schur_num/quaternionic/carrier2_ok` | `carrier_2a4.rs` | `complex_2a4.h` |
| 9 | `Hopf.hs` | 131 | `hopf_quotient.h` — `T6Point = std::array<Trit,6>`、`all_t6_points/act_on/orbit_of/fiber_size_of/proj_class/proj_key/base_card/orbit_reps/fiber_size_distribution` | `hopf.rs` | `a4_group.h`, `tryte.h` 语义 |
| 10 | `SpinHopf.hs` | 80 | `spin_hopf.h` — `spin_fiber/spin_total_card/alpha_mat/alpha_pow/phase_act/spin_act/spin_proj/spin_orbit_reps/phase_orbits` | `spin_hopf.rs` | `hopf_quotient.h`, `sl23.h` |
| 11 | `Spin2.hs` | 77 | `spin_criteria.h`（2 维段）— `spinor_a/spinor_b/ab_mat/ba_mat/commutator2/det_commutator2/abv_*/bav_*/no_common_eigenvector2/spin2_ok` | `spin.rs` | `complex_2a4.h` |
| 12 | `Spin3.hs` | 143 | `spin_criteria.h`（3 维段）— `mat_a3/mat_b3/mat_d3/ab3/ba3/ad3/da3/mul_v3/mul_m3/row_c/row_e/row_e2/det_rows/no_common_eigenvector3/spin3_ok` | `spin.rs` | `eisenstein.h` |
| 13 | `Field.hs` | 170 | `section_field.h` — `BaseKey/Section/base_dim/fiber_dim/flat_dim/effective_dim/overcount/zero_section/constant_section/section_norm/free_orbit_count/non_free_orbit_count/min_fiber_size/max_fiber_size/uniform_coords_faithful/overcount_by_orbit/overcount_decomposition/equivariant_section_possible` | `section.rs` | `carrier_a4.h`, `hopf_quotient.h` |

**强类型纪律沿用 hs**：A₄ 用封闭构造子（cpp 用 `enum class Kind` + 变体字段，非法 `Rot 9 9` 不可构造）；
`Carrier`/`Field` 用定长数组（长度错在类型层被拒）；零浮点、零 `partial` 语义（cpp 用 `constitutional_panic` 对应 `std::abort`）。

**测试移植**：`hs/sov-math/test/Sov/Tests/Core.hs` 中 `carrierTests`/`hopfTests`/`repA4Tests`/`rep2A4Tests`/`sl23Tests`/`spinHopfTests`/`fieldTests` + Spin2/Spin3 共 **~90 条断言**，逐条移植为 `cpp/tests/test_a4_family.cpp` 与 `rust/sov-math/sov-a4/tests/`。

## 2. cpp Validation 6 模块（镜像 rust 源）

rust 是这 6 个设施的**源头**（hs 注释自认「镜像 rust」）：

| rust 源 | cpp 落点 |
|---|---|
| `sov-validation/src/zomega.rs` (101) | `cpp/include/val_zomega.h` |
| `sov-validation/src/chi_inner.rs` (60) | `cpp/include/val_chi_inner.h` |
| `sov-validation/src/gf9_diag.rs` (126) | `cpp/include/val_gf9_diag.h` |
| `sov-validation/src/gf4_magic.rs` (393) | `cpp/include/val_gf4_magic.h` |
| `sov-validation/src/magic_square.rs` (149) + `ternary_magic.rs` (177) | `cpp/include/val_magic_square.h` |
| `sov-validation/src/em_*.rs` (156+237) | `cpp/include/val_em.h` |

命名前缀 `sov::math::val::`，避免与内核层同名；测试 `cpp/tests/test_validation.cpp` 复用 rust 的判据常量。

## 3. rust 内核件补齐（去 `f64`）

| 缺失 | 现有违规 | 落点 |
|---|---|---|
| NSE 求解器（内核级，非 FFI 内嵌） | 仅 `sov-guard/src/sov_ffi.rs` 内有 `NseHandle` | `sov-core/src/l2_nse.rs`：三模式（启发式 0/1 + 形式化 `ns_step` 模式 2），逐条对齐 `cpp/include/nse_solver.h` 与 Agda `NSEOnT6.agda` |
| TimeCrystal | 无 | `sov-topology/src/time_crystal.rs`，对齐 `cpp/include/time_crystal.h` 与 hs `Topology/TimeCrystal.hs` |
| Soliton | `sov-topology/src/lib.rs` 用 `f64`（`compute_rho`/`SOLITON_EIGEN_*`） | 新增 `sov-topology/src/soliton.rs`：Q32 `sin` / Q16 `rho` / 相位阈值，整数配方与 cpp `nayin_soliton_l5.h`、hs `Topology/Soliton.hs` 一致；`lib.rs` 的 `f64` 版保留但标 `#[deprecated]`（**不删**，宪法要求需用户裁决） |

## 4. C ABI 1.1 符号表（新增，向后兼容）

版本：`SOV_ABI_MAJOR=1`、`SOV_ABI_MINOR=1`（1.0 的 71 符号一个不动）。
新增 4 个设施头，共 **48 符号** → 总计 **119 符号 / 27 头**。

### `<sov/a4.h>`（17）

```
sov_a4_card(uint8_t* out)                                    /* 12 */
sov_a4_perm(uint8_t elem, uint8_t out[4])
sov_a4_index(const uint8_t perm[4], uint8_t* out)            /* 奇置换 → -2 */
sov_a4_mul(uint8_t g, uint8_t h, uint8_t* out)
sov_a4_inv(uint8_t g, uint8_t* out)
sov_a4_order(uint8_t g, uint8_t* out)                        /* 1|2|3 */
sov_sl23_card(uint16_t* out)                                 /* 24 */
sov_sl23_mul(const uint8_t g[4], const uint8_t h[4], uint8_t out[4])
sov_sl23_det(const uint8_t g[4], uint8_t* out)               /* 1 / -2 if not in GL */
sov_sl23_inv(const uint8_t g[4], uint8_t out[4])
sov_sl23_pow(const uint8_t g[4], uint32_t n, uint8_t out[4])
sov_sl23_order(const uint8_t g[4], uint8_t* out)
sov_sl23_order4_count(uint8_t* out)                          /* 6 */
sov_sl23_w_is_neg_i(uint8_t* out)                            /* 1 */
sov_cover_perm(const uint8_t g[4], uint8_t out[4])
sov_cover_kernel_size(uint8_t* out)                          /* 2 */
sov_cover_ok(uint8_t* out)                                   /* 1 = 同态+像=A₄+核={±I} */
```

### `<sov/rep.h>`（15）

```
sov_a4_chi(uint8_t irrep, uint8_t elem, int64_t* re, int64_t* im)
sov_a4_chi_orth(uint8_t rho, uint8_t sigma, int64_t* re, int64_t* im)
sov_a4_regular_mult(uint8_t irrep, int64_t* re, int64_t* im) /* 12·d_ρ */
sov_a4_col_orth_at_id(int64_t* re, int64_t* im)              /* 12 */
sov_a4_one_dim_nonfaithful(uint8_t* out)
sov_rep2_class_sizes(uint8_t out[7])
sov_rep2_class_info(uint8_t out[14])                         /* (size,order)×7 */
sov_rep2_char(uint8_t row, uint8_t cls, int64_t* re, int64_t* im)
sov_rep2_char_orth(uint8_t row_a, uint8_t row_b, int64_t* re, int64_t* im)
sov_rep2_dims(uint8_t out[7])
sov_rep2_spinor_dim(uint8_t* out)                            /* 2 */
sov_spin2_det_commutator(int64_t* re, int64_t* im)           /* 2 */
sov_spin2_no_common_eigenvector(uint8_t* out)
sov_spin3_det_rows(int64_t* re, int64_t* im)                 /* 4 */
sov_spin3_no_common_eigenvector(uint8_t* out)
```

### `<sov/hopf.h>`（9）

```
sov_hopf_base_card(uint16_t* out)                            /* 135 */
sov_hopf_proj(uint16_t point, uint16_t* out)                 /* point < 729 */
sov_hopf_fiber_size(uint16_t point, uint8_t* out)
sov_hopf_fiber_distribution(uint8_t out[8])                  /* (size,count)×4 */
sov_spin_fiber_card(uint8_t* out)                            /* 9 */
sov_spin_total_card(uint32_t* out)                           /* 6561 */
sov_spin_phase_act(uint32_t k, const uint8_t v[2], uint8_t out[2])
sov_spin_alpha_pow(uint32_t k, uint8_t out[4])
sov_section_dims(uint32_t out[5])                            /* 135,12,1620,729,891 */
```

### `<sov/carrier.h>`（7）

```
sov_carrier_a4_dim(uint8_t* out)                             /* 12 */
sov_carrier_a4_plancherel(uint16_t* out)                     /* 12 */
sov_carrier_a4_basis_norm(uint8_t elem, int64_t* re, int64_t* im)
sov_carrier_2a4_dim(uint8_t* out)                            /* 24 */
sov_carrier_2a4_bilinear_schur(uint8_t* out)                 /* 1 */
sov_carrier_2a4_fs_indicator(int64_t* out)                   /* −1 */
sov_carrier_2a4_coefficient_gram_det(int64_t* re, int64_t* im)
```

约定沿用 `docs/abi.md` §二：`int32_t` 状态码、调用方缓冲区、库内零分配、无异常；
下标/元素越界 → `SOV_ERR_RANGE`；非法 trit → `SOV_ERR_TRIT`；空指针 → `SOV_ERR_NULL`。

## 5. 施工顺序与验收

1. **s1 cpp A₄ 族** → `cmake --build` + `test_a4_family` 全绿（~90 断言）
2. **s2 rust A₄ 族** → `cargo test -p sov-a4` 全绿（同断言集）
3. **s3 cpp Validation** → `test_validation` 全绿（判据常量与 rust 同源）
4. **s4 rust 内核件** → `cargo test -p sov-topology -p sov-core` 全绿；`grep f64` 仅剩标注保留项
5. **s5 ABI 1.1** → 新头 → `gen_export_map.sh` → 三语言实现 → `ffi_conformance.c` 新向量 → `abi_check.sh --update`
6. **s6 闸门** → `abi_check.sh` 全绿（119 符号、>1126 行）
7. **s7 udk 评估** → `docs/udk-integration.md`

跨语言等价判据（每条必须能机械复跑）：
- cpp 与 rust 的输出逐符号相同（`math_parity` 导出面：新 ABI 向量在 `ffi_conformance` 内共用一份）
- hs 与 cpp/rust 的 ABI 行为逐行相同（现有 `ffi_conformance.sh` 机制不变）
- 与 Agda 的锚定：A₄ 族的新断言在 `verify_agda_anchors.py` 里登记（覆盖/正交性/维数恒等式）
