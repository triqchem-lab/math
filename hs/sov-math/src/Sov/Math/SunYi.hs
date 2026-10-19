-- | Sov.Math.SunYi — 损益链·十二律 (镜像 rust sunyi.rs / C++ loss_gain.h)
--
-- 强类型化: 十二律用 12 构造子枚举 — 不存在"第 13 律";
-- 律的步进在类型层封闭 (enum 循环), 无数组越界。
--
-- 宪法: 仲吕不能自生黄钟 (yi 30 ≠ 81)。
module Sov.Math.SunYi
  ( ToneName(..)
  , allTones
  , toneIndex
  , toneFromIndexMay
  , nextTone
  , twelveLengths
  , toneLength
  , lcmRemainder
  , LossGain(..)
  , lossGainSequence
  , stepLossGain
  , canSun, sunOp
  , canYi,  yiOp
  , isZhonglvBoundary
  ) where

import Data.Word (Word64)


-- | 十二律 — 12 构造子枚举, 顺序 = 损益链生成顺序
data ToneName
  = HuangZhong  -- 黄钟 81
  | LinZhong    -- 林钟 54
  | TaiCu       -- 太簇 72
  | NanLv       -- 南吕 48
  | GuXian      -- 姑洗 64
  | YingZhong   -- 应钟 43
  | RuiBin      -- 蕤宾 57
  | DaLv        -- 大吕 38
  | YiZe        -- 夷则 51
  | JiaZhong    -- 夹钟 34
  | WuYi        -- 无射 45
  | ZhongLv     -- 仲吕 30
  deriving (Eq, Ord, Show, Enum, Bounded)

allTones :: [ToneName]
allTones = [minBound .. maxBound]

toneIndex :: ToneName -> Int
toneIndex = fromEnum

-- 运行期索引 → 律 (越界 Nothing)
toneFromIndexMay :: Int -> Maybe ToneName
toneFromIndexMay i
  | i >= 0 && i < 12 = Just (toEnum i)
  | otherwise        = Nothing

-- PHASE_NEXT: [1,2,3,…,11,0] — enum 后继的封闭循环
nextTone :: ToneName -> ToneName
nextTone ZhongLv  = HuangZhong
nextTone t        = succ t

-- 三分损益长度表 (TWELVE_LENGTHS)
twelveLengths :: [Word64]
twelveLengths = [81,54,72,48,64,43,57,38,51,34,45,30]

toneLength :: ToneName -> Word64
toneLength t = twelveLengths !! toneIndex t

-- LCM 余数表 (LCM_REMAINDERS)
lcmRemainder :: ToneName -> Word64
lcmRemainder t = [177147,118098,157464,104976,139968,93312,124416,82944,110592,73728,98304,65536] !! toneIndex t

-- 损 / 益
data LossGain = Sun | Yi
  deriving (Eq, Show)

-- 宪法固定损益序列 (最后两步连损)
lossGainSequence :: [LossGain]
lossGainSequence =
  [Sun,Yi,Sun,Yi,Sun,Yi,Sun,Yi,Sun,Yi,Sun,Sun]

stepLossGain :: Word64 -> LossGain
stepLossGain s = lossGainSequence !! (fromIntegral s `mod` 12)

-- 宪法级损: n → 2n/3 (可损判定: 2n ≡ 0 mod 3)
canSun :: Word64 -> Bool
canSun n = (n * 2) `mod` 3 == 0

sunOp :: Word64 -> Word64
sunOp n = (n * 2) `div` 3

-- 宪法级益: n → 4n/3 (可益判定: 4n ≡ 0 mod 3)
canYi :: Word64 -> Bool
canYi n = (n * 4) `mod` 3 == 0

yiOp :: Word64 -> Word64
yiOp n = (n * 4) `div` 3

-- 仲吕边界: 第 11 步
isZhonglvBoundary :: Word64 -> Bool
isZhonglvBoundary s = s `mod` 12 == 11
