-- | Sov.Math.Field — 关联丛的截面空间，以及**扭转缺陷**（可算量）
--
-- 定位（2026-10-26）：这是"环更智能"从叙事变成数字的地方。
--
--   底空间     T⁶/A₄                 135 类
--   抽象纤维   C[A₄]                 12 维（`Sov.Math.CarrierA4`）
--   截面       Section = Base → C[A₄]
--
-- 两个维数必须分开（这是本模块唯一实质内容）：
--
--   平纤维维数  flatDim     = 135 × 12 = **1620**   —— 若丛可平凡化，截面空间就这么大
--   有效维数    effectiveDim = Σ_b |fiber(b)| = 729 —— 真实全空间的函数个数
--   超额        overcount = 1620 − 729 = **891**
--
-- ⚠ P1（2026-10-26 评审裁定）：**891 不是这个商的不变量，只是两个描述的差**。
--   典范的事实是 `Σ_b |fiber(b)| = 729`（它就是 |T⁶|，无任何选择）；
--   而 `135 × 12 = 1620` 是**平凡丛** `Base × C[A₄]` 的截面维数——另一个对象。
--   两者之间**没有典范映射**：把 `f : T⁶ → Z[ω]` 送到"逐底点的纤维函数"，
--   需要为每条纤维选一个坐标 `F_b ≅ A₄/Stab_b ↪ A₄`（torsor 选择）。
--   故 "891 = 扭转类/幻影方向" 的说法**不成立**，本模块只保留这个差数与它的轨道分解。
--
-- 依据（全部由 `Sov.Math.Hopf` 的测试钉住，不是估计）：
--   纤维大小分布 = {1:27, 4:54, 6:27, 12:27}
--   Σ_b |fiber(b)| = 27·1 + 54·4 + 27·6 + 27·12 = 729 = |T⁶|
--
-- 解释（**定义**，非定理）：把 891 读成"丛的非平凡性在维数上的签名"。
-- 已被验证的是等式 `Σ|fiber| = 729`（`Hopef` 的 `hopf_fiber_sum_729`）与本模块的
-- `135·12 − 729 = 891`；"891 就是扭转类的大小"是一个**待证的解释**，不要在别处当定理引用。
--
-- 零浮点、零 partial。
module Sov.Math.Field
  ( BaseKey, mkBaseKey, baseKeyValue, allBaseKeys
  , Section, mkSection, sectionList, sectionAt
  , baseDim, fiberDim, flatDim, effectiveDim, overcount
  , zeroSection, constantSection, sectionNorm
  , freeOrbitCount, nonFreeOrbitCount
  , minFiberSize, maxFiberSize, uniformCoordsFaithful
  , overcountByOrbit, overcountDecomposition, equivariantSectionPossible
  ) where

import Sov.Math.CarrierA4 (Carrier, dimCarrier, normC2, zeroC)
import Sov.Math.Eis (Eis)
import Sov.Math.Hopf (baseCard, fiberSizeDistribution, fiberSizeOf, orbitReps)

--------------------------------------------------------------------------------
-- 1. 截面：底空间 → 纤维（长度在构造处校验）
--------------------------------------------------------------------------------

-- | 底空间索引（T5）：只有 `mkBaseKey` 能造，越界在构造处被拒
newtype BaseKey = BaseKey Int deriving (Eq, Ord, Show)

mkBaseKey :: Int -> Maybe BaseKey
mkBaseKey k
  | k >= 0 && k < baseDim = Just (BaseKey k)
  | otherwise             = Nothing

baseKeyValue :: BaseKey -> Int
baseKeyValue (BaseKey k) = k

-- | 全部 135 个合法索引
allBaseKeys :: [BaseKey]
allBaseKeys = [ BaseKey k | k <- [0 .. baseDim - 1] ]

newtype Section = Section [Carrier] deriving (Eq, Show)

sectionList :: Section -> [Carrier]
sectionList (Section xs) = xs

-- | 构造截面：必须恰好 `baseDim` 项
mkSection :: [Carrier] -> Maybe Section
mkSection xs
  | length xs == baseDim = Just (Section xs)
  | otherwise            = Nothing

