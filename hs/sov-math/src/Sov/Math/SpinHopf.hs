-- | Sov.Math.SpinHopf — T⁶ × GF(3)² 上的相位作用（α 阶 4）；"旋量丛"是类比
--
-- ⚠ P2：底侧作用不自由（27 固定点）⇒ 不是主丛/丛，只是带 GF(3)² 因子的分层商。
--   本模块的实质只有两条：α 阶 4、α² = −I，以及 W = (σ₁σ₂)³ 与 α² 在 GF(3)² 上**同算子**。
--
-- 与 Agda 对齐：`Sovereign.Synthesis.Homotopy.HopfSpinLift`
--   （`SpinTotal = TotalSpace × GF(3)²`、`phaseAct`、`phaseAct⁴≡id`、`phase-equivariant`）
--
-- 结构：
--
--   总空间   SpinTotal = T⁶ × GF(3)²      —— 729 × 9 = **6561**
--   底空间   T⁶/A₄                        —— 135 类（`Sov.Math.Hopf.projClass`）
--   相位     α = [[0,2],[1,0]] ∈ SL(2,3)  —— **阶 4**（90°），α² = −I
--   作用     (g,k) : (x,v) ↦ (actOn g x, αᵏ v)   —— 两因子独立 ⇒ 自动等价
--
-- 为什么相位必须住在这里（而不是 A₄）：
--   A₄ 只有 1/2/3 阶元，**没有 4 阶元**（`SL23` 的测试 `sl23_a4_has_no_order4` 验过）。
--   90° 相位 α 只能在双层覆盖 2A₄ = SL(2,F₃) 里存在 ⇒ 旋量丛是承载 C₄ 相位的唯一位置。
--
-- 两条与辫群互锁的事实（本模块把它变成可跑项）：
--   · α² = −I                                  —— 半圈是旋量符号翻转
--   · W = (σ₁σ₂)³ = −I                         —— 辫的中央元在纤维上**同样**取负
--   即：编织一整圈（三次半扭）与"相位半圈"在旋量纤维上是同一个算子。
--
-- 零浮点、零 partial。
module Sov.Math.SpinHopf
  ( SpinFiber, SpinTotal
  , spinFiberCard, spinTotalCard
  , alphaMat, alphaPow
  , phaseAct, spinAct, spinProj
  , spinOrbitReps, phaseOrbits
  ) where

import Data.List (nub)

import Sov.Math.A4 (A4)
import Sov.Math.Hopf (T6Point, actOn, orbitReps, projClass)
import Sov.Math.SL23
import Sov.Math.Trit

type SpinFiber = (Trit, Trit)
type SpinTotal = (T6Point, SpinFiber)

spinFiberCard :: Int
spinFiberCard = 9          -- |GF(3)²|

spinTotalCard :: Int
spinTotalCard = 729 * 9    -- 6561

-- | α = [[0,2],[1,0]]：GF(3)² 上 90° 旋转，阶 4（det = 0·0 − 2·1 = −2 ≡ 1）
alphaMat :: M2
alphaMat = M2 T0 T2 T1 T0

alphaPow :: Int -> M2
alphaPow k = matPow k alphaMat

-- | 纤维上的相位作用：αᵏ
phaseAct :: Int -> SpinFiber -> SpinFiber
phaseAct k = actVec (alphaPow k)

-- | 全空间作用：A₄ 作用于 T⁶ 因子，相位作用于 GF(3)² 因子
spinAct :: (A4, Int) -> SpinTotal -> SpinTotal
spinAct (g, k) (x, v) = (actOn g x, phaseAct k v)

-- | 投影到 Hopf 底空间（只看 T⁶ 因子）
spinProj :: SpinTotal -> Int
spinProj = projClass . fst

-- | 一条纤维轨道上的代表（⟨α⟩ 作用下的轨道），弦：8 非零向量分成 2 条各 4 元
phaseOrbits :: [[SpinFiber]]
phaseOrbits = go nonzeroVectors
  where
    go []       = []
    go (v : vs) =
      let orb = nub [ phaseAct k v | k <- [0 .. 3] ]
      in orb : go (filter (`notElem` orb) vs)

-- | 每个底类取一个代表（复用 `Hopf.orbitReps`，不另写一份）
spinOrbitReps :: [T6Point]
spinOrbitReps = orbitReps
