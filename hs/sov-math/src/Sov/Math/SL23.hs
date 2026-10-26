-- | Sov.Math.SL23 — 2A₄ = SL(2,F₃)：24 个 2×2 幺模矩阵（GF(3) 精确，零浮点）
--
-- 定位：这是"旋量 / 相位"那条腿。A₄ **没有 4 阶元**，所以 C₄ 相位（α，90°）
-- 不可能住在 A₄ 里；它必须上到双层覆盖 2A₄ = SL(2,3) ≅ SL(2,F₃)（|G| = 24）。
--
-- 与 Agda 对齐：
--   `Sovereign.Synthesis.Representation.SL23Cayley`（24 个 Kayley 表）
--   `Sovereign.Algebra.GroupTheory.BraidToSL23`（σ₁/σ₂ 的 Burau 像、`eval-W`、`negI-central`）
--   本模块把 `W = (σ₁σ₂)³ = −I` 与「SL(2,F₃) 在 8 个非零向量上传递」搬成可跑的实现。
--
-- 关键事实（全部由测试全枚举验证）：
--   · 24 个元素、对乘封闭、每个有逆、定义表示**忠实**（24 个互不相同）
--   · σ₁ = [[1,1],[0,1]]、σ₂ = [[1,0],[2,1]]，阶都是 3（char 3）
--   · **W = (σ₁σ₂)³ = −I**                    —— 与 Agda `eval-W` 同一结论
--   · −I 中心、阶 2                              —— 与 `negI-central` 同一结论
--   · 存在 4 阶元                              —— α 相位的住处（A₄ 里没有）
--   · 在 GF(3)² 的 8 个非零向量上**传递**、0 固定 ⇒ **9 = 1 + 8**
module Sov.Math.SL23
  ( M2(..)
  , allSL23, sl23Card
  , matId, matMul, matDet, matInv, matPow, orderOf
  , sigma1, sigma2, negI, wElement
  , elemsOfOrder4
  , actVec, nonzeroVectors, orbitOfVec
  ) where

import Data.List (nub)

import Sov.Math.Error (constitutionalPanic)
import Sov.Math.Trit

-- | GF(3) 上的 2×2 矩阵（元素是 Trit ⇒ 值域天然封闭）
data M2 = M2 Trit Trit Trit Trit deriving (Eq, Show)

sl23Card :: Int
sl23Card = 24

allT :: [Trit]
allT = allTrits

matDet :: M2 -> Trit
matDet (M2 a b c d) = addT (mulT a d) (negT (mulT b c))

-- | 全部 24 个行列式 = 1 的矩阵（穷举 3⁴ = 81 个候选）
allSL23 :: [M2]
allSL23 =
  [ m
  | a <- allT, b <- allT, c <- allT, d <- allT
  , let m = M2 a b c d
  , matDet m == T1
  ]

matId :: M2
matId = M2 T1 T0 T0 T1

matMul :: M2 -> M2 -> M2
matMul (M2 a b c d) (M2 e f g h) =
  M2 (addT (mulT a e) (mulT b g))
     (addT (mulT a f) (mulT b h))
     (addT (mulT c e) (mulT d g))
     (addT (mulT c f) (mulT d h))

-- | 逆：det = 1 时 inv = [[d,−b],[−c,a]]；det = −1 时再乘 det（GF(3) 中 det⁻¹ = det）
matInv :: M2 -> M2
matInv m@(M2 a b c d) =
  case matDet m of
    T1 -> M2 d (negT b) (negT c) a
    T2 -> matMul (M2 T2 T0 T0 T2) (M2 d (negT b) (negT c) a)
    T0 -> constitutionalPanic "SL23: 奇异矩阵不可逆（allSL23 已排除）"

matPow :: Int -> M2 -> M2
matPow n m
  | n <= 0    = matId
  | otherwise = matMul m (matPow (n - 1) m)

-- | 元素阶（1..24 内搜索；SL(2,3) 最大阶 6）
orderOf :: M2 -> Int
orderOf m = case [ k | k <- [1 .. 24], matPow k m == matId ] of
    (k : _) -> k
    []      -> constitutionalPanic "SL23: 元素阶超过 24（不可达）"

--------------------------------------------------------------------------------
-- 生成元（Burau 在 t ≡ −1 mod 3 处的取值）
--------------------------------------------------------------------------------

sigma1, sigma2, negI :: M2
sigma1 = M2 T1 T1 T0 T1   -- [[1,1],[0,1]]
sigma2 = M2 T1 T0 T2 T1   -- [[1,0],[2,1]]
negI   = M2 T2 T0 T0 T2   -- −I = [[2,0],[0,2]]

-- | W = (σ₁σ₂)³ —— 辫群的中央元，在 SL(2,F₃) 里落到 −I
wElement :: M2
wElement = matPow 3 (matMul sigma1 sigma2)

elemsOfOrder4 :: [M2]
elemsOfOrder4 = [ m | m <- allSL23, orderOf m == 4 ]

--------------------------------------------------------------------------------
-- 在 GF(3)² 上的作用（9 个向量 = 8 非零 + 0）
--------------------------------------------------------------------------------

actVec :: M2 -> (Trit, Trit) -> (Trit, Trit)
actVec (M2 a b c d) (x, y) = (addT (mulT a x) (mulT b y), addT (mulT c x) (mulT d y))

nonzeroVectors :: [(Trit, Trit)]
nonzeroVectors = [ (x, y) | x <- allT, y <- allT, (x, y) /= (T0, T0) ]

orbitOfVec :: (Trit, Trit) -> [(Trit, Trit)]
orbitOfVec v = nub [ actVec m v | m <- allSL23 ]
