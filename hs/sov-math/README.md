# sov-math-hs — Sovereign 律算核心 (Haskell 强类型移植)

镜像 `rust/sov-math/sov-core`（对应 C++ `include/`），GHC 9.14 / cabal，零第三方依赖，零浮点。

## 强类型化对照表（相对 Rust 侧的类型升级）

| Rust 侧 | Haskell 侧 | 消灭的运行期错误 |
|---|---|---|
| `enum Trit` + `from_u8` panic | `data Trit = T0 \| T1 \| T2` | 非法 trit 值在类型层不可表达 |
| `RingElement { trits: [u8; 11] }` | `newtype RingElement = RingElement (Vec N11 Trit)`（DataKinds 定长向量） | 位数错误无法编译 |
| `TryteValue { value: u16 }` + `TryteOverflow` | `Tryte = Vec N6 Trit`（值域由构造保证） | 溢出检查整体消失 |
| `pack_5(t0..t4: u8)` | `pack5 :: Trit -> ... -> Trit -> Word8`（恰 5 个 Trit） | arity/值域错误无法编译 |
| `TONE_NAMES: [&str; 12]` 字符串表 | `data ToneName = HuangZhong \| ... \| ZhongLv`（12 构造子枚举） | 第 13 律 / 越界索引不存在 |
| 裸 `i32` Q16 | `newtype Q16`（与裸 Int32 类型隔离） | 定点数与普通整数混用 |
| `MathError` + panic! | GADT `MathError` + `Either` / `constitutionalPanic` | 错误携带证据（当前 Q16、期望/实际维度） |

## 模块地图

```
src/Sov/Math/
  Nat.hs           类型级 Peano 自然数 + singleton（SNat）
  Vec.hs           定长向量 Vec n a（长度在类型上）
  Constants.hs     宪法常数（黄钟 3^11、仲吕 2^16、LCM、陈数 ±2、Δ_Q16 三值并存）
  Trit.hs          L1 GF(3)：C3 旋转 / 乘法 / 范数 / 损益（互逆）
  Layer1.hs        5-trit 打包 + 批量加法/点积/范数
  L2Ops.hs         L2 矩阵乘/√3门控/门控乘/RMSNorm(LUT 注入) + A4 翻转
  Block.hs         SOV v2.6 范畴标签/主权块 128/全息头
  Guard/Chern.hs   S² 12 胞腔 C=2 陈数构造器 (Z[ω] 精确, 与 sov-guard 跨语言对齐)
  Guard/L7L8.hs    L7 陈数守卫 (Verdict 枚举) + L8 全息 Q32 标度 + 数字根
  Topology/*.hs    T6/仲吕/孤子(Q32 Taylor)/时间晶体/N14/指数塔 (全部零浮点)
  Validation/*.hs  Z[ω,i]/杨辉 Lucas/2·A₄ 特征标/GF(9)·GF(4) 幻方/电磁学 15 节
  DuodecClock.hs   DC 联合时钟定义层 (零冥族五字段/σ_DC/联合生成元周期 12, 全枚举)
  Guard/NSE.hs     T⁶ 网格/NSE 演化/宪法审计/SOV v2.6 序列化 (真空常数 (尾数Q16,指数) 表示)
  Tryte.hs         6 位基 3 数（纳音 mod 60、十二律 mod 12 投影）
  Ring.hs          L2 Z/3^11Z 环（进位加法、本征进位乘法、T₁^11 = 0）
  Adc.hs           L0 uint384 ADC 进位链 + LCM 累加器（纯函数式状态）
  Bridge.hs        LCM 桥状态机（陈数守卫 ±655 才放行逆向桥）
  SunYi.hs         十二律损益链（仲吕不能自生黄钟）
  FixedComplex.hs  Q16.16 定点复数（ω = −1/2 + i√3/2，投影层不进状态机）
  Eis.hs           Eisenstein 整数 Z[ω]（与 Agda Sovereign.RootMath.Eisenstein 对齐）
  Error.hs         两层错误体系
test/Sov/Tests/    147 项测试，逐条镜像 rust #[cfg(test)]
```

