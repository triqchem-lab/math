# sov-ffi C ABI 契约 (v1.0)

> **这是我们对外的唯一承诺面**：`dist/include/sov/` 的头 + 动态库导出的符号。
> 三语言实现（cpp / rust / hs）**逐符号等价**，可由任意 C/C++/带 FFI 的宿主加载。
> 单一事实源：`cpp/include/sov/*.h`（版本脚本 `cpp/packaging/sov_ffi.map` 由此自动生成）。

## 一、符号清单（70 个，分 23 个设施头）

| 头 | 符号数 | 符号 |
|---|---|---|
| `<sov/version.h>` | 4 | `sov_abi_version`, `sov_strerror`, `sov_runtime_init`, `sov_runtime_fini` |
| `<sov/trit.h>` | 4 | `sov_trit_add`, `sov_trit_mul`, `sov_trit_sun`, `sov_trit_c3` |
| `<sov/gf9.h>` | 2 | `sov_gf9_mul`, `sov_gf9_norm` |
| `<sov/ring.h>` | 4 | `sov_ring_add`, `sov_ring_mul`, `sov_ring_double`, `sov_ring_to_tryte` |
| `<sov/dc.h>` | 6 | `sov_dc_mixedop`, `sov_dc_sigma`, `sov_dc_crt`, `sov_dc_translate`, `sov_dc_phase_vec`, `sov_dc_rotate_vec` |
| `<sov/dayan.h>` | 2 | `sov_dayan_multiplier`, `sov_crt2` |
| `<sov/width.h>` | 4 | `sov_width_capacity_trits`, `sov_width_capacity_bits`, `sov_width_trits_for_bits`, `sov_width_bits_for_trits` |
| `<sov/adc.h>` | 1 | `sov_adc64` |
| `<sov/bridge.h>` | 1 | `sov_lcm_step` |
| `<sov/l6.h>` | 3 | `sov_zhonglv_step`, `sov_zhonglv_closure`, `sov_sovereign_lcm` |
| `<sov/u384.h>` | 3 | `sov_u384_add`, `sov_u384_mul_u64`, `sov_u384_shr16` |
| `<sov/nse.h>` | 8 | `sov_nse_new`, `sov_nse_step`, `sov_nse_density_q16`, `sov_nse_step_count`, `sov_nse_cell`, `sov_nse_component`, `sov_nse_set_component`, `sov_nse_free` |
| `<sov/t6.h>` | 2 | `sov_t6_index`, `sov_is_standing_node` |
| `<sov/wide.h>` | 3 | `sov_wide_rho`, `sov_wide_const_pi`, `sov_wide_const_sqrt3` |
| `<sov/chern.h>` | 4 | `sov_chern_inject`, `sov_chern_detect`, `sov_chern_inject_monopole`, `sov_chern_compute` |
| `<sov/sunyi.h>` | 4 | `sov_sunyi_length`, `sov_sunyi_gain`, `sov_sunyi_boundary`, `sov_chiral_collapse` |
| `<sov/soliton.h>` | 3 | `sov_soliton_sin_q32`, `sov_soliton_rho_q16`, `sov_soliton_phase` |
| `<sov/eis.h>` | 2 | `sov_eis_mul`, `sov_eis_norm` |
| `<sov/holo.h>` | 1 | `sov_holographic` |
| `<sov/digit.h>` | 2 | `sov_digital_root`, `sov_doz_digit` |
| `<sov/doz.h>` | 2 | `sov_format_doz`, `sov_parse_doz` |
| `<sov/pack.h>` | 2 | `sov_trit_pack5`, `sov_trit_unpack5` |
| `<sov/tryte.h>` | 2 | `sov_tryte_value`, `sov_tryte_unpack` |

契约外符号 **零容忍**：`engineering/verify_export_surface.sh` 断言三语言 dynsym **恰好等于**上表
（缺失 = ABI 不完整；多出 = 导出面失控）。实测：hs 未加版本脚本时泄漏 1224 个 GHC 运行时符号。

## 二、调用约定（所有符号一致）

| 项 | 约定 |
|---|---|
| **返回值** | `int32_t` 状态码：`SOV_OK=0`；`SOV_ERR_TRIT=-1`（分量 ∉ {0,1,2}）；`SOV_ERR_RANGE=-2`（相位/下标/长度越界）；`SOV_ERR_STATE=-3`；`SOV_ERR_NULL=-4`（空指针） |
| **输出参数** | 一律**调用方分配缓冲区**，指针末尾写入；库内**零分配** ⇒ 无 free 语义、无跨语言所有权问题 |
| **唯一例外：NSE 句柄** | `sov_nse_new` **由库分配**，必须经 `sov_nse_free` 释放（唯一入口；`free(NULL)` 合法无操作）。句柄内部布局**不进 ABI**（不透明），故可演进。单句柄非线程安全；不同句柄互不影响 |
| **输入校验** | 所有指针先判空（→ −4）；三进制分量判 ∈ {0,1,2}（→ −1）；相位判 ∈ [0,4)（→ −2）；长度不足（→ −2） |
| **字符串返回** | 仅 `sov_strerror` 返回 `const char*`：**静态存储、无需释放、进程生命周期有效** |
| **缓冲区布局** | `sov_ring_*`：11 个小端 trit；`sov_lcm_step`：6×`uint64` 小端；`sov_wide_*`：8×`uint64` 小端幅度 / 2^384；`sov_chern_*`：864 字节 = [head:12][cell:12][trit:6] |
| **线程安全** | 所有函数**可重入**（纯函数 + 调用方缓冲区，无共享可变状态）；例外：`sov_runtime_init/fini` 仅限进程级一次配对调用 |
| **运行时** | `sov_runtime_init()` 必须先调用（hs 实现据此调 `hs_init`；cpp/rust 为空操作）；退出前 `sov_runtime_fini()` 配对 |
| **异常** | 不抛异常、不 longjmp；错误一律经状态码返回 |

