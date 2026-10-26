-- | Sov.Math.Carrier2A4 — C[2A₄]（24 维函数空间）+ Plancherel 内积 + **Schur 正交性**
--
-- 深化（第 3 项）：把 `Complex2A4` 的"χ = tr"接到 Plancherel 一侧。
-- 字符只是矩阵系数的**迹**（χ = Σᵢ uᵢᵢ）；真正的内积结构在矩阵系数上：
--
-- ⚠ 一条**被实测否决**的假设（2026-10-26）：经典 Schur 关系
--     Σ_g ρᵢⱼ(g)·conj ρₖₗ(g) = (|G|/d)·δᵢₖ·δⱼₗ = 12·δ
--   **只对酉表示**成立。本模型是 Z[ω] 上的非酉实现（实测 U†U = [[1,−1],[−1,2]] ≠ I），
--   该式不成立。非酉情形下正确的、可算的判据是**系数 Gram 矩阵可逆**：
--     G_{(ij),(kl)} = Σ_g uᵢⱼ(g)·conj uₖₗ(g) 是 4×4 Hermite 阵，det ≠ 0
--   ⇒ 4 个矩阵系数线性无关 ⇒ 系数空间维数 d² = 4 ⇒ 不可约（系数层面）。
--   迹只是 G 的"对角和"：χ = Σᵢ uᵢᵢ，故 ⟨χ,χ⟩ = 24 与 G 的可逆性相容。
--
-- 载体：Field = 2A₄ → Z[ω]（24 维自由模），配
--   · 左正则作用 (ρ_h f)(x) = f(h⁻¹x)
--   · Plancherel 配对 ⟨f,g⟩ = Σ_x f(x)·conj(g(x))（未归一 = 24·⟨f,g⟩_{L²}）
--
-- 全枚举口径：δ 正交 24²=576 对、正则作用 24²=576 对、Schur 4×4=16 对。
-- 零浮点、零 partial。
module Sov.Math.Carrier2A4
  ( Field, dimField, zeroF, basisAt, dotF, normF2
  , regAct2, matrixCoeff, eqF
  , coefficientGram, gramHermitian, gramDet, coefficientsIndependent
  , deltaOrthonormal, regAct2Hom, regAct2Unitary
  , carrier2Ok
  ) where

import Data.List (nub)

import Sov.Math.Complex2A4 (M2E (..), group24, idM, invM, mulM)
import Sov.Math.Eis

-- | 2A₄ 上的函数空间（24 维自由 Z[ω]-模）
type Field = M2E -> Eis

dimField :: Int
dimField = 24

zeroF :: Field
zeroF _ = eisZero

-- | δ_g
basisAt :: M2E -> Field
basisAt g x = if x == g then eisOne else eisZero

-- | Plancherel 配对（未归一 = |G|·⟨f,g⟩_{L²}）
dotF :: Field -> Field -> Eis
dotF f g = foldr (\x acc -> addE (mulE (f x) (conjE (g x))) acc) eisZero group24

normF2 :: Field -> Eis
normF2 f = dotF f f

-- | 逐点相等（`Field` 是函数，没有 Eq 实例，故显式在全群上比较）
eqF :: Field -> Field -> Bool
eqF f g = all (\x -> f x == g x) group24

-- | 左正则作用：(ρ_h f)(x) = f(h⁻¹·x)
regAct2 :: M2E -> Field -> Field
regAct2 h f = \x -> f (mulM (invM h) x)

-- | 定义表示的矩阵系数 uᵢⱼ(g) = (ρ(g))ᵢⱼ，i, j ∈ {0,1}
matrixCoeff :: Int -> Int -> Field
matrixCoeff 0 0 (M2E a _ _ _) = a
matrixCoeff 0 _ (M2E _ b _ _) = b
matrixCoeff _ 0 (M2E _ _ c _) = c
matrixCoeff _ _ (M2E _ _ _ d) = d

--------------------------------------------------------------------------------
-- 检验
--------------------------------------------------------------------------------

deltaOrthonormal :: Bool
deltaOrthonormal =
  all (\(g, h) -> dotF (basisAt g) (basisAt h) == (if g == h then eisOne else eisZero))
      [ (g, h) | g <- group24, h <- group24 ]

regAct2Hom :: Bool
regAct2Hom =
  all (\(g, h) -> all (\i -> eqF (regAct2 g (regAct2 h (basisAt (elemAt i))))
                                  (regAct2 (mulM g h) (basisAt (elemAt i))))
                      [0 .. dimField - 1])
      [ (g, h) | g <- group24, h <- group24 ]
  where
    elemAt k = case drop k group24 of
                 (x : _) -> x
                 []      -> idM

regAct2Unitary :: Bool
regAct2Unitary =
  all (\g -> all (\(i, j) -> dotF (regAct2 g (basisAt (elemAt i))) (regAct2 g (basisAt (elemAt j)))
                            == dotF (basisAt (elemAt i)) (basisAt (elemAt j)))
                 idxPairs) group24
  where
    elemAt k = case drop k group24 of
                 (x : _) -> x
                 []      -> idM
    idxPairs = [ (i, j) | i <- [0 .. dimField - 1], j <- [0 .. dimField - 1] ]

-- | 系数 Gram 矩阵（4×4）
coeffPairs :: [(Int, Int)]
coeffPairs = [ (i, j) | i <- [0, 1], j <- [0, 1] ]

coefficientGram :: [[Eis]]
coefficientGram =
  [ [ dotF (matrixCoeff i j) (matrixCoeff k l) | (k, l) <- coeffPairs ]
  | (i, j) <- coeffPairs ]

-- | G 是 Hermite 的：G[i][j] == conj(G[j][i])
gramHermitian :: Bool
gramHermitian =
  all (\(i, j) -> matrixAt i j == conjE (matrixAt j i)) idxPairs
  where
    idxPairs = [ (i, j) | i <- [0 .. 3], j <- [0 .. 3] ]
    matrixAt i j = case drop i coefficientGram of
                     (row : _) -> case drop j row of
                                    (x : _) -> x
                                    []      -> eisZero
                     []        -> eisZero

-- | 4×4 行列式（Laplace；Eis 上精确）
det4 :: [[Eis]] -> Eis
det4 m = detN m
  where
    detN :: [[Eis]] -> Eis
    detN []  = eisOne
    detN [r] = headOf r
    detN rows = foldr addE eisZero
      [ mulE (sign k) (mulE (entry rows 0 k) (detN (minor rows k)))
      | k <- [0 .. length rows - 1] ]
    headOf (x : _) = x
    headOf []      = eisZero
    entry rows i k = case drop i rows of
                       (r : _) -> case drop k r of
                                    (x : _) -> x
                                    []      -> eisZero
                       []      -> eisZero
    minor rows k = [ [ e | (j, e) <- zip [0 ..] r, j /= k ] | r <- drop 1 rows ]
    sign k = if even k then eisOne else Eis (-1) 0

gramDet :: Eis
gramDet = det4 coefficientGram

-- | 系数线性无关（非酉情形下代替 Schur 常数的判据）
coefficientsIndependent :: Bool
coefficientsIndependent = gramDet /= eisZero

carrier2Ok :: Bool
carrier2Ok =
  length group24 == dimField
  && length (nub group24) == dimField
  && deltaOrthonormal
  && regAct2Hom
  && regAct2Unitary
  && gramHermitian
  && coefficientsIndependent
