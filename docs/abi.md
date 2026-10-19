# sov-ffi C ABI 契约 (v1.0)

> **这是我们对外的唯一承诺面**：`dist/include/sov/` 的头 + 动态库导出的符号。
> 三语言实现（cpp / rust / hs）**逐符号等价**，可由任意 C/C++/带 FFI 的宿主加载。
> 单一事实源：`cpp/include/sov/*.h`（版本脚本 `cpp/packaging/sov_ffi.map` 由此自动生成）。

## 一、符号清单（45 个，分 18 个设施头）

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
| `<sov/t6.h>` | 2 | `sov_t6_index`, `sov_is_standing_node` |
| `<sov/wide.h>` | 3 | `sov_wide_rho`, `sov_wide_const_pi`, `sov_wide_const_sqrt3` |
| `<sov/chern.h>` | 4 | `sov_chern_inject`, `sov_chern_detect`, `sov_chern_inject_monopole`, `sov_chern_compute` |
| `<sov/sunyi.h>` | 4 | `sov_sunyi_length`, `sov_sunyi_gain`, `sov_sunyi_boundary`, `sov_chiral_collapse` |
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
| 实现一致 | `engineering/ffi_conformance.sh` | 三语言同一向量 **853 行逐行相同** |
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

## 六、尚未纳入 ABI 的能力（诚实清单）

| 能力 | 现状 |
|---|---|
| NSE 求解器（6624 格点演化） | cpp `nse_solver.h` / rust `NSESolver` / hs `Guard/NSE.hs` 均有，但**跨语言状态量太大**，需设计句柄或摘要式 ABI |
| 孤子 ρ(step) Q16 | 三语言实现方式不同（hs Q32 正弦 / rust f64 / cpp Q16 近似），需先统一为**同一整数配方**再入 ABI |
| 12 律音高 / 纳音 60 序列 | 部分已入（`sov_sunyi_*`），纳音全表待定 |
| ~~Doz 字符串渲染~~ | ✅ 已入 ABI（第四批 `sov_format_doz`/`sov_parse_doz`，含 render→parse 往返不变式） |
| TDA 持续同调 / 手性几何完整面 | 三语言覆盖不一致，未入 |

## 七、往返不变式（可判定的强验证）

| 不变式 | 覆盖 | 证据 |
|---|---|---|
| `parse_doz(format_doz(v)) == v` | 15 个值（含 0、11、12、144、729、177147、2^64−1=18 位 Doz） | 一致性输出 `rt=1` ×15，三语言相同 |
| `tryte_value(tryte_unpack(v)) == v` | **全枚举 v ∈ [0,729)** | `tryte_rt_729=1`，三语言相同 |
| 错误路径 | 缓冲不足 / 小写 / 非法字符 / 空串 / 真溢出 / 解包越界 | 三语言逐项返回码相同 |
