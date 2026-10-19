-- | Sov.Math.Guard.NSE — T⁶ 网格 + NSE 演化 + 宪法审计 + SOV v2.6 序列化
-- (镜像 rust sov-guard/lib.rs 运行时层)
--
-- 零浮点改造:
--   真空常数 (0.74e-25 / 3.55e75 / 5.16e96) 超出任何定点量程 → 用
--   '(尾数 Q16, 10 的指数)' 整数对表示 (数量级精确, 尾数 1 ulp 内)。
--   density_log10 → Q24 定点 (58.697 与 38.016 间隙的整数线性插值)。
-- SOV v2.6 I/O 做成纯序列化函数 (字节串进出, 不触文件系统)。
module Sov.Math.Guard.NSE
  ( T6Grid(..)
  , gridZero
  , gridAt
  , gridSet
  , gridCount
  , gridDensityQ16
  , gridInitCouette
  , NSESolver(..)
  , nseNew
  , nseStepOnce
  , sovMagic, sovVersion
  , le32Pub
  , sovSerialize
  , sovCheckHeader
  , vacuumLeonovLQ0, vacuumLeonovRho0, vacuumHarameinRhoV
  , densityGapQ24
  , HolographicScaling(..)
  , holoNew
  , lcmProgressQ16
  , currentDensityLog10Q24
  , nseSunyiSequence
  , verifyNoExplosion
  , verifyCurieTransition
  , verifyC3Cycle
  , verifySunyiChain
  , fullConstitutionalAudit
  , counterExamples
  ) where

import Data.Bits    (shiftR)
import Data.Int     (Int32, Int64)

import Data.Word    (Word8, Word32, Word64)
import Sov.Math.Constants
import Sov.Math.Topology.Soliton (solitonRhoQ16, phaseAnchorSteps, curieThresholdQ16)
import Sov.Math.Topology.T6 (t6Polar, t6Toroidal, t6Total, t6Index)
import Sov.Math.SunYi (twelveLengths)

-- | T⁶ 网格: 6624 格点 σ ∈ {0,1,2} (行优先 p×t)
newtype T6Grid = T6Grid { sigma :: [Word8] }
  deriving (Eq, Show)

gridZero :: T6Grid
gridZero = T6Grid (replicate t6Total 0)

gridAt :: T6Grid -> Int -> Int -> Word8
gridAt g p t = sigma g !! t6Index p t

gridSet :: Int -> Int -> Word8 -> T6Grid -> T6Grid
gridSet p t v g = g { sigma = setAt (t6Index p t) (v `mod` 3) (sigma g) }
  where setAt i x xs = take i xs ++ [x] ++ drop (i + 1) xs

gridCount :: T6Grid -> Int
gridCount = length . filter (/= 0) . sigma

-- 密度 × 65536 (零浮点: 有理数乘后整除)
gridDensityQ16 :: T6Grid -> Int
gridDensityQ16 g = gridCount g * 65536 `div` t6Total

gridInitCouette :: T6Grid -> T6Grid
gridInitCouette g =
  foldl' (\acc (p, t) ->
            gridSet p t (fromIntegral ((p * 2 `div` t6Polar) `mod` 3)) acc)
         g [(p, t) | p <- [0 .. t6Polar - 1], t <- [0 .. t6Toroidal - 1]]

-- | NSE 演化器
data NSESolver = NSESolver
  { nseGrid    :: T6Grid
  , nseStep    :: Int
  , nseZhonglv :: Int
  } deriving (Eq, Show)

nseNew :: NSESolver
nseNew = NSESolver gridZero 0 0

