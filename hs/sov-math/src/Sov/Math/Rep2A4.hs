-- | Sov.Math.Rep2A4 — 2A₄ = SL(2,3) 的 7 个不可约表示（Z[ω] 精确）
--
-- **类序 = 计算序**（2026-10-26 改）：7 个共轭类由 24 个矩阵做矩阵共轭算出，
-- 类的编号就是 `sl23Classes` 的索引。表中每一列对应哪一类，由
-- `classInfoC`（大小, 元素阶）与 `classRepresentatives`（代表矩阵）共同钉住，
-- 不再依赖任何"约定序"。
--
-- 表的每一行都是**算出来**的：
--   · 3 个 1 维  —— 穷举 {1,ω,ω²} 在 7 类上的分配，要求对 24×24 个乘积乘法一致
--                  （`oneDimHoms`，机械搜索，不抄表）
--   · 3 个 2 维  —— 定义表示 χ₂ 按元素阶取值（1↦2, 2↦−2, 4↦0, 3↦−1, 6↦1）：
--                  4 阶元特征值 ±i ⇒ 迹 0；3 阶元特征值 ω,ω² ⇒ 迹 −1；
--                  负元 χ(−g) = −χ(g) ⇒ 6 阶类取 +1。再与两个非平凡 1 维张量积得 χ₂′/χ₂″。
--   · 1 个 3 维  —— χ₃(A₄) 的拉回，按元素阶：1,2 ↦ 3；4 ↦ −1（双对换）；3,6 ↦ 0
--
-- 验收：7×7 = 49 对 Σ_c |c|·χ_i(c)·conj χ_j(c) = 24·δ_ij（`allCharsOrthogonal`）。
-- 维数：Σ d_ρ² = 3·1 + 3·4 + 9 = 24 = |2A₄|。
--
-- 为什么重要：2A₄ 是**唯一**能承载 C₄ 相位 α（阶 4）的地方——A₄ 没有 4 阶元。
-- 三个 2 维表示在 −I 上取 −1（χ(−I) = −2）⇒ **忠实**；1 维与 3 维在 −I 上取 +1
-- ⇒ 经 A₄ 分解、非忠实。
--
-- 标注纪律（P8）：
--   · `predictedDims`、`expectedSignature` 是**预测**，随后被
--     `dimsIdentity` / `classStructureOk` / `allCharsOrthogonal` 验证；
--   · 其余（类签名、正交性、正则多重度）是**已验证的断言**。
--
-- 零浮点；唯一的 partial 入口是 `constitutionalPanic`（不可达分支）。
module Sov.Math.Rep2A4
  ( groupOrder, classSizes7, classSizesSum
    -- * 共轭类：计算序
  , sl23Classes, classRepresentatives, classInfoC, sl23ClassOf, negClassOf, identityClassIndex
  , classSignature, expectedSignature, classStructureOk
    -- * 特征标表（同一计算序）
  , Spinor(..), allSpinors, spinorChar, spinorValue
  , oneDimHoms, nontrivialHomsSorted, firstOrder3Class, oneDimChars, threeDimChar, allChars
  , chiPairing, spinorOrthogonality, charPairingL, allCharsOrthogonal, allCharsDistinct
  , regularChar, regularMultiplicity, regularMultiplicityOk
    -- * 维数恒等式
  , predictedDims, dimsIdentity
  ) where

import Data.List (nub, sort)

import Sov.Math.Complex2A4 (spinorCharFromModel)
import Sov.Math.Eis
import Sov.Math.Error (constitutionalPanic)
import Sov.Math.SL23 (M2 (..), allSL23, matId, matInv, matMul, matPow)
import Sov.Math.Trit (negT)

groupOrder :: Integer
groupOrder = 24

--------------------------------------------------------------------------------
-- 1. 共轭类：由 24 个矩阵算出（矩阵共轭 + 元素阶）
--------------------------------------------------------------------------------

orderOfM :: M2 -> Int
orderOfM m = case [ k | k <- [1 .. 24], matPow k m == matId ] of
    (k : _) -> k
    []      -> constitutionalPanic "Rep2A4: 元素阶超过 24（不可达）"

conjClass :: M2 -> [M2]
conjClass m = nub [ matMul (matMul h m) (matInv h) | h <- allSL23 ]

sl23Classes :: [[M2]]
sl23Classes = go allSL23
  where
    go []       = []
    go (m : ms) = conjClass m : go (filter (`notElem` conjClass m) ms)

classRepresentatives :: [M2]
classRepresentatives = [ m | (m : _) <- sl23Classes ]

