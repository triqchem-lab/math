-- | dump-tables — 跨语言全域对账转储器 (与 rust dump_tables 同格式)
--
-- 行格式: 节名,参数...,结果 — 由 python oracle 做三路精确对账。
-- 注意: 本转储是**对账通道**, 不是证明; 精确基线由 oracle 的整数运算给出。
module Main (main) where

import Data.Bits    (xor, shiftL, shiftR)
import Data.List    (intercalate)
import Data.Word    (Word64)
import System.IO

import Sov.Math.Trit
import Sov.Validation.GF9Diag

gf9OfIdx :: Int -> Gf9
gf9OfIdx i = Gf9 (fromTritIndex (fromIntegral (i `div` 3))) (fromTritIndex (fromIntegral (i `mod` 3)))

gf9ToIdx :: Gf9 -> Int
gf9ToIdx (Gf9 x y) = fromIntegral (toTritIndex x) * 3 + fromIntegral (toTritIndex y)

-- GF(9) 乘法: (x+yα)(u+vα) = (xu+2yv) + (xv+yu)α  (α²=2)
gf9Mul :: Gf9 -> Gf9 -> Gf9
gf9Mul (Gf9 x y) (Gf9 u v) =
  Gf9 (addT (mulT x u) (mulT (mulT T2 y) v)) (addT (mulT x v) (mulT y u))

-- 权值 ↔ 环元素 (Z/3^11 规范代表元)
weightOf :: [Trit] -> Word64
weightOf = go 1
  where
    go _  []     = 0
    go w (t : ts) = fromIntegral (toTritIndex t) * w + go (w * 3) ts

ofWeight :: Word64 -> [Trit]
ofWeight v0 = go v0 (11 :: Int)
  where
    go _ 0 = []
    go v k = fromTritIndex (fromIntegral (v `mod` 3)) : go (v `div` 3) (k - 1)

-- 进位加法 (与 Sov.Math.Ring.addR 同语义)
addList :: [Trit] -> [Trit] -> [Trit]
addList = go T0
  where
    go _  []     []     = []
    go c  (x:xs) (y:ys) =
      let s     = addT (addT x y) c
          carry = if toTritIndex x + toTritIndex y + toTritIndex c >= 3 then T1 else T0
      in s : go carry xs ys
    go _  []     _      = []
    go _  _      []     = []

-- 乘法: 位权整数 mod 3^11 (与 Sov.Math.Ring.mulR 同语义)
ringMul :: [Trit] -> [Trit] -> [Trit]
ringMul a b = ofWeight ((weightOf a * weightOf b) `mod` (3 ^ 11))

-- xorshift64 (与 rust dump_tables 同种子同算法)
data Rng = Rng !Word64
rngNext :: Rng -> (Word64, Rng)
rngNext (Rng x0) =
  let x1 = x0 `xor` (x0 `shiftL` 13)
      x2 = x1 `xor` (x1 `shiftR` 7)
      x3 = x2 `xor` (x2 `shiftL` 17)
  in (x3, Rng x3)

rngRange :: Rng -> Word64 -> Word64 -> (Word64, Rng)
rngRange r lo hi = let (n, r') = rngNext r in (lo + n `mod` (hi - lo), r')

line :: String -> [Int] -> Int -> String
line tag params result = tag ++ "," ++ intercalate "," (map show params) ++ "," ++ show result

main :: IO ()
main = do
  hSetBuffering stdout (BlockBuffering Nothing)
  -- GF9MUL: 81 对全枚举
  mapM_ (\(i, j) ->
    putStrLn (line "GF9MUL" [i, j] (gf9ToIdx (gf9Mul (gf9OfIdx i) (gf9OfIdx j)))))
    [(i, j) | i <- [0 .. 8], j <- [0 .. 8]]
  -- GF3ADD / GF3MUL: 9 + 9
  mapM_ (\(a, b) -> putStrLn (line "GF3ADD" [a, b] (fromIntegral (toTritIndex (addT (toEnum a) (toEnum b))))))
        [(a, b) | a <- [0 .. 2], b <- [0 .. 2]]
  mapM_ (\(a, b) -> putStrLn (line "GF3MUL" [a, b] (fromIntegral (toTritIndex (mulT (toEnum a) (toEnum b))))))
        [(a, b) | a <- [0 .. 2], b <- [0 .. 2]]
  -- RINGADD2X: 全部 177147 元素 v+v (加法双倍全域)
  mapM_ (\v ->
    let x = ofWeight (fromIntegral v)
    in putStrLn (line "RINGADD2X" [fromIntegral v] (fromIntegral (weightOf (addList x x)))))
    [0 :: Int .. 177146]
  -- RINGMULGEN: 全部 v × 3^k (乘法生成元全域)
  mapM_ (\(v, k) ->
    let x = ofWeight (fromIntegral v)
        g = ofWeight (3 ^ (k :: Int))
    in putStrLn (line "RINGMULGEN" [v, k] (fromIntegral (weightOf (ringMul x g)))))
    [(v, k) | v <- [0 :: Int .. 177146], k <- [0 .. 10]]
  -- RINGMULSAMPLE: 种子化 1e6 对 (与 rust 同种子)
  let goSample :: Int -> Rng -> IO ()
      goSample 0 _ = return ()
      goSample n r = do
        let (a, r1) = rngRange r 0 177147
            (b, r2) = rngRange r1 0 177147
        putStrLn (line "RINGMULSAMPLE" [fromIntegral a, fromIntegral b]
                    (fromIntegral (weightOf (ringMul (ofWeight a) (ofWeight b)))))
        goSample (n - 1 :: Int) r2
  goSample 1000000 (Rng 0x9E3779B97F4A7C15)
