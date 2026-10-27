-- | Sov.Math.Spin3 — 3 维表示 ρ₃ 的判据：**没有公共特征向量**（det = 4 子式路线）。
--
-- 镜像 FML `SpinIrreducible3` / `SpinNoInvariantLine3`（2026-10-27 完成链）：
-- 设 v 是 ρ(g0)、ρ(g1)、ρ(g2) 的公共特征向量（g0/g1/g2 ∈ SL(2,F₃) 经 toA4 拉到 A₄：
-- Flip₀ / Rot₃₀ / Rot₂₀），则交换子
--     C = A·B − B·A = [[1,0,1],[2,−2,2],[1,−2,1]]
--     E = A·D − D·A = [[1,1,−1],[2,0,−2],[1,1,−1]]
-- 的三条行（C₀、C₂、E₂）组成线性无关方程组，行列式 = **4 ≠ 0**
-- ⇒ 三方程只有零解 ⇒ v = 0 ⇒ 3 维表示不可约。
-- （与 2 维不同：这里**不需要**无挠引理，det = 4 直接非奇异。）
--
-- 矩阵字面取自 FML `A4ThreeDimRep.agda` 的 `rho3`（toA4 g0 / g1 / g2 三件）：
--   A = [[−1,1,0],[0,1,0],[0,1,−1]]   B = [[0,−1,1],[1,−1,1],[0,0,1]]
--   D = [[0,0,−1],[1,0,−1],[1,−1,0]]
-- 四个闭乘积（与 FML 的 AB-mat/BA-mat/AD-mat/DA-mat 逐项一致）：
--   A·B = [[1,0,0],[1,−1,1],[1,−1,0]]      B·A = [[0,0,−1],[−1,1,−1],[0,1,−1]]
--   A·D = [[1,0,0],[1,0,−1],[0,1,−1]]      D·A = [[0,−1,1],[−1,0,1],[−1,0,0]]
--
-- 零浮点、零 partial（add3/mulV3 只接受模块内构造的 3 元行）。
module Sov.Math.Spin3
  ( matA3, matB3, matD3
  , ab3, ba3, ad3, da3
  , mulV3, mulM3
  , rowC, rowE, rowE2, detRows
  , noCommonEigenvector3, spin3Ok
  ) where

import Sov.Math.Eis
import Sov.Math.Error (constitutionalPanic)

-- 恰 3 元的行 / 3 行的矩阵（长度由模块内构造保证，越界走 constitutionalPanic）
type Row3 = [Eis]
type Mat3 = [Row3]

e1 :: Eis
e1 = Eis 1 0

add3 :: [Eis] -> Eis
add3 [a, b, c] = addE (addE a b) c
add3 _         = constitutionalPanic "Spin3.add3: 行必须是 3 项"

subR :: Row3 -> Row3 -> Row3
subR [a, b, c] [x, y, z] = [subE a x, subE b y, subE c z]
subR _ _                 = constitutionalPanic "Spin3.subR: 行必须是 3 项"

mulV3 :: Mat3 -> Row3 -> Row3
mulV3 [[a, b, c], [d, e, f], [g, h, i]] [x, y, z] =
  [ add3 [mulE a x, mulE b y, mulE c z]
  , add3 [mulE d x, mulE e y, mulE f z]
  , add3 [mulE g x, mulE h y, mulE i z] ]
mulV3 _ _ = constitutionalPanic "Spin3.mulV3: 矩阵必须是 3 行"

-- 行组合式矩阵乘法（与 FML mulMM3 同构：第 i 行 = Σₖ aᵢₖ·bₖ）
mulM3 :: Mat3 -> Mat3 -> Mat3
mulM3 [[a0, a1, a2], [b0, b1, b2], [c0, c1, c2]]
      [[b00, b01, b02], [b10, b11, b12], [b20, b21, b22]] =
  [ [ addE (addE (mulE a0 b00) (mulE a1 b10)) (mulE a2 b20)
    , addE (addE (mulE a0 b01) (mulE a1 b11)) (mulE a2 b21)
    , addE (addE (mulE a0 b02) (mulE a1 b12)) (mulE a2 b22) ]
  , [ addE (addE (mulE b0 b00) (mulE b1 b10)) (mulE b2 b20)
    , addE (addE (mulE b0 b01) (mulE b1 b11)) (mulE b2 b21)
    , addE (addE (mulE b0 b02) (mulE b1 b12)) (mulE b2 b22) ]
  , [ addE (addE (mulE c0 b00) (mulE c1 b10)) (mulE c2 b20)
    , addE (addE (mulE c0 b01) (mulE c1 b11)) (mulE c2 b21)
    , addE (addE (mulE c0 b02) (mulE c1 b12)) (mulE c2 b22) ] ]