## 三、版本与兼容策略

- 版本宏：`SOV_ABI_MAJOR`/`SOV_ABI_MINOR`、`SOV_ABI_VERSION_ENCODE`、`SOV_ABI_AT_LEAST(maj,min)`、`sov_abi_version()`
- **兼容规则**：新增符号 ⇒ 升 MINOR（向后兼容）；改签名/语义/删符号 ⇒ 升 MAJOR（破坏性）
- **ELF 符号版本**：`SOV_FFI_1.0`（版本脚本节点）；`libsov_ffi.so.1`（soname）
- **语言档位**：头文件支持 C99..C23 与 C++11..C++23（19 档位 × gcc/clang 矩阵全绿），
  语言探测宏 `SOV_HAS_C99/C11/C17/C23`、`SOV_HAS_CXX11/17/23`
- **合规**：标识符全部落 `sov_`/`SOV_` 私有前缀，不侵占 ISO 保留空间（C99 §7.1.3/§7.26）；
  每个头单头自足（§7.1.2）；见 `engineering/c99_compliance_check.py`

## 四、闸门（改 ABI 必须同时过）

| 闸门 | 命令 | 判据 |
|---|---|---|
| 导出面 | `engineering/verify_export_surface.sh` | 三语言 dynsym == 契约（缺/多皆红） |
| 实现一致 | `engineering/ffi_conformance.sh` | 三语言同一向量 **1011 行逐行相同** |
| 头合规 | `engineering/c99_compliance_check.py` | 保留标识符 / 自足性 / 版本闸门 |
| 档位矩阵 | `engineering/c_standard_matrix.sh` | C99..C23 × C++11..23 × gcc/clang |
| 安装 | `engineering/install_c_abi.sh` | `dist/` 标准布局 + pkg-config 三套 |

## 五、语义约定中必须显式规定的边界（实测踩过）

| 项 | 规定 | 踩坑记录 |
|---|---|---|
| 负数整除 | `sov_zhonglv_step` 的 `/2^16` **向零截断**（C/Rust 原生语义；hs 必须用 `quot` 而非 `div`） | 初版 hs 用 `div`（向下取整）⇒ `zl4` 分歧（7881498391 vs 7881497637），一致性测试抓出 |
| trit 越界 | 所有接收 trit 的接口逐个校验 ∈ {0,1,2} → `SOV_ERR_TRIT` | rust 侧陈数接口初版漏校验 ⇒ `err_chern_trit` 分歧 |
| 长度不足 | 缓冲类接口 `len < 需求` → `SOV_ERR_RANGE`（不是 −1） | 陈数接口统一后对齐 |
| 除数 / 相位 | `lcm <= 0` → `SOV_ERR_RANGE`；相位 ∉ [0,4) → `SOV_ERR_RANGE` | — |
| Doz 大小写 | 只接受大写 `A`/`B`；小写 → `SOV_ERR_RANGE`（bad-doz TRAP 的解析端对应） | — |
| Doz 溢出 | 解析累加超出 `uint64` → `SOV_ERR_RANGE`（18 位 `B`×18 = 12^18−1 > 2^64−1） | 初版测试用了 12^16（≈1.85e17，**未溢出**）⇒ 分支未被覆盖，已换真溢出向量 |
| Doz 缓冲 | `cap < digits + 1` → `SOV_ERR_RANGE`；结果 NUL 结尾，`written` 不含 NUL | — |
| **128 位中间量** | `sov_u384_*` 的进位/溢出**不丢失**：`carry_out` 回传超出 2^384 的部分（`u384_add` 传 1 位、`mul_u64` 传 64 位） | 与 `sov_lcm_step` 的「只回绕 limb1」语义不同 —— 后者是既有状态机语义，前者是纯算术原语，二者并存且都显式文档化 |
| **定点中间量宽度** | Q32 正弦 Taylor 与 ρ 的 `sq²` **必须 128 位中间量**（C: `__int128`；Rust: `i128`） | int64 溢出：`x²≈4.55e19 > 2^63`、`2^32² = 2^64` ⇒ cpp/rust 得 `-32768` 而 hs（`Integer` 任意精度）得 `32768`；一致性测试抓出 |
| 相态编号 | **沿用 cpp/rust 既有语义：0(Solid) < 2(Liquid) < 1(Curie) < 3(Superfluid) —— 非单调** | **人类裁决「兼容优先」：保持既有编号不重编号**；头文件以 `SOV_PHASE_NUMBERING_FROZEN` 显式冻结，改动需升 MAJOR |

