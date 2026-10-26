-- | Sov.Math.Cover2A4 — **T1（P0）**：覆盖映射 SL(2,F₃) → A₄，核 = {±I}
--
-- 这是全库最吃重的一处：此前 "2A₄ = SL(2,F₃)" 只是**命名**。本模块把它变成定理：
--
--   ① P¹(F₃) = GF(3)² 的 4 条 1 维子空间（直线）
--   ② 每个 g ∈ SL(2,F₃) 置换这 4 条线 ⇒ `permOfM g : [Int]`（4 点置换）
--   ③ 该置换是**同态**（`cover_perm_is_hom`，576 对全枚举）
--   ④ 像 ⊆ A₄（每个像是偶置换）
--   ⑤ 像 **恰等于** A₄（与我们 12 个标签置换的集合逐项相等）
--   ⑥ 核 = {I, −I}（24 个矩阵里只有这两个诱导恒等置换）
--   ⑦ 由 ⑤⑥ 得 |SL(2,F₃)/{±I}| = 12 = |A₄| ⇒ `SL(2,F₃)/{±I} ≅ A₄`
--
-- 于是"双重覆盖"、"旋量符号来自 −I"（χ(−I) = −2）都有了群论根据。
--
-- 全枚举口径（S6）：③ 576 对、⑤ 24 项、⑥ 24 项，均为**有限全枚举**，非归纳证明。
-- 零浮点、零 partial（`!!` 只在长度已定的 4 元列表上）。
module Sov.Math.Cover2A4
  ( lineReps, canonLine, lineIndex
  , permOfM, composeP, isEvenP
  , imagePerms, kernelOfCover
  , coverOk
  ) where

import Data.List (nub, sort)

import Sov.Math.A4 (A4, a4Perm, allA4)
import Sov.Math.Error (constitutionalPanic)
import Sov.Math.SL23 (M2, allSL23, matId, matMul, negI, actVec)
import Sov.Math.Trit (Trit (..), mulT)

--------------------------------------------------------------------------------
-- ① P¹(F₃)：4 条 1 维子空间
--------------------------------------------------------------------------------

-- | 每条线的规范代表（首非零坐标归一为 1）
lineReps :: [(Trit, Trit)]
lineReps = [ (T1, T0), (T0, T1), (T1, T1), (T1, T2) ]

-- | 把非零向量规范成其所在线的代表（v ~ 2v = −v）
canonLine :: (Trit, Trit) -> (Trit, Trit)
canonLine (T0, T0) = constitutionalPanic "Cover2A4: 零向量不构成直线"
canonLine (T0, _)  = (T0, T1)
canonLine (T1, y)  = (T1, y)
canonLine (T2, y)  = (T1, mulT T2 y)     -- 乘 −1 使首坐标变 1

-- | 线索引 0..3
lineIndex :: (Trit, Trit) -> Int
lineIndex v = case [ i | (i, r) <- zip [0 ..] lineReps, r == canonLine v ] of
    (i : _) -> i
    []      -> constitutionalPanic "Cover2A4: 不在 4 条线中（不可达）"

--------------------------------------------------------------------------------
-- ②③ 置换与同态
--------------------------------------------------------------------------------

-- | g 在 4 条线上的作用
permOfM :: M2 -> [Int]
permOfM m = [ lineIndex (actVec m r) | r <- lineReps ]

-- | 置换复合：先 q 后 p
composeP :: [Int] -> [Int] -> [Int]
composeP p q = [ p !! (q !! i) | i <- [0 .. 3] ]

-- | 偶置换判定（逆序数）
isEvenP :: [Int] -> Bool
isEvenP p = even (length [ () | i <- [0 .. 3], j <- [i + 1 .. 3], p !! i > p !! j ])

imagePerms :: [[Int]]
imagePerms = nub (map permOfM allSL23)

-- | 核：诱导恒等置换的矩阵
kernelOfCover :: [M2]
kernelOfCover = [ m | m <- allSL23, permOfM m == [0, 1, 2, 3] ]

--------------------------------------------------------------------------------
-- ⑦ 综合自检
--------------------------------------------------------------------------------

coverOk :: Bool
coverOk =
  -- ③ 同态（576 对）
  all (\(g, h) -> permOfM (matMul g h) == composeP (permOfM g) (permOfM h)) ghPairs
  -- ④ 像 ⊆ A₄
  && all isEvenP imagePerms
  -- ⑤ 像 = A₄（与我们 12 个标签置换逐项一致）
  && sort imagePerms == sort (map a4Perm (allA4 :: [A4]))
  -- ⑥ 核 = {I, −I}
  && length kernelOfCover == 2
  && matId `elem` kernelOfCover
  && negI `elem` kernelOfCover
  -- ⑦ 商阶 = 12
  && length imagePerms == 12
  where
    ghPairs = [ (g, h) | g <- allSL23, h <- allSL23 ]
