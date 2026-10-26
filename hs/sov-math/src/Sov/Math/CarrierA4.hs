-- | Sov.Math.CarrierA4 — 内核载体：C[A₄]（A₄ 上的函数空间，12 维）+ Plancherel 内积
--
-- 定位（2026-10-26 人类裁定）：载体是**函数空间**，不是点集、不是点积几何。
--
--   载体   Carrier = Vec N12 Eis        —— A₄ 上取值于 Z[ω] 的函数（12 维自由模）
--   内积   dotC f g = Σ_x f x · conj(g x)   —— Plancherel 配对（**未归一**，= 12·⟨f,g⟩）
--   分解   C[A₄] = ⊕_ρ ρ^{⊕ d_ρ}，Σ_ρ d_ρ² = |A₄| = 12
--          **结构证明在 `Sov.Math.RepA4.colOrthAtId`**（列正交在单位元处），
--          本模块只提供数值 `plancherelSum` 供内核判据使用。
--
-- 为什么不用 Z/12：
--   Z/12 交换 ⇒ 只有 1 维不可约表示 ⇒ **矢量表示不存在**（"矢量"退化成一堆标量）。
--   C[A₄] 的正则表示含 1, 1′, 1″, 3 四者；其中 3 维 R3 是 A₄ 唯一的忠实不可约
--   （三个 1 维表示都经 A₄/V₄ ≅ C₃ 分解，非忠实 —— 见 `RepA4.oneDimRepsNonFaithful`）。
--
-- 归一化约定：12 在 Z[ω] 中不可逆，故只给未归一配对；归一化定理一律写成
--   `12 · ⟨f,g⟩` 的整数形式（与 Agda `PlancherelTheorem` 同一约定）。
module Sov.Math.CarrierA4
  ( Carrier, dimCarrier
  , zeroC, basisVec, dotC, normC2
  , Irrep(..), allIrreps, dimI, plancherelSum
  , regAct
  ) where

import Sov.Math.A4
import Sov.Math.Eis
import Sov.Math.Nat
import Sov.Math.RepA4 (Irrep(..), allIrreps, dimI)
import Sov.Math.Vec

-- | A₄ 上的函数空间：12 维自由 Z[ω]-模（= C[A₄]）
type Carrier = Vec N12 Eis

dimCarrier :: Int
dimCarrier = 12

zeroC :: Carrier
zeroC = replicateV n12 eisZero

-- | δ_g：在 a4Index g 处为 1，其余 0 —— L² 的基（正交归一，见测试）
basisVec :: A4 -> Carrier
basisVec g = generateV n12 (\i -> if i == a4Index g then eisOne else eisZero)

-- | 左正则作用：(ρ_g f)(x) = f(g⁻¹·x) —— 让 C[A₄] 真正是一个表示，而不只是向量空间
regAct :: A4 -> Carrier -> Carrier
regAct g f = generateV n12 step
  where
    step i = case drop i allA4 of
               (x : _) -> maybe eisZero id (indexV (a4Index (mulA4 (invA4 g) x)) f)
               []      -> eisZero

-- | Plancherel 内积（未归一）：∑_x f(x) · conj(g(x))
dotC :: Carrier -> Carrier -> Eis
dotC f g = foldrV addE eisZero (zipWithV mulE f (mapV conjE g))

-- | 范数平方（未归一）：dotC f f
normC2 :: Carrier -> Eis
normC2 f = dotC f f

-- | Plancherel 恒等式的**数值**：Σ_ρ d_ρ² = 12 = dimCarrier。
--
-- 结构证明（为什么必须是 12）在 `Sov.Math.RepA4.colOrthAtId`：
-- 列正交在单位元处给 Σ_ρ d_ρ·χ_ρ(e) = |A₄|，而 χ_ρ(e) = d_ρ。
plancherelSum :: Int
plancherelSum = sum [ dimI r * dimI r | r <- allIrreps ]