## 五点五、NSE 语义分叉（**实测发现，不是推测**）

cpp 的 `NSESolver::step` 与 rust/hs 的 `step_once` 在 delta 公式上不同：

| 实现 | delta | 1 步后是否空间相关 |
|---|---|---|
| cpp `sunyi_step(old, s+p+t)` | `((s+p+t) 偶 ? 1 : 2)` | **是**（半格点 +1、半格点 +2） |
| rust / hs `(old + (s 偶 ? 1 : 2))` | `(s 偶 ? 1 : 2)` | 否（全格点同 delta） |

实测（Python 独立复算，144×46=6624 格点，初态全 0）：**1 步后 3312/6624 = 50.0% 格点不同**；
二者其余部分（ρ≥0.38 的 C3 相位调制、1500 周期、12 步仲吕）完全一致 ⇒ 分歧就是这一行。

**ABI 的处理（人类指示：原理查形式化库）**：两种启发式泵浦保留为 mode 0/1，
并新增 **mode 2 = 形式化 `nsStep`**，其定义**逐条对应** Agda 项目
`Sovereign/Application/Problems/NavierStokes/NSEOnT6.agda`：

| Agda 定义 | ABI 实现 |
|---|---|
| `Torus6 = C3 × … × C3`（729 点） | `x ∈ [0,729)` 基-3 编码 |
| `Field = 6 × ScalarField` | `comp[i*729 + x]`，6 分量 × 729 格 |
| `shiftAt i`（坐标 i +1 mod 3） | `shift_at(i, x)` = `x + ((digit+1)%3)·3^i − digit·3^i` |
| `diffF i f x = f(shiftAt i x) ⊕ negate(f x)` | `tadd(at(shift,x), tneg(at(x)))` |
| `div v x = sum6 (diffF 0 v₁) … (diffF 2 v₆)` | 六项 ⊕ 折叠 |
| `grad f = (diffF 0 f, diffF 1 f, diffF 2 f, …)` | 分量 i 用轴 `i%3` |
| `nsStep v = v +F grad (div v)` | 逐分量 `v ⊕ grad(div v)` |

**独立验证**：按 Agda 定义逐条转写成 Python（非读我方实现），与 ABI mode 2 的
**四步全状态摘要（6×729 = 4374 trits）完全一致** ⇒ 这是「锚定到形式化」而非「三个实现互相同意」。

## 六、尚未纳入 ABI 的能力（诚实清单）

| 能力 | 现状 |
|---|---|
| ~~NSE 求解器~~ | ✅ 已入 ABI（第七批，句柄式；双模式见 §5.5） |
| ~~孤子 ρ(step) Q16~~ | ✅ 已入 ABI（第五批，统一 Q32 Taylor 整数配方，三语言逐位相同） |
| 孤子 ρ 与实数域 sin² 的关系 | ⚠ 本 ABI 只保证**三语言同一整数配方**；与实数域 `sin²` 的一致性由 oracle 回执 `ba93f0e3` 承载（Q16 误差 ≤1 ulp），**不是**内核证明 |
| 12 律音高 / 纳音 60 序列 | 部分已入（`sov_sunyi_*`），纳音全表待定 |
| ~~Doz 字符串渲染~~ | ✅ 已入 ABI（第四批 `sov_format_doz`/`sov_parse_doz`，含 render→parse 往返不变式） |
| TDA 持续同调 / 手性几何完整面 | 三语言覆盖不一致，未入 |

## 七、往返不变式（可判定的强验证）

| 不变式 | 覆盖 | 证据 |
|---|---|---|
| `parse_doz(format_doz(v)) == v` | 15 个值（含 0、11、12、144、729、177147、2^64−1=18 位 Doz） | 一致性输出 `rt=1` ×15，三语言相同 |
| `tryte_value(tryte_unpack(v)) == v` | **全枚举 v ∈ [0,729)** | `tryte_rt_729=1`，三语言相同 |
| 错误路径 | 缓冲不足 / 小写 / 非法字符 / 空串 / 真溢出 / 解包越界 | 三语言逐项返回码相同 |

## 八、版本状态说明

`SOV_ABI_MAJOR.MINOR = 1.0` 目前为**预发布**（pre-release）：尚无外部消费者，
基线 `docs/abi-baseline-v1.0.txt` 作为**当前工作快照**，符号集仍随开发增长。
**首次对外发布时冻结**：此后新增符号 ⇒ 升 MINOR，删除/改签名 ⇒ 升 MAJOR（`abi_check.sh` 已强制）。

