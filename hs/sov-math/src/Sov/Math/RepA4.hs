-- | Sov.Math.RepA4 — A₄ 的不可约表示：特征标表 + 正交性（Z[ω] 精确，整数形式）
--
-- 与 Agda 对齐：`Sovereign.Synthesis.A4.A4Representation`（特征标表 + 正交性）
--               `Sovereign.Synthesis.A4.A4OneDimHom`（三个 1 维表示由 abelianization A₄→C₃ 导出）
--
-- ⚠ 类划分是**算出来的，不是按构造子名猜的**（2026-10-26 实测）：
--   `Rot i G0` 并不构成一个共轭类。按 `a4Perm` 的置换语义计算共轭，
--   四个类是：
--
--     CE   {Id}
--     CV₄  {Flip H0, Flip H1, Flip H2}                 = V₄ \ {e}
--     CA   {Rot0G0, Rot2G0, Rot1G1, Rot3G1}
--     CB   {Rot0G1, Rot1G0, Rot2G1, Rot3G0}
--
--   三个 1 维表示在 CV₄ 上恒为 1（即核 ⊇ V₄）⇒ A₄ **没有忠实的 1 维表示** ⇒
--   "球谐矢量不要标量" 的群论理由（标量 = 1 维 = 必然丢 A₄ 结构）。
--
-- 四个不可约表示：
--
--   类      CE(1)   CV₄(3)   CA(4)   CB(4)
--   R1       1        1        1       1
--   R1'      1        1        ω       ω²
--   R1''     1        1        ω²      ω
--   R3       3       −1        0       0
--
-- 关键事实（本模块给的，不是抄的）：
--   · chiOrth ρ σ = Σ_g χ_ρ(g)·conj χ_σ(g) = 12·δ_{ρσ}     —— 行正交（整数，无分母）
--   · 正则表示多重度：Σ_g χ_reg(g)·conj χ_ρ(g) = 12·d_ρ    —— 每个 irrep 出现 d_ρ 次
--   · 列正交在单位元处 ⇒ Σ_ρ d_ρ² = |A₄| = 12              —— Plancherel 的结构证明
--
-- 归一化：|A₄| = 12 在 Z[ω] 中不可逆 ⇒ 一律写 `12·(...)` 的整数形式。
-- 零浮点、零 partial。
module Sov.Math.RepA4
  ( Irrep(..), allIrreps, irrepCount, dimI
  , ChiClass(..), classOf, classA, classB
  , chi
  , chiOrth
  , regularCharacter
  , regularMultiplicityNum
  , colOrthAtId
  , oneDimRepsNonFaithful
  ) where

import Data.List (nub)

import Sov.Math.A4
import Sov.Math.Eis
import Sov.Math.Error (constitutionalPanic)

-- | 不可约表示指标（4 个 = 4 个共轭类）
data Irrep = R1 | R1' | R1'' | R3 deriving (Eq, Ord, Show)

allIrreps :: [Irrep]
allIrreps = [R1, R1', R1'', R3]

irrepCount :: Int
irrepCount = 4

dimI :: Irrep -> Int
dimI R1   = 1
dimI R1'  = 1
dimI R1'' = 1
dimI R3   = 3

--------------------------------------------------------------------------------
-- 1. 共轭类（划分来自实测；其共轭不变性由测试 rep_class_conj_invariant 验证）
--------------------------------------------------------------------------------

data ChiClass = CE | CV4 | CA | CB deriving (Eq, Ord, Show)

-- | 共轭类：**由 mulA4/invA4 算出**（不再手写清单）
conjA4 :: A4 -> [A4]
conjA4 g = nub [ mulA4 (mulA4 h g) (invA4 h) | h <- allA4 ]

a4Classes :: [[A4]]
a4Classes = go allA4
  where
    go []       = []
    go (g : gs) = conjA4 g : go (filter (`notElem` conjA4 g) gs)

-- | 含给定元素的类
classContaining :: A4 -> [A4]
classContaining g = case [ c | c <- a4Classes, g `elem` c ] of
    (c : _) -> c
    []      -> constitutionalPanic "RepA4: 元素不在任何共轭类中（不可达）"

-- | 类 CA：含 `Rot F0 G0` 的 3-循环类
classA :: [A4]
classA = classContaining (Rot F0 G0)

-- | 类 CB：另一个 3-循环类（全为 Rot 且与 CA 不同）
classB :: [A4]
classB = case [ c | c <- a4Classes, c /= classA, all isRot c ] of
    (c : _) -> c
    []      -> constitutionalPanic "RepA4: 找不到第二个 3-循环类（不可达）"
  where
    isRot (Rot _ _) = True
    isRot _         = False

classOf :: A4 -> ChiClass
classOf Id       = CE
classOf (Flip _) = CV4
classOf g
  | g `elem` classA = CA
  | g `elem` classB = CB
  | otherwise       = CE   -- Id / Flip 已在上面返回，不可达

--------------------------------------------------------------------------------
-- 2. 特征标表
--------------------------------------------------------------------------------

chi :: Irrep -> A4 -> Eis
chi R1   _ = eisOne
chi R1'  g = case classOf g of
    CE  -> eisOne
    CV4 -> eisOne
    CA  -> eisOmega
    CB  -> eisOmega2
chi R1'' g = case classOf g of
    CE  -> eisOne
    CV4 -> eisOne
    CA  -> eisOmega2
    CB  -> eisOmega
chi R3 Id       = Eis 3 0
chi R3 (Flip _) = Eis (-1) 0
chi R3 (Rot _ _) = eisZero

--------------------------------------------------------------------------------
-- 3. 行正交性（Schur）：Σ_g χ_ρ(g)·conj χ_σ(g) = |A₄|·δ_{ρσ}
--------------------------------------------------------------------------------

chiOrth :: Irrep -> Irrep -> Eis
chiOrth rho sigma =
  foldr addE eisZero [ mulE (chi rho g) (conjE (chi sigma g)) | g <- allA4 ]

--------------------------------------------------------------------------------
-- 4. 正则表示：χ_reg = |A₄|·δ_e —— 每个 irrep 出现 d_ρ 次
--------------------------------------------------------------------------------

regularCharacter :: A4 -> Eis
regularCharacter Id = Eis 12 0
regularCharacter _  = eisZero

-- | Σ_g χ_reg(g)·conj χ_ρ(g) = 12 · d_ρ
regularMultiplicityNum :: Irrep -> Eis
regularMultiplicityNum rho =
  foldr addE eisZero
    [ mulE (regularCharacter g) (conjE (chi rho g)) | g <- allA4 ]

--------------------------------------------------------------------------------
-- 5. 列正交在单位元处 ⇒ Plancherel 恒等式 Σ_ρ d_ρ² = |A₄|
--------------------------------------------------------------------------------

colOrthAtId :: Eis
colOrthAtId =
  foldr addE eisZero
    [ mulE (Eis (fromIntegral (dimI rho)) 0) (chi rho Id) | rho <- allIrreps ]

--------------------------------------------------------------------------------
-- 6. 三个 1 维表示非忠实（核 ⊇ V₄）
--------------------------------------------------------------------------------

oneDimRepsNonFaithful :: Bool
oneDimRepsNonFaithful =
  all (\rho -> all (\g -> chi rho g == eisOne) v4) [R1, R1', R1'']
  where
    v4 = Id : [ Flip k | k <- [minBound .. maxBound] ]