classOrderOf :: [M2] -> Int
classOrderOf (m : _) = orderOfM m
classOrderOf []      = 0

-- | 计算序下的 (类大小, 元素阶)
classInfoC :: [(Int, Int)]
classInfoC = [ (length c, classOrderOf c) | c <- sl23Classes ]

classSizes7 :: [Integer]
classSizes7 = map (fromIntegral . fst) classInfoC

classSizesSum :: Integer
classSizesSum = sum classSizes7

-- | 单位元所在的类索引（计算序下不一定为 0）
identityClassIndex :: Int
identityClassIndex = sl23ClassOf matId

-- | 元素 → 类索引（计算序）
sl23ClassOf :: M2 -> Int
sl23ClassOf m = case [ i | (m', i) <- classOfTable, m' == m ] of
    (i : _) -> i
    []      -> constitutionalPanic "Rep2A4: 元素不在 24 个矩阵中（不可达）"
  where
    classOfTable = [ (x, i) | (i, c) <- zip [0 ..] sl23Classes, x <- c ]

negM :: M2 -> M2
negM (M2 a b c d) = M2 (negT a) (negT b) (negT c) (negT d)

representativeOf :: Int -> M2
representativeOf k = case drop k classRepresentatives of
    (m : _) -> m
    []      -> constitutionalPanic "Rep2A4: 类索引越界（不可达）"

-- | 类索引 → 其负元所在的类索引（−I 的作用）
negClassOf :: Int -> Int
negClassOf i = sl23ClassOf (negM (representativeOf i))

classSignature :: [(Int, Int)]
classSignature = sort classInfoC

expectedSignature :: [(Int, Int)]
expectedSignature = sort [(1, 1), (1, 2), (4, 3), (4, 3), (4, 6), (4, 6), (6, 4)]

classStructureOk :: Bool
classStructureOk = classSignature == expectedSignature

--------------------------------------------------------------------------------
-- 2. 三个 1 维表示：穷举 hom（机械搜索）
--
-- χ(g) = ω^{k(类)}；要求 k 在类上常值、k(单位元类) = 0，且对所有 24×24 个乘积
-- 满足 ω^{k(gh)} = ω^{k(g)}·ω^{k(h)}。解恰有 3 个。
--------------------------------------------------------------------------------

atE :: [Eis] -> Int -> Eis
atE xs k = case drop k xs of
    (x : _) -> x
    []      -> eisZero

oneDimHoms :: [[Eis]]
oneDimHoms =
  [ vals
  | vals <- sequence (replicate (length classInfoC) [eisOne, eisOmega, eisOmega2])
  , atE vals (sl23ClassOf matId) == eisOne
  , all (homOK vals) ghPairs
  ]
  where
    ghPairs = [ (g, h) | g <- allSL23, h <- allSL23 ]
    homOK vals (g, h) =
      atE vals (sl23ClassOf (matMul g h))
        == mulE (atE vals (sl23ClassOf g)) (atE vals (sl23ClassOf h))

oneDimChars :: [[Eis]]
oneDimChars = oneDimHoms

--------------------------------------------------------------------------------
-- 3. 三个 2 维（旋量）表示：按元素阶取值，再与两个非平凡 1 维张量积
--------------------------------------------------------------------------------

-- | 定义表示的复特征标：**由显式 Z[ω] 矩阵的迹导出**（T2）
--
-- `Complex2A4` 给出 2A₄ ⊂ GL(2, Z[ω]) 的显式 24 元模型（⟨i,j,U⟩，闭包实测 24），
-- 其迹经 `spinorCharFromModel` 按 (类大小, 元素阶) 索引取用；
-- `traceDependsOnlyOnSizeOrder` 证明这个索引是良定义的。
spinorDefining :: [Eis]
spinorDefining = [ spinorCharFromModel sz od | (sz, od) <- classInfoC ]

-- | 按阶读数（原实现）——现在只是**推论**：与模型迹逐项一致（见测试 rep2_spinor_matches_model）
spinorValue :: Int -> Eis
spinorValue 1 = Eis 2 0
spinorValue 2 = Eis (-2) 0
spinorValue 4 = eisZero
spinorValue 3 = Eis (-1) 0
spinorValue 6 = Eis 1 0
spinorValue _ = eisZero

data Spinor = S2 | S2' | S2'' deriving (Eq, Ord, Show)

allSpinors :: [Spinor]
allSpinors = [S2, S2', S2'']

-- 两个非平凡 1 维（去掉全 1 行），用于张量积
nontrivialHoms :: [[Eis]]
nontrivialHoms = [ h | h <- oneDimHoms, h /= replicate (length classInfoC) eisOne ]

-- | 计算序里第一个 3 阶类的索引
firstOrder3Class :: Int
firstOrder3Class = case [ i | (i, (_, o)) <- zip [0 ..] classInfoC, o == 3 ] of
    (i : _) -> i
    []      -> 0

-- | **钉死标签**（P6）：在该类上取 ω 的非平凡 hom 为 χ₁′，取 ω² 的为 χ₁″。
--   不再依赖穷举顺序。
nontrivialHomsSorted :: [[Eis]]
nontrivialHomsSorted =
  [ h | h <- nontrivialHoms, atE h firstOrder3Class == eisOmega ]
    ++ [ h | h <- nontrivialHoms, atE h firstOrder3Class /= eisOmega ]

spinorChar :: Spinor -> [Eis]
spinorChar S2 = spinorDefining
spinorChar S2' = zipWith mulE spinorDefining (pickNth 0)
spinorChar S2'' = zipWith mulE spinorDefining (pickNth 1)

pickNth :: Int -> [Eis]
pickNth k = case drop k nontrivialHomsSorted of
    (h : _) -> h
    []      -> replicate (length classInfoC) eisOne   -- 不可达（恰有 2 个非平凡）

--------------------------------------------------------------------------------
-- 4. 一个 3 维：χ₃(A₄) 的拉回（按元素阶）
--------------------------------------------------------------------------------

threeDimValue :: Int -> Eis
threeDimValue 1 = Eis 3 0
threeDimValue 2 = Eis 3 0
threeDimValue 4 = Eis (-1) 0
threeDimValue _ = eisZero

threeDimChar :: [Eis]
threeDimChar = map (threeDimValue . snd) classInfoC

--------------------------------------------------------------------------------
-- 5. 完整正交性（7×7 = 49 对）
--------------------------------------------------------------------------------

allChars :: [[Eis]]
allChars = oneDimChars ++ map spinorChar allSpinors ++ [threeDimChar]

charPairingL :: [Eis] -> [Eis] -> Eis
charPairingL a b = foldl addE eisZero
  [ scaleE size (mulE x (conjE y)) | (size, x, y) <- zip3 classSizes7 a b ]
  where
    scaleE n (Eis p q) = Eis (n * p) (n * q)

allCharsOrthogonal :: Bool
allCharsOrthogonal =
  all (\(i, j) -> charPairingL (allChars !! i) (allChars !! j)
                    == (if i == j then Eis 24 0 else eisZero))
      [ (i, j) | i <- [0 .. 6], j <- [0 .. 6] ]

allCharsDistinct :: Bool
allCharsDistinct = length (nub allChars) == 7

--------------------------------------------------------------------------------
-- 正则表示：χ_reg = 24·δ_I，⇒ Σ_c |c|·χ_reg(c)·conj χ_ρ(c) = 24·d_ρ
-- （每个 irrep 在 C[2A₄] 中出现 d_ρ 次；与 A₄ 侧 `RepA4.regularMultiplicityNum` 对称）
--------------------------------------------------------------------------------

regularChar :: [Eis]
regularChar = [ if i == identityClassIndex then Eis 24 0 else eisZero
              | i <- [0 .. length classInfoC - 1] ]

regularMultiplicity :: [Eis] -> Eis
regularMultiplicity chi = charPairingL regularChar chi

regularMultiplicityOk :: Bool
regularMultiplicityOk =
  all (\(chi, d) -> regularMultiplicity chi == Eis (24 * d) 0)
      [ (chi, dimOf chi) | chi <- allChars ]
  where
    dimOf chi = case drop identityClassIndex chi of
                  (Eis a _ : _) -> a
                  []            -> 0

chiPairing :: Spinor -> Spinor -> Eis
chiPairing i j = charPairingL (spinorChar i) (spinorChar j)

spinorOrthogonality :: Bool
spinorOrthogonality =
  all (\(i, j) -> chiPairing i j == (if i == j then Eis 24 0 else eisZero))
      [ (i, j) | i <- allSpinors, j <- allSpinors ]

--------------------------------------------------------------------------------
-- 6. 维数恒等式 Σ_ρ d_ρ² = |G| = 24
--------------------------------------------------------------------------------

predictedDims :: [Integer]
predictedDims = [1, 1, 1, 2, 2, 2, 3]

dimsIdentity :: Bool
dimsIdentity = sum [ d * d | d <- predictedDims ] == groupOrder