## 九、与 Agda 形式化库的锚定（原理来源）

| ABI 设施 | Agda 依据（`/data/work/discrete-mathematics`） |
|---|---|
| `sov_nse_*` mode 2 | `Application/Problems/NavierStokes/NSEOnT6.agda`: `nsStep`、`div`、`grad`、`diffF`、`shiftAt` |
| `sov_sunyi_*` 损益 | `NSE.agda` §0 公理7（损一 `+1`、益一 `+2`）、`sunyi-ratio 3/2` |
| `sov_zhonglv_step` | `NSE.agda` §0 公理5（`3^11 / 2^16`）、`zhonglv-mult 8` |
| `sov_soliton_*` | `Base/Invariants.agda`（相位锚 4500 / 域值）与 `NSE.agda` §6 λ 推导 |
| `sov_t6_index` / `is_standing_node` | `NSE.agda` §0 公理3（`POLAR=144`、`TOROIDAL=46`、`TOTAL=6624`） |
| `sov_eis_*` | `Foundation/Eisenstein.agda`（0 postulate/0 hole） |
| `sov_ring_*` / `sov_gf9_*` | `Base/Trit.agda`、`Algebra/FieldTower/GF9.agda` |
| `sov_dc_*` | `Algebra/GroupTheory/DuodecClock.agda` + `DCGroup.agda`（`dcZeroOblivion`） |

### 已做「强验证」的设施（逐项记录）

| 设施 | 强验证方式 | 结果 |
|---|---|---|
| `sov_nse_*` mode 2 | 按 Agda 定义逐条转写 Python（不读我方实现），四步全状态 4374 trits 对照 | ✅ 完全一致 |
| `sov_sunyi_length` | 解析 `SunyiChain.agda` 的 `十二律长度` 元组原文 vs ABI 实测 | ✅ 12/12 |
| `sov_sunyi_gain` | 解析 `loss-gain-seq`（Sun/Yi 序列）vs ABI 实测 | ✅ 12/12 |
| `sov_sunyi_remainder` | 解析 `LCM_REMAINDERS` 元组 vs ABI 实测 | ✅ 12/12 |
| 仲吕闭合常数 | `huangzhong-rem`/`zhonglv-rem` 的 refl 级等式 | ✅ 177147 × 65536 = 11609505792 |
| `sov_gf9_mul` | 解析 `GF9.agda`（`α² = −1`、`GF9 = GF3 × GF3`）→ 独立评估 81 项乘法表 | ✅ 81/81 |
| `sov_eis_mul` | 解析 `Eisenstein.agda` 的 `_*ᵉ_` 公式原文并断言规范形 `(a*c-b*d, a*d+b*c-b*d)`；再用该公式独立评估 625 乘积校验和 | ✅ 公式逐字一致；625/625 校验和相同（694701810） |
| `sov_dc_crt` | 逐行解析 `Duodecimal.agda` 的 12 条 `crt12` 子句 + `toℕ₁₂` 表 | ✅ 12/12 |
| `sov_trit_add`/`sov_trit_mul` | 解析 `Base/Trit.agda` 的 `_⊕_`/`_⊗_` 子句（按顺序模拟模式匹配，与 `_ ⊗ _ = T₁` 规范项一致） | ✅ 18/18 |
| `sov_dc_mixedop` | 解析 `DuodecClock.agda` 的 `mulAlpha` 13 条子句 + 复用 ⊕ 表，组合出 144 项 | ✅ 144/144 |
| `sov_zhonglv_closure` / `sov_sovereign_lcm` | 解析 `FixedPointQ16.agda` 的 `zhonglvClosure acc = (acc × CLOSURE_NUM) / CLOSURE_DEN` + `SunyiBase.agda` 的 `CLOSURE_NUM/DEN/SOVEREIGN_LCM` + 两条 refl 定理（`30→81`、`81→218`） | ✅ 定义一致；两条 refl 实例相等；LCM = 11609505792 |
| `sov_zhonglv_step`（8 步） | 用 Agda 的**单次闭合**复合 8 次（每步 mod LCM）与 ABI 的 8 步实现对照 | ✅ 4/4 输入完全一致（含 `30→84981`、`987654321→6523580390`） |
| `sov_dayan_multiplier` | 解析 `DayanState.agda` 的 `dayan-init`/`dayan-step` 原文形态；用 ℤ 版独立实现核对 ABI | ✅ 1101/1101 互素对（m≤60），校验和 823007911 |
| `sov_crt2` | 广义二模 CRT：验证解满足两同余且唯一落在 [0,m₁m₂) | ✅ 576/576（模数对取自 {2,3,5,7,11}），物不知数 → 23 |

核对器：`engineering/verify_agda_anchors.py`（直接读 Agda 源文件，**不读我方表**；已并入 `abi_check.sh` 第 4.5 步）。
当前覆盖 **925 项**：十二律 12 + 损益 12 + 余数 12 + 闭合常数 2 + GF(9) 81 + Eisenstein 625 + CRT 12 + Trit 18 + DC mixedOp 144 + 仲吕闭合 7。