## 构建与测试

```bash
cd hs/sov-math
cabal build all
cabal run sov-math-test   # ✅ 全部通过: 147/147
cabal run sov-validation  # 8 个验证组全绿 (镜像 rust main.rs)
```

## 等价性分级 (诚实声明)

| 级别 | 模块 | 证据 |
|---|---|---|
| **全域机器对账** | GF(3)/GF(9)/Ring 加法与乘生成元 | oracle 回执 `5e59305a`: 三路 (python 基线/rust dump/hs dump) 3125863 点逐点一致, 生成元论域全枚举 |
| **精确等价 + 完整论域测试** | Eis/Z[ω,i]/GF(4) 域表/损益链/陈数构造 | 域有限且小, 测试即全枚举 |
| **结构化对账** | Ring 完整乘法 (3.1e10 对超论域) | 生成元全域 1948617 + 双倍全域 177147 + 种子抽样 1e6 |
| **近似投影 (approx 命名显式)** | `solitonRhoQ16Approx` (1 ulp) / `computeHolographicApprox` / `currentDensityLog10Q24Approx` | sin²/log₁₀ 是超越对象, 离散基座只有定点逼近 — 两语言等价仅指逼近算法语义一致, 非值相等 |
| **未对账** | u384 (2³⁸⁴ 不可穷举) | 仅结构测试 (加法/乘标量/移位锚点) |

## DC 定义层三语言同步 (DuodecClock/DCGroup 镜像)

| 层 | hs | rust | cpp |
|---|---|---|---|
| DuodecPoint/mixedOp/零冥族五字段/σ_DC/生成元阶 12 | DuodecClock.hs ✅ | dc_clock.rs ✅ (7 测试) | dc_clock.h ✅ (14 检查) |
| 结合律 1728 case / 同态 144 case / CRT 往返 | 全枚举 ✅ | 全枚举 ✅ | 全枚举 ✅ |
| CRT 锚点表 (e→0, g→1, (T₀,α)→9, (T₁,a₀)→4) | ✅ | ✅ | ✅ |
| Doz 位形 A/B | ✅ | ✅ | ✅ |

## 实测性能/能耗 (rho 450k ops, Xeon E5-2686 v4, RAPL 实测)

| 语言 | 墙钟 | 单次延迟 | 包能耗(净, 扣 6.5W 空载) | 能效 |
|---|---|---|---|---|
| rust | 6.53 s | 102 µs/op | 102.4 J | **228 µJ/op** |
| cpp | 7.77 s | 119 µs/op | 118.9 J | 264 µJ/op |
| hs | 10.15 s | 167 µs/op | 168.0 J | 371 µJ/op |

harness: engineering/bench_energy.py (RAPL 实测 + 空载基线扣除, 3 轮中位数)

## 跨语言对齐说明

- **陈数精确性已三语言对齐**: 注入 C=2 单极 → Haskell `computeChernExact = (−432, 216)`
  与 rust `test_chern_exact` 4 项测试逐条一致 (C = −2 精确, 浮点版只有 −1.9999999999999998)。

- `mulR` 走位权整数（`Σ dᵢ·3ⁱ` 乘积 mod 3¹¹ 后按基 3 重编码），与 rust 逐位进位
  传播算法在同一规范代表元上汇合——`mul_coeff_carry`（2×2=4 → 1+3）与
  `t1_pow11_zero` 是交叉验证锚点。
- `accStep` 镜像 rust 仅对 limb[0] 取模的语义（三语言可交叉验证优先于局部"修正"）。
- `vavx3`（SIMD 硬件抽象层）不移植——Haskell 侧为纯参考后端。

## 宪法红线

- 全库无 `Float`/`Double`（`-Wall -Werror` 下引入即编译失败可审计）。
- `{0,1,2}` 编码：`Trit` 只有 3 个构造子，`{-1,0,1}` 不可表达。
- 跨层操作必须走 `Sov.Math.Bridge`（陈数锁定才允许逆向桥）。
