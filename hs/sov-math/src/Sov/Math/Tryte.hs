-- | Sov.Math.Tryte — 6 位基 3 数 (镜像 rust types.rs::TryteValue)
--
-- 强类型化: Tryte 直接表示为 Vec N6 Trit — 值域 [0,729) 由构造天然保证,
-- Rust 侧 TryteOverflow 错误在此类型层不可发生。
-- 纳音标签 (mod 60) 与十二律 (mod 12) 是投影, 不是存储。
module Sov.Math.Tryte
  ( Tryte
  , tryteZero
  , mkTryte          -- 从 Word16 构造 (超范围 → Maybe)
  , tryteValue       -- 投影: [0,729)
  , nayinLabel
  , semitone
  , tryteFromVec
  , tryteToVec
  ) where

import Data.Word  (Word16)
import Sov.Math.Nat
import Sov.Math.Trit
import Sov.Math.Vec

-- | Tryte = 6 个 GF(3) trit, 值 = Σ dᵢ·3ⁱ ∈ [0, 729)
newtype Tryte = Tryte { unTryte :: Vec N6 Trit }
  deriving (Eq, Show)

tryteZero :: Tryte
tryteZero = Tryte (replicateV n6 T0)

-- 外部边界构造: 超出 [0,729) 返回 Nothing (内部构造不需要检查)
mkTryte :: Word16 -> Maybe Tryte
mkTryte v
  | v < 729   = Just (Tryte (encodeVec v))
  | otherwise = Nothing
  where
    encodeVec :: Word16 -> Vec N6 Trit
    encodeVec = go n6
      where
        go :: SNat m -> Word16 -> Vec m Trit
        go SZ     _ = VNil
        go (SS n) w = fromTritIndex (fromIntegral (w `mod` 3)) :> go n (w `div` 3)

-- | 投影: 数值 [0,729)
tryteValue :: Tryte -> Word16
tryteValue (Tryte v) = go 0 1 (toList v)
  where
    go acc _  []     = acc
    go acc w (t : ts) = go (acc + fromIntegral (toTritIndex t) * w) (w * 3) ts

-- | 纳音标签: mod 60
nayinLabel :: Tryte -> Word16
nayinLabel t = tryteValue t `mod` 60

-- | 十二律: mod 12
semitone :: Tryte -> Word16
semitone t = tryteValue t `mod` 12

tryteFromVec :: Vec N6 Trit -> Tryte
tryteFromVec = Tryte

tryteToVec :: Tryte -> Vec N6 Trit
tryteToVec = unTryte