负向控制（已实测三条，判官确实会说「不」）：
- 篡改余数 `124416→124417` → `不一致 1` FAIL
- 篡改 `gf9_3_3` 为 7（α·α 应为 2）→ `不一致 1` FAIL
- 篡改 CRT `(2,2)` 为 9（应为 2）→ `不一致 1` FAIL
- 篡改 `trit22` 为 0（`T₂⊕T₂` 应为 1）→ `不一致 1` FAIL
- 篡改 `mix_10_11` 相位为 2（应为 1）→ `不一致 1` FAIL
- 篡改 `zc30` 为 82（Agda refl 定理为 81）/ `sovereign_lcm` 为 11609505793 → 立即判不一致 FAIL

⚠ 其余设施仍是**定义级对应**（未做逐条转写 + 全状态对照），不得声称已强验证。

## 十、大衍求一术：道与术（以及一处必须说明的分歧）

**道（principle）**：求乘率 `k` 使 `奇 · k ≡ 1 (mod 定)` —— 即 Bezout 系数存在性。
**术（technique）**：秦九韶《数书九章》「置奇右上，定居右下，立天元一于左上……递互除乘，须使右上末后奇一而止」。

形式化依据：`Algebra/GroupTheory/DayanState.agda`（四格状态 + `dayan-step` + `乘率-full`）、
`DayanCorrect.agda`（把 `init-invariant` / `step-invariant` / `terminate-correct` 写成**义务类型**）。

### ⚠ 实测分歧与修复：不是 ℕ 不够用，是术文实现有两处缺陷

`Data.Nat` / `Data.Nat.Properties` / `Data.Nat.DivMod` 对本法完全够用（取模、商余关系、模加乘法律、强归纳都在）。
失败来自实现，而非 `ℕ` 算术的表达力：

| 版本 | 递推 | 乘率取值 | 通过率（m≤60，1101 组） |
|---|---|---|---|
| A. `DayanState.agda` 原文 | `lt' = (q·lt) ∸ lb` | 步数=0 → `lt`；否则 `定 ∸ lt` | 249/1101 = **22.6%** |
| B. 仅改方向（仍 `∸`） | `lt' = lb ∸ (q·lt)` | 同上 | 59/1101 = **5.4%**（更差） |
| **C. 方向 + 系数模 定** | `lt' = (lb + 定 − (q·lt mod 定)) mod 定` | **`lt` 直接** | **1101/1101 = 100%**；m≤200 为 12231/12231 |

- **缺陷 ①**：Bezout 递推应为 `新系数 = 旧B − q × 旧A`，原文写成 `q·lt ∸ lb`（操作数顺序反了）。
- **缺陷 ②**：`∸` 饱和减在 `q·lt > lb` 时抹掉符号；原文再用「步数>0 ⇒ 乘率 = 定 ∸ lt」补偿，只在部分情形成立。
- **修复**：系数直接保持在 `[0, 定)` 的剩余类代表元 —— 减法变模加，**全程 ℕ**，且乘率就是 `lt`（无需后处理）。
- **等价性**：修正形（纯 ℕ）与 ABI 现用的 ℤ 版在 m≤200 的 12231 组上**逐值相同**（0 处不一致）。

完整诊断与修正形（含 Agda 代码草案与证明路线、诚实边界）：见 [`docs/dayan-nat-fix.md`](dayan-nat-fix.md)。
⚠ 三个义务（`init`/`step`/`terminate`）在修正形下**仍未给出 Agda 证明项**，本文只主张数值正确性。

### 经典实例（ABI 实测）

- 物不知数：`x≡2(3), x≡3(5), x≡2(7)` → `crt2` 两步得 **23**（`23 ≡ 2 mod 3, 3 mod 5, 2 mod 7`）✓
- 乘率：`乘率(2,3)=2`、`乘率(1,5)=1`、`乘率(3,5)=2` ✓

## 十一、位宽转换：二进制硬件 ↔ 三进制（这才是 3¹¹/2¹⁶ 的读法）

依据：`docs/duodecimal/22-doz-formalization.md` §位宽对照 + Agda `Synthesis/NumberTheory/DigitField.agda`（纯容量算术）。

**位域容量 = 状态数**：b 个二态域有 2^b 种状态，t 个三态域有 3^t 种。
「b 位二进制可无损表示 t 个三进制位」⇔ **2^b ≥ 3^t**。

| 关系 | 数值 | 结论 |
|---|---|---|
| 3¹⁰ vs 2¹⁶ | 59049 < 65536 | 16 位装得下 **10 trits** |
| 2¹⁶ vs 3¹¹ | 65536 < 177147 | 16 位装**不下** 11 trits |
| 3¹¹ | 177147 | 11 trits 需 **18 位**（2¹⁷=131072 < 177147 ≤ 2¹⁸） |
| 3⁹·2¹⁸ | 5159780352 = 12⁹ | 数位分离的零浪费划分（`capacity-*` refl） |
| 3²⁷ / 2⁴³ | 7625597484987 / 8796093022208 | Agda `capacity-27trit` / `capacity-43bit`（refl） |

