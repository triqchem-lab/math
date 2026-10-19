-- | Sov.Math.Adc — L0 二进制 ADC 进位链 (镜像 rust l0_adc.rs / C++ adc_limb.h)
--
-- Uint384 = 6 × 64 位 limb (小端序), 定长向量编码: limb 数错误无法编译。
-- LcmAccumulator 为纯函数式状态: step / zhonglvClosure 返回新累加器。
--
-- 宪法: acc = (acc × 黄钟 + delta) mod LCM_TOTAL; 仲吕闭合 = (acc × 黄钟) >> 16。
module Sov.Math.Adc
  ( Uint384
  , u384Zero
  , u384FromU64
  , u384Limbs
  , u384Add
  , u384MulU64
  , u384Shr
  , LcmAccumulator(..)
  , newAcc
  , accStep
  , zhonglvClosure
  , layer1Value
  ) where

import Data.Bits   ((.&.))

import Data.Word   (Word64)
import Sov.Math.Constants
import Sov.Math.Nat
import Sov.Math.Vec

-- | 384 位无符号整数: 6 × 64 位 limb, 小端 (索引 0 = 最低位)
type Uint384 = Vec N6 Word64

u384Zero :: Uint384
u384Zero = replicateV n6 0

u384FromU64 :: Word64 -> Uint384
u384FromU64 v = v :> replicateV n5 0

u384Limbs :: Uint384 -> [Word64]
u384Limbs = toList

-- 多 limb 加法 (ADC 进位链): 用 Integer 中间量避免溢出
u384Add :: Uint384 -> Uint384 -> Uint384
u384Add = go 0
  where
    go :: Word64 -> Vec n Word64 -> Vec n Word64 -> Vec n Word64
    go _  VNil      VNil      = VNil
    go c  (x :> xs) (y :> ys) =
      let s   = toInteger x + toInteger y + toInteger c
          low = fromInteger (s .&. 0xFFFFFFFFFFFFFFFF)
          hi  = fromInteger (s `div` 2 ^ (64 :: Int)) :: Word64
      in low :> go hi xs ys

-- 多 limb × u64 标量
u384MulU64 :: Uint384 -> Word64 -> Uint384
u384MulU64 v scalar = go 0 v
  where
    go :: Word64 -> Vec n Word64 -> Vec n Word64
    go _ VNil      = VNil
    go c (x :> xs) =
      let p   = toInteger x * toInteger scalar + toInteger c
          low = fromInteger (p .&. 0xFFFFFFFFFFFFFFFF)
          hi  = fromInteger (p `div` 2 ^ (64 :: Int)) :: Word64
      in low :> go hi xs

-- 右移 shift 位 (shift ≥ 384 → 0)
-- 右移: 整体值 div 2^shift (shift' = 0 时对齐 rust bit_shift == 0 分支)
u384Shr :: Uint384 -> Int -> Uint384
u384Shr v shift' = intToLimbs n6 (limbsToInteger v `div` (2 ^ shift'))
  where
    limbsToInteger :: Vec n Word64 -> Integer
    limbsToInteger = foldrV (\w acc -> toInteger w + acc * 2 ^ (64 :: Int)) 0
    intToLimbs :: SNat m -> Integer -> Vec m Word64
    intToLimbs SZ     _ = VNil
    intToLimbs (SS n) i =
      fromInteger (i .&. 0xFFFFFFFFFFFFFFFF) :> intToLimbs n (i `div` 2 ^ (64 :: Int))

-- ═════════════════════════════════════════════════
-- LCM 累加器 — 层1↔层2 桥接状态 (纯函数式)
-- ═════════════════════════════════════════════════

data LcmAccumulator = LcmAccumulator
  { accValue     :: Uint384
  , accStepCount :: Int
  , accChernQ16  :: Int  -- Q16.16 定点 (整型, 禁浮点)
  } deriving (Eq, Show)

newAcc :: LcmAccumulator
newAcc = LcmAccumulator
  { accValue     = u384Zero
  , accStepCount = 0
  , accChernQ16  = fromIntegral chernTargetQ16
  }

-- 步进: acc = (acc × 黄钟 + delta) mod LCM_TOTAL
-- 镜像 rust: 仅对 limb[0] 做 mod/carry, Word64 加法自然回绕 (与 rust release 一致)
accStep :: Word64 -> LcmAccumulator -> LcmAccumulator
accStep delta acc =
  let m      = u384MulU64 (accValue acc) huangzhong
      total  = headV m + delta                  -- limb[0] += delta (回绕语义同 rust)
      modVal = total `mod` lcmTotal
      carry  = total `div` lcmTotal
      -- 上位 limb 加 carry: 借一位 0 作 limb[0], 加完后取尾 (镜像 rust limbs[1] += carry)
      bumped5 = tailV (u384Add (0 :> tailV m) (u384FromU64 carry))
  in acc { accValue     = modVal :> bumped5
         , accStepCount = accStepCount acc + 1
         }

-- 仲吕闭合: acc = (acc × 黄钟) >> 16, 步数归零
zhonglvClosure :: LcmAccumulator -> LcmAccumulator
zhonglvClosure acc =
  acc { accValue     = u384Shr (u384MulU64 (accValue acc) huangzhong) zhonglvShift
      , accStepCount = 0
      }

-- 层1截断值 (低 64 位)
layer1Value :: LcmAccumulator -> Word64
layer1Value = headV . accValue