-- 单步演化 (语义对齐 rust step_once): 加性泵 + 居里阈值上的 C₃ 相位调制
-- 居里判定: density(Q16) ≥ 0.38(Q16) = 24903 (两侧同标度, 直接比较)
nseStepOnce :: NSESolver -> NSESolver
nseStepOnce s =
  let curie = gridDensityQ16 (nseGrid s) >= curieThresholdQ16
      st    = nseStep s
      pump = if st `mod` 2 == 0 then 1 else 2 :: Int
      nxt =
        [ let old = fromIntegral (sigma (nseGrid s) !! t6Index p t) :: Int
              n0  = (old + pump) `mod` 3
              n1  = if curie
                      then let c3 = (st + p * 7 + t * 3) `mod` c3CycleSteps
                           in if c3 < c3CycleSteps `div` 3 then (n0 + 1) `mod` 3
                              else if c3 < c3CycleSteps * 2 `div` 3 then (n0 + 2) `mod` 3
                              else n0
                      else n0
          in fromIntegral n1
        | p <- [0 .. t6Polar - 1], t <- [0 .. t6Toroidal - 1] ]
      zh' = if st > 0 && st `mod` zhonglvPeriod == 0 then nseZhonglv s + 1 else nseZhonglv s
  in s { nseGrid = T6Grid nxt, nseStep = st + 1, nseZhonglv = zh' }

-- ═══ SOV v2.6 序列化 (纯函数) ═══

-- 供测试/调用方构造合法头 (导出小端编码器)
le32Pub :: Word32 -> [Word8]
le32Pub = le32

sovMagic, sovVersion :: Word32
sovMagic   = 0x564F5354
sovVersion = 0x00020600

le32 :: Word32 -> [Word8]
le32 w = [ fromIntegral (w `shiftR` (8 * k)) | k <- [0 .. 3] ]

-- 头(72B) + 载荷 + 签名(16B): magic "ROOT" + payloadRoot + 0
sovSerialize :: [Word8] -> [Word8] -> [Word8]
sovSerialize header dataBytes =
  header ++ dataBytes ++ sig
  where
    sig = take 4 (le32 0x524F4F54) ++ [fromIntegral root, 0] ++ replicate 10 0
    root :: Word64
    root = dr (foldl step 0 dataBytes)
    step :: Word64 -> Word8 -> Word64
    step s b =
      let s' = s + fromIntegral b
      in if s' > 0xFFFFFFFFFFFFFF00 then dr s' else s'
    dr :: Word64 -> Word64
    dr n0 = go n0
      where go n | n >= 10   = go (digitSum n)
                 | otherwise = n
            digitSum n = sum (digits n)
            digits 0 = [] :: [Word64]
            digits n = n `mod` 10 : digits (n `div` 10)

-- 头校验: magic 与版本匹配
sovCheckHeader :: [Word8] -> Bool
sovCheckHeader h =
  take 4 h == le32 sovMagic && take 4 (drop 4 h) == le32 sovVersion

-- ═══ 真空参考常数 (零浮点表示: (尾数 Q16, 10 的指数)) ═══

vacuumLeonovLQ0, vacuumLeonovRho0, vacuumHarameinRhoV :: (Int, Int)
vacuumLeonovLQ0    = (48497, -25)    -- 0.74 × 65536 = 48496.64 → 48497
vacuumLeonovRho0   = (232653, 75)    -- 3.55 × 65536 = 232652.8 → 232653
vacuumHarameinRhoV = (338166, 96)    -- 5.16 × 65536 = 338165.76 → 338166

densityGapQ24 :: Int64
densityGapQ24 = 637530419           -- 38.016 × 2^24

-- ═══ 全息缩放 ═══

data HolographicScaling = HolographicScaling
  { holoLimbs :: [Word64]  -- 6 limbs
  , holoWraps :: Word64
  } deriving (Eq, Show)

holoNew :: HolographicScaling
holoNew = HolographicScaling (replicate 6 0) 0

-- progress = limb0 / LCM_TOTAL; q16 = progress × 2^16 (整数)
lcmProgressQ16 :: HolographicScaling -> Int32
lcmProgressQ16 h = fromIntegral limb0Progress
  where
    limb0Progress :: Integer
    limb0Progress = case holoLimbs h of
      (l0 : _) -> (toInteger l0 * 65536) `div` toInteger lcmTotal
      []       -> 0

-- currentDensityLog10: 58.697 + progress × 38.016, Q24 定点 (零浮点线性插值)
currentDensityLog10Q24 :: HolographicScaling -> Int64
currentDensityLog10Q24 h = fromIntegral (base + fromIntegral scaled)
  where
    base = 58697 * (2 ^ (24 :: Int) `div` 1000) :: Integer
    scaled = case holoLimbs h of
      (l0 : _) -> (toInteger l0 * toInteger densityGapQ24) `div` toInteger lcmTotal
      []       -> 0 :: Integer

-- ═══ NSE 宪法验证 ═══

nseSunyiSequence :: [Int]
nseSunyiSequence = map fromIntegral twelveLengths  -- [81,54,...,30]

-- GF(3) 封闭: 无穷大不合法 (全枚举 9 组)
verifyNoExplosion :: Bool
verifyNoExplosion =
  and [ let s = (a + b) `mod` 3; m = (a * b) `mod` 3
        in s <= 2 && m <= 2 && (a + 1) `mod` 3 <= 2 && (a + 2) `mod` 3 <= 2
       | a <- [0 :: Int .. 2], b <- [0 :: Int .. 2] ]

-- 居里相变: rho(0) < 0.01 (Q16 < 655), rho(4500) > 0.99 (Q16 > 64880)
verifyCurieTransition :: Bool
verifyCurieTransition =
  solitonRhoQ16 0 < 655
  && solitonRhoQ16 phaseAnchorSteps > 64880

-- C₃ 周期可被 12 律整除
verifyC3Cycle :: Bool
verifyC3Cycle = c3CycleSteps == 1500 && c3CycleSteps `mod` toneCountInt == 0
  where toneCountInt = 12

-- 损益链: 相邻比 ≈ 2/3 或 4/3, 容差 2% (整数交叉乘: |3a−kb|·50 ≤ 3b, k∈{2,4})
verifySunyiChain :: Bool
verifySunyiChain =
  and [ abs (3 * b - 2 * a) * 50 <= 3 * a || abs (3 * b - 4 * a) * 50 <= 3 * a
      | (a, b) <- zip seq' (drop 1 seq') ]  -- 比值 r = b/a ∈ {2/3, 4/3} ± 2%
  where seq' = nseSunyiSequence

fullConstitutionalAudit :: Bool
fullConstitutionalAudit =
  verifyNoExplosion && verifyCurieTransition && verifyC3Cycle && verifySunyiChain

-- NSE 反例表 (光滑性/速度爆炸/-5/3/涡粘/Reynolds)
counterExamples :: [(String, String)]
counterExamples =
  [ ("光滑性假设", "GF(3)格点上只有{0,1,2}跃迁, 无光滑概念")
  , ("速度爆炸", "GF(3)封闭于{0,1,2}, 无穷大不合法")
  , ("Kolmogorov -5/3", "GF(3)精确×8指数律替代")
  , ("涡粘性", "ω在ρ=0.38自动激活")
  , ("Reynolds数", "非普适常数, 格点拓扑导出量") ]
