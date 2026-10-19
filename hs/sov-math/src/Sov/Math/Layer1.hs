-- | Sov.Math.Layer1 — L1 5-trit 打包/批量运算 (镜像 rust l1_pack.rs / C++ gf3_layer1.h)
--
-- pack5: 5 个 GF(3) trit → 1 字节 base-3 值 [0,242] — 参数恰为 5 个 Trit
-- (类型层强制, 无 arity 错误)。
-- unpack5: 1 字节 → 恰 5 个 trit (高位优先: out₀ = t₄ … out₄ = t₀)。
module Sov.Math.Layer1
  ( pack5
  , unpack5
  , packBatch
  , unpackBatch
  , layer1Add
  , layer1Dot
  , layer1NormSq
  ) where

import Data.Word  (Word8)
import Sov.Math.Trit

-- 5 个 trit → byte (base-3, 值域 [0,242] ⊂ Word8)
pack5 :: Trit -> Trit -> Trit -> Trit -> Trit -> Word8
pack5 a b c d e =
  fromIntegral $ toI a + 3 * toI b + 9 * toI c + 27 * toI d + 81 * toI e
  where toI = fromIntegral . toTritIndex :: Trit -> Int

-- byte → 5 trit (高位优先: head = t₄)
unpack5 :: Word8 -> (Trit, Trit, Trit, Trit, Trit)
unpack5 p =
  ( g 81
  , g 27
  , g 9
  , g 3
  , g 1 )
  where
    v  = fromIntegral p :: Int
    g :: Int -> Trit
    g w = fromTritIndex (fromIntegral ((v `div` w) `mod` 3))

-- 批量打包: n trit → ceil(n/5) 字节 (末组补 T0)
packBatch :: [Trit] -> [Word8]
packBatch ts = go ts
  where
    go (a : b : c : d : e : rest) = pack5 a b c d e : go rest
    go rest = [padPack rest]
    padPack (a : b : c : d : _) = pack5 a b c d T0
    padPack [a, b, c]           = pack5 a b c T0 T0
    padPack [a, b]              = pack5 a b T0 T0 T0
    padPack [a]                 = pack5 a T0 T0 T0 T0
    padPack []                  = 0

-- 批量解包: 取前 n 个 trit
unpackBatch :: [Word8] -> Int -> [Trit]
unpackBatch packed n = take n (concatMap expand packed)
  where
    expand w = let (h4, h3, h2, h1, h0) = unpack5 w
               in [h0, h1, h2, h3, h4]  -- 还原原始 trit 序 (t₀..t₄, 批量 roundtrip 语义)

-- GF(3) 逐 trit 加法: (a+b) mod 3 (截断到短侧长度, 对齐 rust)
layer1Add :: [Trit] -> [Trit] -> [Trit]
layer1Add = zipWith addT

-- GF(3) 点积: Σ aᵢ×bᵢ (整数累加, 对齐 rust layer1_dot — 非模 3)
layer1Dot :: [Trit] -> [Trit] -> Int
layer1Dot a b = sum [ fromIntegral (toTritIndex x) * fromIntegral (toTritIndex y) :: Int | (x, y) <- zip a b ]

-- GF(3) 范数平方: Σ [aᵢ ≠ 0]
layer1NormSq :: [Trit] -> Int
layer1NormSq = length . filter (/= T0)
