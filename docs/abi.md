# sov-ffi C ABI 契约 (v1.0)

> **这是我们对外的唯一承诺面**：`dist/include/sov/` 的头 + 动态库导出的符号。
> 三语言实现（cpp / rust / hs）**逐符号等价**，可由任意 C/C++/带 FFI 的宿主加载。
> 单一事实源：`cpp/include/sov/*.h`（版本脚本 `cpp/packaging/sov_ffi.map` 由此自动生成）。

## 一、符号清单（60 个，分 21 个设施头）

| 头 | 符号数 | 符号 |
|---|---|---|
| `<sov/version.h>` | 4 | `sov_abi_version`, `sov_strerror`, `sov_runtime_init`, `sov_runtime_fini` |
| `<sov/trit.h>` | 4 | `sov_trit_add`, `sov_trit_mul`, `sov_trit_sun`, `sov_trit_c3` |
| `<sov/gf9.h>` | 2 | `sov_gf9_mul`, `sov_gf9_norm` |
| `<sov/ring.h>` | 4 | `sov_ring_add`, `sov_ring_mul`, `sov_ring_double`, `sov_ring_to_tryte` |
| `<sov/dc.h>` | 4 | `sov_dc_mixedop`, `sov_dc_sigma`, `sov_dc_crt`, `sov_dc_translate` |
| `<sov/adc.h>` | 1 | `sov_adc64` |
| `<sov/bridge.h>` | 1 | `sov_lcm_step` |
| `<sov/l6.h>` | 1 | `sov_zhonglv_step` |
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
| 实现一致 | `engineering/ffi_conformance.sh` | 三语言同一向量 **949 行逐行相同** |
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

核对器：`engineering/verify_agda_anchors.py`（直接读 Agda 源文件，**不读我方表**；已并入 `abi_check.sh` 第 4.5 步）。
当前覆盖 **754 项**：十二律 12 + 损益 12 + 余数 12 + 闭合常数 2 + GF(9) 81 + Eisenstein 625 + CRT 12。

负向控制（已实测三条，判官确实会说「不」）：
- 篡改余数 `124416→124417` → `不一致 1` FAIL
- 篡改 `gf9_3_3` 为 7（α·α 应为 2）→ `不一致 1` FAIL
- 篡改 CRT `(2,2)` 为 9（应为 2）→ `不一致 1` FAIL

⚠ 其余设施仍是**定义级对应**（未做逐条转写 + 全状态对照），不得声称已强验证。