ABI 新增四个函数（`sov_width_*`），强验证：解析 `DigitField.agda` 的 6 条 `capacity-* ≡ …` refl 等式与 ABI 对照
（0 不一致），并核对关系 `3¹⁰ < 2¹⁶ < 3¹¹`、`16 位→10 trits`、`11 trits→18 位`。

⚠ 范畴红线（文档 §7.5）：这是**位域容量**层。「12 态位域 = 1 三态 + 2 二态」**不能**读成
「12 进制 = 3 进制 × 2 进制」——后者是记数法层（Doz/Positional），二者之间是**双射桥**（`DigitRealization`）。

### 实现缺陷记录（本轮实测）

`sov_width_bits_for_trits(40)` 初版**死循环**：`3⁴⁰ = 1.2157×10¹⁹ > 2⁶³`，而 `cap` 为 `uint64`，
`cap <<= 1` 在 2⁶³ 后回绕为 0 ⇒ `0 < need` 永真。修复：移位只到 2⁶³，仍不足则返回 **64**；
`trits_for_bits` 侧改用 `p <= cap/3` 判据避免 `p*3` 溢出。修复后全量测试 8 秒完成。

## 十二、三种「12」必须分开说（位域容量 ≠ 展示群周期）

存在三种互不替代的「12」，混用即范畴错误：

| 读法 | 出处（形式化） | 「12 =」的含义 | 分量结构 |
|---|---|---|---|
| **① 位域容量** | `Synthesis/NumberTheory/DigitField.agda`：`capacity-12 : 3 * (2 * 2) ≡ 12`（refl） | 12 个**状态**可划分为 1 个三态域 + 2 个二态域（**基数/双射**事实） | 3·2·2，纯计数，**不含代数结构** |
| **② DC 展示群（联合周期）** | `GroupTheory/DuodecClock.agda`：`joint-period-12 : 3 * 4 ≡ 12`（refl）、`mixedOp-12-cycle : ∀ p → mixedOp^12 p ≡ p` | 联合生成元 g 的**阶恰为 12**（`12 = char(GF(9)) × ord(α)`） | **C₃（加法，特征 3）× C₄（乘法相位，⟨α⟩ ⊂ GF(9)\*)**** |
| **③ 记数法** | `Synthesis/Format/{Doz,Positional}.agda` + 桥 `DigitRealization` | 基 12 的记数法（Doz 一位 ∈ 0..11），与 12 态位域**双射** | 位值权重 12^k |

### 关键：C₄ ≠ V₄ —— 相位不可退化为两个二进制位

「2 个二态域」作为**群**是 C₂×C₂ = **V₄**（每个非平凡元素阶为 2）；
而相位 ⟨α⟩ 是**循环群 C₄**（存在阶恰为 4 的元素）。两者阶同为 4，但**不同构**：
C₃×V₄ ≇ C₃×C₄。所以「12 态位域 = 1 三态 + 2 二态」**只能**作容量读法，
不能读成 DC 的分量分解 —— 这正是「相位不可约」红线（C₄→C₂ 非忠实商）。

Agda 的三元证据：`PhaseSubgroup.alpha-order-exactly-4 = (alpha-4-id, alpha-2-not-id, alpha-not-id)`
—— α⁴ = 1 且 α² ≠ 1 且 α ≠ 1。

### ABI 强验证（本轮新增）

从单位元 e 出发调用 `sov_dc_translate(k)`，实测 g^k 全表：

```
k :  0    1    2    3    4    5    6    7    8    9    10   11   12
g^k: 00   11   22   03   10   21   02   13   20   01   12   23   00
```

- **g^k = (T_{k mod 3}, a_{k mod 4})** 全 13 项吻合 ⇒ 联合周期 12 与其两个分量同时被判定
- `jnt0 = jnt12 = e`，且 `k=1..11` 均 ≠ e ⇒ 阶**恰为** 12（不是 6 或 4）
- `comp_periods=trit3:1,phase4:1` ⇒ trit 分量周期 3、相位分量周期 4
- `ord4_a1=(1,1)`、`ord4_a2=(2,2)`、`ord4_a4=(1,0)` ⇒ α≠e、**α²≠e**、α⁴=e
  ⇒ **C₄ 而非 V₄**（若相位是 V₄，α² 就会等于 e）

核对器断言解析 `joint-period-12`、`mixedOp-12-cycle`、`alpha-order-exactly-4` 三处原文后与上表比对。

## 十三、相位信息的三层：信息 / 结构 / 几何（回应「是否丢失相位」）

