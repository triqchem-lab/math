-- | Sov.Math.Trit — L1 GF(3) 有限域 (镜像 rust l1_field.rs / types.rs::Trit)
--
-- 强类型化核心: Trit 是 3 构造子枚举, {0,1,2} 之外的值在类型层不可表达。
-- Rust 侧 from_u8 的 runtime panic 在此被彻底消灭 — 没有 "非法 trit" 这回事。
--
-- 宪法:
--   C3 顺时针  c3cw  : (t+1) mod 3   正四面体 C3 轴 120° 旋转
--   C3 逆时针  c3ccw : (t+2) mod 3   顺转两步, 非"减法"
--   乘法      mul   : T₂⊗T₂ = T₁    (2×2 ≡ 1 mod 3)
--   范数      normSq: |0|=0, |1|=|2|=1
--   损 sun = c3ccw, 益 yi = c3cw (宪法级损益, 互逆)
module Sov.Math.Trit
  ( Trit(..)
  , allTrits
  , toTritIndex    -- 对齐 C/Rust 边界的投影: Trit → {0,1,2}
  , fromTritIndex  -- 总函数: 模 3 归约, 不可能失败
  , t0, t1, t2
  , c3cw, c3ccw
  , mulT
  , normSqT
  , sun, yi
  , addT
  , negT
  ) where

import Data.Word (Word8)

-- | GF(3) 三值 — 只有这三个构造子
data Trit = T0 | T1 | T2
  deriving (Eq, Ord, Show, Enum, Bounded)

allTrits :: [Trit]
allTrits = [minBound .. maxBound]

-- | 投影到 {0,1,2} (跨语言 FFI 边界用; 库内一律保持 Trit 类型)
toTritIndex :: Trit -> Word8
toTritIndex T0 = 0
toTritIndex T1 = 1
toTritIndex T2 = 2

-- | 总函数: 任意 Word8 模 3 归约 — 不会 panic
fromTritIndex :: Word8 -> Trit
fromTritIndex v = toEnum (fromIntegral (v `mod` 3))

t0, t1, t2 :: Trit
t0 = T0
t1 = T1
t2 = T2

-- C3 顺时针: 正四面体 C3 轴 120° 旋转
c3cw :: Trit -> Trit
c3cw T0 = T1
c3cw T1 = T2
c3cw T2 = T0

-- C3 逆时针: 顺转两步, 非"减法"
c3ccw :: Trit -> Trit
c3ccw = c3cw . c3cw

-- GF(3) 乘法: (a×b) mod 3, 宪法 T₂⊗T₂=T₁
mulT :: Trit -> Trit -> Trit
mulT T0 _  = T0
mulT _  T0 = T0
mulT T1  b = b
mulT a  T1 = a
mulT T2 T2 = T1

-- 范数平方: |0|=0, |1|=|2|=1
normSqT :: Trit -> Trit
normSqT T0 = T0
normSqT _  = T1

-- 宪法级损: T₂→T₁, T₁→T₀ (吸收), T₀→T₂
sun :: Trit -> Trit
sun = c3ccw

-- 宪法级益: (t+1) mod 3
yi :: Trit -> Trit
yi = c3cw

-- GF(3) 加法: (a+b) mod 3 (C3 旋转的复合)
addT :: Trit -> Trit -> Trit
addT a b = fromTritIndex (toTritIndex a + toTritIndex b)

-- GF(3) 加法逆: -a (a + (-a) = 0)
negT :: Trit -> Trit
negT T0 = T0
negT T1 = T2
negT T2 = T1
