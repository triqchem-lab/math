-- | Sov.Math.A4 — A₄（12 元交替群）作为 4 点的偶置换
--
-- 强类型化（对齐本库原则）：12 个元素用**封闭构造子**表达，非法值在类型层不可表达；
-- 不用 `Word8` 索引，避免 `Rot 9 9` 这类越界元素。
--
-- 置换约定（与 Agda `Sovereign.Synthesis.A4.A4Group.perm` 一致）：
--   Rot i zero : 绕顶点 i 的 3-轮换（不含 i 的三个顶点）—— **顶点固定性已被测试验证**
--                （`a4_rot_fixes_its_vertex`）；
--   Rot i one  : 反向 3-轮换；
--   Flip k     : 三个双对换 (0 1)(2 3) / (0 2)(1 3) / (0 3)(1 2)——**已被测试验证**
--                （`a4_flip_two_transpositions`：无不动点的对合）。
--
-- ⚠ P7：`zero`/`one`（G0/G1）只是**方向标签**——本库没有约定坐标系/朝向，
--   因此"顺时针/逆时针"**没有可检验定义**，不得当作几何语义引用。
--
-- 乘法 `mulA4 g h` = 先 h 后 g（左作用）：`(g∘h)(x) = g(h(x))`。
-- 零 partial：不使用 `head` / `!!` / `fromJust`（-Wx-partial 在 -Wall 内）。
module Sov.Math.A4
  ( A4(..), F4(..), F2(..), F3(..)
  , allA4, a4Card
  , a4Perm, a4Index, a4OfPerm
  , idA4, mulA4, invA4
  , genG1, genG2, genA4, a4Generated
  ) where

import Data.List (find, nub, sort)

import Sov.Math.Error (constitutionalPanic)

-- | 4 / 2 / 3 值的封闭枚举（对应 Agda 的 Fin 4 / Fin 2 / Fin 3）
data F4 = F0 | F1 | F2 | F3 deriving (Eq, Ord, Show, Enum, Bounded)
data F2 = G0 | G1 deriving (Eq, Ord, Show, Enum, Bounded)
data F3 = H0 | H1 | H2 deriving (Eq, Ord, Show, Enum, Bounded)

-- | A₄ = 1（Id）+ 4×2（Rot）+ 3（Flip）= 12
data A4 = Id | Rot F4 F2 | Flip F3 deriving (Eq, Ord, Show)

a4Card :: Int
a4Card = 12

allA4 :: [A4]
allA4 = [Id]
     ++ [ Rot i j | i <- [minBound .. maxBound], j <- [minBound .. maxBound] ]
     ++ [ Flip k  | k <- [minBound .. maxBound] ]

-- | 安全取第 k 项（替代 `!!`）
atMay :: [a] -> Int -> Maybe a
atMay []       _ = Nothing
atMay (x : _)  0 = Just x
atMay (_ : xs) k = atMay xs (k - 1)

-- | 作为 {0,1,2,3} 上的偶置换（列表第 i 项 = 第 i 点的像）
a4Perm :: A4 -> [Int]
a4Perm Id          = [0, 1, 2, 3]
a4Perm (Rot F0 G0) = [0, 2, 3, 1]   -- (1 2 3)
a4Perm (Rot F0 G1) = [0, 3, 1, 2]   -- (1 3 2)
a4Perm (Rot F1 G0) = [2, 1, 3, 0]   -- (0 2 3)
a4Perm (Rot F1 G1) = [3, 1, 0, 2]   -- (0 3 2)
a4Perm (Rot F2 G0) = [1, 3, 2, 0]   -- (0 1 3)
a4Perm (Rot F2 G1) = [3, 0, 2, 1]   -- (0 3 1)
a4Perm (Rot F3 G0) = [1, 2, 0, 3]   -- (0 1 2)
a4Perm (Rot F3 G1) = [2, 0, 1, 3]   -- (0 2 1)
a4Perm (Flip H0)   = [1, 0, 3, 2]   -- (0 1)(2 3)
a4Perm (Flip H1)   = [2, 3, 0, 1]   -- (0 2)(1 3)
a4Perm (Flip H2)   = [3, 2, 1, 0]   -- (0 3)(1 2)

-- | 元素在 allA4 中的位置（0..11）；用于 `Carrier = Vec N12 Eis` 的坐标
a4Index :: A4 -> Int
a4Index g = case findIndexIn 0 allA4 of
    Just n  -> n
    Nothing -> constitutionalPanic "A4: 元素不在 allA4 中（构造子已封闭，不可达）"
  where
    findIndexIn :: Int -> [A4] -> Maybe Int
    findIndexIn _ []       = Nothing
    findIndexIn n (h : hs) = if h == g then Just n else findIndexIn (n + 1) hs

-- | 由偶置换反查群元素（偶置换共 12 个，与 allA4 一一对应）
a4OfPerm :: [Int] -> A4
a4OfPerm p = case find (\g -> a4Perm g == p) allA4 of
    Just g  -> g
    Nothing -> constitutionalPanic "A4: 奇置换不属于 A₄"

idA4 :: A4
idA4 = Id

-- | 群乘法：先 h 后 g —— (g∘h)(x) = g(h(x))
mulA4 :: A4 -> A4 -> A4
mulA4 g h = a4OfPerm [ pick (a4Perm g) (pick (a4Perm h) i) | i <- [0 .. 3] ]
  where
    pick xs k = maybe (constitutionalPanic "A4: 置换下标越界（不可达）") id (atMay xs k)

-- | 生成元（T4）：g₁ = (0 1 2) 的 3-轮换，g₂ = (0 1)(2 3) 的双对换
genG1, genG2 :: [Int]
genG1 = [1, 2, 0, 3]      -- (0 1 2)
genG2 = [1, 0, 3, 2]      -- (0 1)(2 3)

composeP :: [Int] -> [Int] -> [Int]
composeP p q = [ p !! (q !! i) | i <- [0 .. 3] ]

-- | 由 g₁、g₂ 闭包生成的全部置换（有限 ⇒ 单侧闭包即群）
genA4 :: [[Int]]
genA4 = go [[0, 1, 2, 3]] [[0, 1, 2, 3]]
  where
    go acc []       = acc
    go acc (x : xs) =
      let new = [ y | g <- [genG1, genG2], let y = composeP g x, y `notElem` acc ]
      in go (acc ++ new) (xs ++ new)

-- | 生成集恰是我们的 12 个标签置换（T4 判据）
a4Generated :: Bool
a4Generated = length (nub genA4) == 12
              && sort genA4 == sort (map a4Perm allA4)

-- | 逆元（置换取逆）
invA4 :: A4 -> A4
invA4 g = a4OfPerm (map invImage [0 .. 3])
  where
    p = a4Perm g
    invImage j = case find (\i -> atMay p i == Just j) [0 .. 3] of
        Just i  -> i
        Nothing -> constitutionalPanic "A4: 置换非双射（不可达）"