| 层 | 是否丢失 | 依据 |
|---|---|---|
| **信息**（12 个元素 + 群律） | **不丢** | CRT 双射：`crt12` 12/12 与 ABI `sov_dc_crt` 一致；`mixedOp` 144/144 与 Agda `mixedOp`（分量 ⊕ / `mulAlpha`）一致 |
| **结构**（哪一分量加法、哪一分量乘法） | **不丢，但 C₁₂ 投影会遮住** | `sov_dc_mixedop` 是**分量式**运算（trit ⊕、相位 `mulAlpha`），不是 mod-12 数值加法；而 `sov_dc_crt`/`translate` 是数值投影（`g^k=(T_{k%3},a_{k%4})`）—— 后者确实抹掉了「乘法 vs 加法」的区别，读 `x ∈ [0,12)` 看不出这一点 |
| **几何**（旋转作为矢量/球谐） | **原先缺失，本轮补上** | 见下 |

### 几何层：C₄ 的 90° 旋转（**GF(3) 标准三进制 {0,1,2}**，不用平衡三进制）

`R = [[0,2],[1,0]]`（2 ≡ −1 mod 3），作用 `(x,y) ↦ (2y mod 3, x mod 3)`：

| k | 相位矢量 (trit) | 含义 |
|---|---|---|
| 0 | (1, 0) | 0° |
| 1 | (0, 1) | 90° |
| 2 | (2, 0) | 180°（= 2I，即「−I」的 GF(3) 写法） |
| 3 | (0, 2) | 270° |

新符号 `sov_dc_phase_vec` / `sov_dc_rotate_vec`（trit 输入输出，越界 → `SOV_ERR_TRIT`）。实测：

- **群同态**：`R^k ∘ R^j = R^(k+j mod 4)`，4×4×3 组输入**全枚举 0 反例**
- `R² = 2I`（(1,0) → (2,0)）⇒ **C₄ 而非 V₄**（V₄ 每元素平方为 I）
- `R⁴ = I`（(2,1) 连转四次复原）
- 相位矢量 = `R^k(1,0)`（0 反例）；非法 trit（如 3）→ `SOV_ERR_TRIT`

> ⚠ 修正记录：初版用 ℤ 与 {−1,0,1}（平衡三进制）表示矩阵元，**违反宪法**（基底是标准三进制 GF(3)）。
> 现已全面改为 GF(3)：ABI 与 Agda 形式化均使用 `Trit` / `{0,1,2}`。

### ✅ 形式化桥已补齐：`RotBridge`（Agda，0 postulate / 0 hole，内核 exit 0）

原缺口（Agda 只有 `PhaseSubgroup.rot = 乘 α` 的纯群作用，没有矩阵实现）已补上。
新增模块 `/data/work/discrete-mathematics/bridge-from-sov-math/RotBridge.agda`：

| 内容 | 形式 |
|---|---|
| 2×2 **GF(3)** 矩阵 + 乘法/单位/取负 | `record Mat2 { … : Trit }`、`mulM`（用 `⊕`/`⊗`）、`idM`、`negIM` |
| 90° 旋转 | `R = mat T₀ T₂ T₁ T₀`（2 ≡ −1） |
| 相位映射 | `Rot a0 = idM`、`Rot a1 = R`、`Rot a2 = negIM`、`Rot a3 = negRM` |
| **群同态** | `Rot-hom : ∀ a b → Rot (mulAlpha a b) ≡ mulM (Rot a) (Rot b)` — **16/16 refl** |
| **单射** | `Rot-injective`（经 `decode` 左逆：`decode-Rot : ∀ a → decode (Rot a) ≡ just (toFin a)`，4 个 refl） |
| **阶恰为 4** | `R²≡2I`、`R⁴≡I` 均 `refl` |

⇒ 「⟨α⟩ ≅ ⟨R⟩」现在是**内核判定的事实**，而不再只是 ABI 侧的矩阵恒等检查。
核对器 `verify_agda_anchors.py` 会检查该模块存在、关键定理为 refl、且 ABI 几何层取值与 `Rot` 一致。

### **为什么不证「3D 球谐连续实现等价」**（本节是立场声明，不是躲闪）

1. **宪法级**：全库 0 处 `Float`/`Real`/`Complex`；基底是 GF(3)/GF(9) 有限域。
   `Y_l^m(θ,φ)` 需要 ℝ、S² 上的测度、L² 内积、sin/cos 与连带 Legendre 多项式 —— 全是连续统对象。
2. **命题本身不成立为「定理」**：ℤ² 上的有限 C₄ 作用与 L²(S²) 上的无限维表示之间不存在可判定的同构陈述；
   要谈「等价」必须先固定拓扑、测度、内积，并构造**等变等距**——那是连续统内部的命题。
3. **能证的部分已在 `RotBridge §6` 给出**（ℓ=1 三重态骨架，全程 GF(3)）：
   `rot3 = [[0,2,0],[1,0,0],[0,0,1]]`；`m=0` 轴固定（`refl`）、`z=0` 子空间退化为 2D 的 `R`（4 步 `refl`）、
   `rot3² = diag(2,2,1)`、`rot3⁴ = I`（均 `refl`）。
4. **结论**：不是「没证」，而是**在本基座内不可表达**。把连续实现当等价对象会引入连续统污染；
   正确形态是把连续实现作为**投影对象**研究（三重完备性标尺给出 0/0/0 诊断），而不是去证等号。