mulM3 _ _ = constitutionalPanic "Spin3.mulM3: 矩阵必须是 3 行"

-- 三个 A₄ 元素的 ρ₃（FML A4ThreeDimRep.agda rho3：toA4 g0 / g1 / g2）
matA3, matB3, matD3 :: Mat3
matA3 = [ [Eis (negate 1) 0, e1, eisZero]
        , [eisZero, e1, eisZero]
        , [eisZero, e1, neg1e] ]
  where neg1e = Eis (negate 1) 0
matB3 = [ [eisZero, neg1e, e1]
        , [e1, neg1e, e1]
        , [eisZero, eisZero, e1] ]
  where neg1e = Eis (negate 1) 0
matD3 = [ [eisZero, eisZero, neg1e]
        , [e1, eisZero, neg1e]
        , [e1, neg1e, eisZero] ]
  where neg1e = Eis (negate 1) 0

-- 四个闭乘积（由 mulM3 计算；与 FML 已证值逐项一致，测试断言）
ab3, ba3, ad3, da3 :: Mat3
ab3 = mulM3 matA3 matB3
ba3 = mulM3 matB3 matA3
ad3 = mulM3 matA3 matD3
da3 = mulM3 matD3 matA3

-- 交换子行：C = A·B − B·A（3 行），E = A·D − D·A（3 行）
rowC :: [Row3]
rowC = zipWith subR ab3 ba3

rowE :: [Row3]
rowE = zipWith subR ad3 da3

rowE2 :: Row3
rowE2 = case rowE of
  [_, _, e2] -> e2
  _          -> constitutionalPanic "Spin3.rowE2: 需要 3 行"

-- 判据行组：C₀、C₂、E₂ 的 3×3 行列式
rows3 :: Mat3
rows3 = case rowC of
  [c0, _, c2] -> [c0, c2, rowE2]
  _           -> constitutionalPanic "Spin3.rows3: 需要 3 行"

detRows :: Eis
detRows = case rows3 of
  [ [r00, r01, r02], [r10, r11, r12], [r20, r21, r22] ] ->
    addE (addE t1 t2) t3
    where
      t1 = mulE r00 (subE (mulE r11 r22) (mulE r12 r21))
      t2 = negE (mulE r01 (subE (mulE r10 r22) (mulE r12 r20)))
      t3 = mulE r02 (subE (mulE r10 r21) (mulE r11 r20))
  _ -> constitutionalPanic "Spin3.detRows: 需要 3×3"

-- 判据本体：det = 4 ≠ 0 ⇒ 交换子方程组只有零解
noCommonEigenvector3 :: Bool
noCommonEigenvector3 =
  detRows == Eis 4 0
  && rowC == [ [e1, eisZero, e1]
              , [Eis 2 0, Eis (negate 2) 0, Eis 2 0]
              , [e1, neg2e, e1] ]
  && rowE2 == [e1, e1, neg1e]
  where neg2e = Eis (negate 2) 0
        neg1e = Eis (negate 1) 0

spin3Ok :: Bool
spin3Ok = noCommonEigenvector3
  && ab3 == [ [e1, eisZero, eisZero]
            , [e1, neg1e, e1]
            , [e1, neg1e, eisZero] ]
  && ba3 == [ [eisZero, eisZero, neg1e]
            , [neg1e, e1, neg1e]
            , [eisZero, e1, neg1e] ]
  && ad3 == [ [e1, eisZero, eisZero]
            , [e1, eisZero, neg1e]
            , [eisZero, e1, neg1e] ]
  && da3 == [ [eisZero, neg1e, e1]
            , [neg1e, eisZero, e1]
            , [neg1e, eisZero, eisZero] ]
  where neg1e = Eis (negate 1) 0
