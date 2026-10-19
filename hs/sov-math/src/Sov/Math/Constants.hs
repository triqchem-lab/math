-- | Sov.Math.Constants — 宪法常数 (镜像 rust constants.rs / C++ lcm_constants.h)
--
-- 所有层的不可变常数集中定义。全整数, 零浮点。
-- 类型级对应: HUANGZHONG = 3^11, ZHONGLV_BOUNDARY = 2^16。
module Sov.Math.Constants
  ( huangzhong
  , zhonglvShift
  , zhonglvBoundary
  , lcmTotal
  , binaryLimbBits
  , adcLimbCount
  , tritsPerByte
  , tritsPerTryte
  , tryteMaxValue
  , microPump
  , midPump
  , grandPump
  , polarWinding
  , toroidalWinding
  , chernTarget
  , chernTargetQ16
  , deltaQ16
  , chiralCollapse
  , sqrt3Q16Floor
  , nayinCount
  , toneCount
  , wuxingCount
  , c3CycleSteps
  , zhonglvPeriod
  , chernGuardTolerance
  ) where

import Data.Int   (Int32)
import Data.Word  (Word64)

-- 黄钟 = 3^11, LCM 桥乘数
huangzhong :: Word64
huangzhong = 177147

-- 仲吕边界 = 2^16, 二进制截断宽度
zhonglvShift :: Int
zhonglvShift = 16

zhonglvBoundary :: Word64
zhonglvBoundary = 65536

-- LCM = 3^11 × 2^16
lcmTotal :: Word64
lcmTotal = huangzhong * zhonglvBoundary  -- 11609505792

-- ADC limb 配置 (uint384 = 6 × 64 位)
binaryLimbBits :: Int
binaryLimbBits = 64

adcLimbCount :: Int
adcLimbCount = 6

-- GF(3) 编码
tritsPerByte :: Int
tritsPerByte = 5

tritsPerTryte :: Int
tritsPerTryte = 6

tryteMaxValue :: Word64
tryteMaxValue = 729  -- 3^6

-- 泵周期
microPump, midPump, grandPump :: Int
microPump = 12
midPump   = 96
grandPump = 6624

-- T⁶ 环面
polarWinding, toroidalWinding :: Int
polarWinding    = 144
toroidalWinding = 46

-- 陈数 (与 C++ lcm_constants.h int CHERN_TARGET=2 对齐, 整型禁浮点)
chernTarget :: Int32
chernTarget = 2

chernTargetQ16 :: Int32
chernTargetQ16 = 131072  -- 2.0 × 65536

-- ⚠ 来源: 实验物理锚定 (H₂O@C₆₀ 光谱能级分裂 ≈0.5 meV), 非代数推导
-- wiki CHIRAL_COLLAPSE = 113507; SQRT3_Q16_FLOOR = floor(√3 × 65536) 数学精确截断
deltaQ16 :: Int32
deltaQ16 = 113506

chiralCollapse :: Int32
chiralCollapse = 113507

sqrt3Q16Floor :: Int32
sqrt3Q16Floor = 113511

-- 纳音 / 十二律 / 五行
nayinCount, toneCount, wuxingCount :: Int
nayinCount  = 60
toneCount   = 12
wuxingCount = 5

-- C3 孤子 (L5)
c3CycleSteps :: Int
c3CycleSteps = 1500

-- 仲吕 (L6)
zhonglvPeriod :: Int
zhonglvPeriod = 12

-- 陈数守卫容差 (bridge.rs: ±655)
chernGuardTolerance :: Int32
chernGuardTolerance = 655