## 十五、离散球体：库内**已有**形式化（本轮核对确认）

先前的 `RotBridge §6` 是我"从零搭"的 ℓ=1 骨架；核对后发现**更完整的离散球体形式化早已存在**，
已接入核对器（`check_discrete_sphere`）：

| 对象 | 形式化位置 | 内容 |
|---|---|---|
| **S² 12 胞腔**（正十二面体） | `Synthesis/Geometry/ChernEulerLadder.agda` | `s2-12cell-vertices = 20`、`edges = 30`、`faces = 12`；`s2-12cell-euler : 20−30+12 ≡ 2`（refl） |
| **c₁ = χ** | 同上 | `chern-equals-euler-2d : ChernNumber ≡ 2`（refl）——与 ABI 陈数 `C = ±2`（432/216）一致 |
| **T⁶ 的 χ = 0** | 同上 | `t6-euler : 1−6+15−20+15−6+1 ≡ 0`（refl） |
| **T⁶ → S² 的离散 Hopf** | `Synthesis/Homotopy/HopfConstruction.agda` | 全空间 `GF(3)⁶`（729 点）→ 底空间 `T⁶╱A₄` → 纤维 = A₄ 轨道 |
| **A₄ 无 4 阶元 / 2A₄ 有** | `Synthesis/A4/BinaryTetrahedral.agda`、`…DefiningRep.agda` | `q8-i⁴ : qi⁴ ≡ q1`；`orderOf g3 = 4` |

### 关键结构洞察：相位 C₄ ∉ A₄，而 ∈ 2A₄ = SL(2,3)

- 12 胞腔球面的对称群是 **A₄**（阶 12），A₄ 的元素阶只能是 1/2/3 —— **装不下 4 阶相位**；
- 相位矩阵 `R = [[0,2],[1,0]]` 的 `det = 1`（`R-det-is-1 = refl`）⇒ **R ∈ SL(2,3) = 2A₄**（双重覆盖）；
- 所以 C₄ 作用在 ℓ=1 的**双双态（spinor 覆盖）**上，而不是置换 12 个胞腔。
  这解决了「A₄ 有 12 个元素、12 胞腔也有 12 个，为何相位不是 A₄ 的子群」的表面矛盾。

### 诚实边界

- 库内形式化的是**胞腔计数与拓扑不变量**（χ、陈数、Hopf 投影的集合论层面），**不是** S² 的度量/球谐解析式；
- 我们的 `RotBridge` 给的是 A₄ 的 2-覆盖上的 4 阶作用，**不是** S² 上的连续旋转群作用；
- ~~把 HopfConstruction 的 A₄ 作用提升到 2A₄~~ → **已做**，见下节。

## 十六、A₄ → 2A₄ 的提升：相位与 Hopf 投影等变对齐（已完成）

新增模块 `bridge-from-sov-math/HopfSpinLift.agda`（**cubical，0 postulate / 0 hole，agda exit 0**）。

### 构造

```
SpinTotal = TotalSpace × GF(3)²                      -- 自旋全空间（T⁶ 格点 × 2 维表示空间）
phaseAct g (x , v) = (a4Action g x , ρ(R) · v)       -- 底上走 A₄，自旋上走 2A₄ 的 2 维表示
spinProj (x , v)   = [ x ]                            -- 投影到 A₄ 轨道（同 HopfConstruction.totalProj）
```
其中 `R = [[0,2],[1,0]]`（GF(3)，4 阶）、`g = π(R)` 是 A₄ 的 2 阶元（双重覆盖的投影）。

### 四条断言（均已证）

| 断言 | 内容 |
|---|---|
| `phaseAct⁴≡id` | 四次回位：底上 `g²` 用两次、自旋上 `R⁴ = I`（9 个 trit 组合逐项 refl） |
| `phaseAct²-nontrivial` | 平方非恒等：`R² = 2I`（自旋翻转），正是双重覆盖的非平凡性 |
| `phase-equivariant` | 相位作用**保持 A₄ 轨道** ⇒ 投影到底空间不变（复用 `H.action-in-orbit`） |
| `detR≡1` | `ρ(R) ∈ SL(2,3) = 2A₄` |

### 结论（结构语义）

相位 C₄ **不置换 12 个胞腔**，而是作用在**自旋纤维**上；投影后到底空间是**不变的**（同一 A₄ 轨道）。
这正是「A₄ 装不下 4 阶相位、而 2A₄ 装得下」的精确表述 —— 相位是 spin 自由度，不是胞腔置换。

### 诚实边界

- 模块以「`g` 是 A₄ 的 2 阶元（`g² = id`）」为**参数假设**（A₄ 的三个双对换满足；存在性显然），
  具体实例化（构造该 `g` 并证其阶为 2）留给 `Synthesis/A4/A4Group` 侧；
- 证的是**离散/GF(3) 层面的等变**，不是 S² 上连续旋转群的等变。