-- | 取某个底点的纤维向量（索引是 `BaseKey`，不可能越界；`Maybe` 只防内部长度失配）
sectionAt :: Section -> BaseKey -> Maybe Carrier
sectionAt (Section xs) (BaseKey b) = atMay xs b
  where
    atMay :: [Carrier] -> Int -> Maybe Carrier
    atMay []       _ = Nothing
    atMay (y : _)  0 = Just y
    atMay (_ : ys) k = atMay ys (k - 1)

zeroSection :: Section
zeroSection = Section (replicate baseDim zeroC)

constantSection :: Carrier -> Section
constantSection c = Section (replicate baseDim c)

-- | 逐点 Plancherel 范数（未归一，= 12·‖·‖²）
sectionNorm :: Section -> BaseKey -> Maybe Eis
sectionNorm s b = fmap normC2 (sectionAt s b)

--------------------------------------------------------------------------------
-- 2. 维数与扭转缺陷
--------------------------------------------------------------------------------

baseDim :: Int
baseDim = baseCard          -- 135

fiberDim :: Int
fiberDim = dimCarrier       -- 12

-- | 若丛可平凡化（每个底点都挂完整 12 维纤维）
flatDim :: Int
flatDim = baseDim * fiberDim   -- 1620

-- | 有效维数：Σ_b |fiber(b)| —— 等于 |T⁶| = 729
effectiveDim :: Int
effectiveDim = sum (map fiberSizeOf orbitReps)

-- | 扭转缺陷 = 平凡化多出来的方向数
overcount :: Int
overcount = flatDim - effectiveDim   -- 891

--------------------------------------------------------------------------------
-- 3. 自由 / 非自由轨道计数
--------------------------------------------------------------------------------

freeOrbitCount :: Int
freeOrbitCount = length [ x | x <- orbitReps, fiberSizeOf x == fiberDim ]

nonFreeOrbitCount :: Int
nonFreeOrbitCount = length orbitReps - freeOrbitCount

--------------------------------------------------------------------------------
-- 4. 统一坐标能否忠实描述整丛
--
-- 若想用「底点 × 12 个纤维坐标」统一描述状态（即把丛平凡化），每条轨道都必须
-- 有 12 个不同点；但最小纤维 = 1（固定点）⇒ **不可能**。
-- 这是 891 个幻影方向的直接来源，也是"无 A₄-等变截面"的离散形式。
--------------------------------------------------------------------------------

minFiberSize, maxFiberSize :: Int
minFiberSize = minimum (map fiberSizeOf orbitReps)   -- 1
maxFiberSize = maximum (map fiberSizeOf orbitReps)   -- 12

-- | 统一 12 坐标是否忠实：需要每条轨道都自由
uniformCoordsFaithful :: Bool
uniformCoordsFaithful = minFiberSize == fiberDim     -- False

--------------------------------------------------------------------------------
-- 5. 扭转缺陷的**结构分解**（不只是减法）
--
--   每个底点缺的坐标数 = 12 − |fiber(b)|，故
--     overcount = Σ_b (12 − |fiber(b)|)
--                  = 27·(12−1) + 54·(12−4) + 27·(12−6) + 27·(12−12)
--                  = 297 + 432 + 162 + 0 = 891
-- 这把"缺陷"落到具体的轨道类型上：谁贡献了多少，一目了然。
--------------------------------------------------------------------------------

-- | [(纤维大小, 轨道条数, 该类贡献的缺陷)]
overcountByOrbit :: [(Int, Int, Int)]
overcountByOrbit =
  [ (s, n, n * (fiberDim - s)) | (s, n) <- fiberSizeDistribution ]

-- | 结构分解是否与 `overcount` 一致（逐点缺失坐标之和 = 总数）
overcountDecomposition :: Bool
overcountDecomposition = sum [ d | (_, _, d) <- overcountByOrbit ] == overcount

--------------------------------------------------------------------------------
-- 6. 等变截面
--
-- 主丛的等变截面要求每条纤维都是自由轨道（12 个不同点），
-- 才能让 g·s(b) 遍历整条纤维。存在 1 点轨道 ⇒ **不存在**等变截面。
-- 这与 `uniformCoordsFaithful = False` 是同一件事的两个说法。
--------------------------------------------------------------------------------

equivariantSectionPossible :: Bool
equivariantSectionPossible = uniformCoordsFaithful   -- False
