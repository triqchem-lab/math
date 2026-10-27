-- | Sov.Math.Spin2 — 2 维旋量判据：ρ(g0) 与 ρ(g9) **没有公共特征向量**。
--
-- 镜像 FML `Sovereign.Synthesis.A4.SpinNoInvariantLine`（2026-10-27 完成的
-- 不可约性内部判据）：设 v 是 ρ(g0) 与 ρ(g9) 的公共特征向量，则消元给出
-- 2·x = 0 与 2·y = 0；Z[ω] 无挠 ⇒ v = 0 ⇒ 2 维表示不可约。
--
-- hs 侧的等价裁定（零浮点、全枚举不必要的部分用行列式）：
--   C = AB − BA = [[1, ω²−1],[1−ω, 0]]，det C = 2 ≠ 0
--   ⇒ C 可逆 ⇒ A(Bv) = B(Av) 只有 v = 0 ⇒ 公共特征向量不存在。
-- （行列式 = 2 的手算链：C₁₁ = ω²−1 = −2−ω，C₁₀ = 1−ω，
--   (−2−ω)(1−ω) = −3 ⇒ det = −1 − (−3) = 2。）
--
-- 与 Agda 对齐表：
--   spinorA          ↔ toME g0  = m2e c0 cw c1w c0   = [[0, ω],[1+ω, 0]]
--   spinorB          ↔ toME g9  = m2e cw2 cw c0 cw   = [[ω², ω],[0, ω]]
--   abMat / baMat    ↔ AB-mat / BA-mat（已证闭乘积）
--   abvFst / abvSnd  ↔ ABv-fst / ABv-snd
--   bavFst / bavSnd  ↔ BAv-fst / BAv-snd
--
-- 零浮点、零 partial。
module Sov.Math.Spin2
  ( spinorA, spinorB
  , abMat, baMat, commutator2, detCommutator2
  , abvFst, abvSnd, bavFst, bavSnd
  , noCommonEigenvector2, spin2Ok
  ) where

import Sov.Math.Eis
import Sov.Math.Complex2A4 (M2E (..), mulM, detM)

-- ρ(g0) = [[0, ω],[1+ω, 0]]
spinorA :: M2E
spinorA = M2E eisZero eisOmega (addE eisOne eisOmega) eisZero

-- ρ(g9) = [[ω², ω],[0, ω]]
spinorB :: M2E
spinorB = M2E eisOmega2 eisOmega eisZero eisOmega

subM2 :: M2E -> M2E -> M2E
subM2 (M2E a b c d) (M2E e f g h) =
  M2E (addE a (negE e)) (addE b (negE f)) (addE c (negE g)) (addE d (negE h))

-- 已证闭乘积：AB = [[0, ω²],[−ω, −1]]，BA = [[−1, 1],[−1, 0]]
abMat, baMat :: M2E
abMat = mulM spinorA spinorB
baMat = mulM spinorB spinorA

-- 交换子 C = AB − BA = [[1, ω²−1],[1−ω, 0]]
commutator2 :: M2E
commutator2 = subM2 abMat baMat

-- det C = 2 ≠ 0 ⇒ C 可逆 ⇒ 公共特征向量只能是零向量
detCommutator2 :: Eis
detCommutator2 = detM commutator2

-- 四个分量化简（对 v = (x, y)）：
--   A(Bv) = (ω²·y, (−ω)·x ⊕ (−1)·y)；B(Av) = (y ⊕ (−1)·x, (−1)·x)
abvFst, abvSnd, bavFst, bavSnd :: Eis -> Eis -> Eis
abvFst x y = addE (mulE eisZero x) (mulE eisOmega2 y)
abvSnd x y = addE (mulE eisMOmega x) (mulE eisMOne y)
bavFst x y = addE y (mulE eisMOne x)
bavSnd x _ = mulE eisMOne x

-- 判据本体：det C = 2 且 C 的四格与文档一致
noCommonEigenvector2 :: Bool
noCommonEigenvector2 =
  detCommutator2 == Eis 2 0
  && commutator2 == M2E (Eis 1 0) (Eis (negate 2) (negate 1))
                        (Eis 1 (negate 1)) (Eis (negate 1) 0)

spin2Ok :: Bool
spin2Ok = noCommonEigenvector2
  && abMat == M2E eisZero eisOmega2 eisMOmega eisMOne
  && baMat == M2E eisMOne eisOne eisMOne eisZero
  && all (\(x, y) -> abvFst x y == mulE eisOmega2 y
                  && bavSnd x y == mulE eisMOne x)
         [(eisOne, eisZero), (eisZero, eisOmega), (eisMOmega, eisOmega2)]
