-- | Sov.Math.Hopf — 离散**分层商** T⁶ → T⁶/A₄（GF(3)⁶，729 点）；Hopf 只是类比
--
-- ⚠ P2（2026-10-26 评审裁定）：A₄ 只置换**前 4 坐标**，作用**不自由**——有 27 个固定点，
--   纤维大小取值 1/4/6/12 而非常数。因此这**不是**主丛，也不是纤维丛，而是分层商；
--   "无等变截面"在这里是**固定点的直接推论**（固定点处 g·s(b) = s(b)），
--   不是 S³→S² 那种拓扑障碍。凡引用本模块，必须按"分层商 + 类比"表述。
--
-- 与 Agda `Sovereign.Synthesis.Homotopy.HopfConstruction` 同一结构（并补齐它没算的那一项）：
--
--   全空间  T⁶ = 729 点（`Tryte`，6 个 trit）
--   底空间  T⁶/A₄ = **135** 个等价类（A₄ 偶置换只动前 4 坐标，后 2 坐标自由：15 × 9）
--   纤维    轨道 A₄·x，大小 = 12 / |Stab|（取值 1 / 4 / 6 / 12）
--   投影    projClass : T⁶ → Fin 135
--
-- 关键性质（本模块以全枚举验证，不靠断言）：
--   · projClass 在 A₄ 轨道上不变          —— hopf-orbit-invariant
--   · projClass 取满 135 个值              —— hopf-base-card
--   · 纤维大小分布 {1:27, 4:54, 6:27, 12:27} —— hopf-fiber-distribution
--   · 因此 **729 − 27 = 702** 才是"非固定点"，而不是 729 − 9；
--     `1×27 + 13×54 = 729` 只是 702 = 13×54 的算术巧合，**不是 A₄ 轨道分解**
--     （A₄ 的轨道大小上限是 12，不可能有 54 元轨道）。
--
-- 零浮点、零 partial（-Wall -Werror）。
module Sov.Math.Hopf
  ( T6Point, allT6Points
  , actOn, orbitOf, fiberSizeOf
  , projClass, projKey, baseCard
  , orbitReps, fiberSizeDistribution
  ) where

import Data.List (nub, sort)
import Data.Word (Word16)

import Sov.Math.A4
import Sov.Math.Error (constitutionalPanic)
import Sov.Math.Trit
import Sov.Math.Tryte
import Sov.Math.Vec

type T6Point = Tryte

-- | 729 个格点（值域由 mkTryte 保证封闭）
allT6Points :: [T6Point]
allT6Points = [ t | v <- ([0 .. 728] :: [Word16]), Just t <- [mkTryte v] ]

--------------------------------------------------------------------------------
-- 1. A₄ 作用：置换前 4 个坐标，后 2 个固定
--------------------------------------------------------------------------------

actOn :: A4 -> T6Point -> T6Point
actOn g t = case toList (tryteToVec t) of
    [v0, v1, v2, v3, v4, v5] ->
      let p    = a4Perm g
          vs   = [v0, v1, v2, v3]
          pick i = case drop i vs of
                     (x : _) -> x
                     []      -> constitutionalPanic "Hopf: 坐标下标越界（f=偶置换，不可达）"
          new4 = [ pick i | i <- p ]
      in case new4 of
           [w0, w1, w2, w3] -> tryteFromVec (w0 :> w1 :> w2 :> w3 :> v4 :> v5 :> VNil)
           _                -> constitutionalPanic "Hopf: A₄ 置换长度必须为 4"
    _ -> constitutionalPanic "Hopf: T⁶ 点必须是 6 个 trit"

orbitOf :: T6Point -> [T6Point]
orbitOf x = nub [ actOn g x | g <- allA4 ]

fiberSizeOf :: T6Point -> Int
fiberSizeOf = length . orbitOf

--------------------------------------------------------------------------------
-- 2. 投影：T⁶ → T⁶/A₄（135 类）
--
-- 轨道由「前 4 坐标的多重集」+「第 5、6 坐标」唯一决定：
--   · 多重集 (n₀,n₁,n₂)，n₀+n₁+n₂ = 4 —— 共 C(6,4) = 15 种
--   · 后 2 坐标各 3 种                      —— 共 9 种
--   ⇒ 15 × 9 = 135
--------------------------------------------------------------------------------

multiSets15 :: [(Int, Int, Int)]
multiSets15 =
  [ (n0, n1, n2)
  | n0 <- [4, 3 .. 0]
  , n1 <- [4 - n0, 4 - n0 - 1 .. 0]
  , let n2 = 4 - n0 - n1
  , n2 >= 0
  ]

multiSetCode :: (Int, Int, Int) -> Int
multiSetCode k = case [ i | (i, m) <- zip [0 ..] multiSets15, m == k ] of
    (i : _) -> i
    []      -> constitutionalPanic "Hopf: 多重集超出 (n₀+n₁+n₂=4)"

-- | 轨道键：(n₀, n₁, n₂, 第5坐标, 第6坐标)
projKey :: T6Point -> (Int, Int, Int, Trit, Trit)
projKey t = case toList (tryteToVec t) of
    [v0, v1, v2, v3, v4, v5] ->
      let count x = length [ () | v <- [v0, v1, v2, v3], v == x ]
      in (count T0, count T1, count T2, v4, v5)
    _ -> constitutionalPanic "Hopf: T⁶ 点必须是 6 个 trit"

-- | 底空间坐标 0..134
projClass :: T6Point -> Int
projClass t =
  let (n0, n1, n2, v4, v5) = projKey t
  in multiSetCode (n0, n1, n2) * 9
       + fromIntegral (toTritIndex v4) * 3
       + fromIntegral (toTritIndex v5)

baseCard :: Int
baseCard = 135

-- | 每条轨道取一个代表（135 条）。用于构造截面与逐点判据。
orbitReps :: [T6Point]
orbitReps = nubBy projClass allT6Points
  where
    nubBy f = go []
      where
        go _ [] = []
        go seen (x : xs)
          | f x `elem` seen = go seen xs
          | otherwise       = x : go (f x : seen) xs

--------------------------------------------------------------------------------
-- 3. 纤维大小分布（全枚举）
--------------------------------------------------------------------------------

-- | [(纤维大小, 该大小的轨道条数)]，按大小升序
fiberSizeDistribution :: [(Int, Int)]
fiberSizeDistribution = sort [ (s, length (filter (== s) sizes)) | s <- nub sizes ]
  where
    sizes = map fiberSizeOf orbitReps   -- 复用顶层 orbitReps（135 条）
