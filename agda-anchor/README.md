# agda-anchor — 跨语言实现的 Agda 规范锚 (等级 3 等价)

## 链路

```
discrete-mathematics 规范层 (0 postulate / 0 hole)
  Foundation/Eisenstein (971 行) · FieldTower/GF9 (1104) · GroupTheory/Duodecimal (724)
  GroupTheory/DuodecClock (886) · GroupTheory/DCGroup (193) · Base/Trit (282)
        │  本目录 SpecAnchor.agda 逐值 refl 判定 (内核回执)
        ▼
SpecAnchor.agda  ── crosscheck.py ──▶  rust/hs/cpp dump_tables 输出
        │                                    │
        └──── 值完全一致 ⇒ 实现锚定到内核 ────┘
```

## 覆盖

| 锚 | 项数 | 内容 |
|---|---|---|
| GF(3) 加法表 | 9 | `add-00 … add-22` 全表 refl |
| GF(3) 乘法表 | 9 | `mul-00 … mul-22`（含宪法 `T₂⊗T₂=T₁`） |
| char-3 归零 | 3 | `(t⊕t)⊕t ≡ T₀` 三分量 |
| C4 相位 | 3 | `α⁴=a₀`、`α²`、`α³` |
| 零冥族/单位元 | 14 | `mixedOp x e ≡ x` 12 点 + 生成元 + 损益抵消 |
| CRT 12 点 | 12 | `toℕ₁₂ (crt12 Tᵢ αᵏ)` 全表 + 4 个实现锚点 |
| Eisenstein | 4 | `ω³≡1`、`ω²=ω·ω`、`ω·ω²≡1`、`\|ω\|=1` |

## 复跑

```bash
agda --guardedness -i $AGDA_LIB/src -i $STDLIB/src -i agda-anchor agda-anchor/SpecAnchor.agda
python3 agda-anchor/crosscheck.py <dump.csv>
```

内核回执: `ace0e46a2e1854a56ac11f2468238964dacc63f37bdab5405ee78df811f52462`（源码一改即失效）。

## 诚实边界

- 锚定的是**投影层数值**（Z/12 是 DC 的投影）；DC 本源事实（`mixedOp-12-cycle`、`dcZeroOblivion`）
  由库内既有证明承载，本模块**引用**而非重证。
- `DCMIX` 144 项的等价依据是 mixedOp 定义逐字同构（`(x⊕y, mulAlpha a b)`），未逐项做 Agda refl。
- 超大论域（Ring 3¹¹、u384）仍由 dump 对账 + 结构化覆盖承载，非内核证明。
