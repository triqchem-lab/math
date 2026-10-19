-- | Sov.Math.L2Ops — L2 向量/矩阵运算 + A4 翻转 (镜像 rust l2_ops.rs / l2_matmul.rs / vavx3.rs 纯语义)
--
-- Haskell 侧为纯参考后端 (rust 侧 VAVX3 是 SIMD 硬件抽象, 语义以 vavx3 纯分支为准):
--   matmul: t = Σ GF3_MUL[xⱼ][wⱼ] (trit 乘积整数累加) → (t mod 3)
--   √3 门控: (nz << 16) > threshold_q16 → 1, 否则 0
--   门控乘: r = (gate × x) mod 3
--   rmsNorm: 预计算 LUT 注入 (rust 同签名; Q16.rsqrt 表零浮点预生成)
--
-- 强类型化: 向量长度误差用类型对齐 (zipWith 截断到短侧, 对齐 rust .min(len));
-- A4Op 是 4 构造子枚举 — 不存在的翻转操作不可表达。
module Sov.Math.L2Ops
  ( matmulGF3
  , gateSqrt3
  , gatedMul
  , rmsNorm
  , ringVectorAdd
  , ringDotProduct
  , A4Op(..)
  , a4FlipTriple
  ) where

import Data.Bits  (shiftL)
import Data.Int   (Int32)
import Sov.Math.Ring
import Sov.Math.Trit

-- GF(3) 矩阵乘法: x = n×in_dim (行优先), w = out_dim×in_dim → n×out_dim
-- 语义对齐 vavx3::gf3_matmul: trit 乘积整数累加后 mod 3
matmulGF3 :: [[Trit]] -> [[Trit]] -> [[Trit]]
matmulGF3 x w = [ [ sumMod3 [ mulI xj wj | (xj, wj) <- zip row wRow ] | wRow <- w ]
                | row <- x ]
  where
    sumMod3 :: [Int] -> Trit
    sumMod3 ts = fromTritIndex (fromIntegral (sum ts `mod` 3))
    mulI a b = fromIntegral (toTritIndex a) * fromIntegral (toTritIndex b) :: Int

-- √3 门控: 每行非零计数 nz, (nz << 16) > threshold → T1 否则 T0
gateSqrt3 :: [[Trit]] -> Int32 -> [Trit]
gateSqrt3 x threshold =
  [ if (length (filter (/= T0) (take dim row)) `shiftL` 16) > fromIntegral threshold
      then T1 else T0
  | row <- x ]
  where dim = case x of { (r : _) -> length r; [] -> 0 }

-- 门控乘: rᵢ = (gateᵢ/𝑑 × xᵢ) mod 3
gatedMul :: [Trit] -> [Trit] -> Int -> [Trit]
gatedMul gate x dim =
  [ mulT (gate !! (i `div` dim)) xi | (i, xi) <- zip [0 ..] x ]

-- RMSNorm: Q16.rsqrt 表外部注入 (零浮点; rust 同签名 rsqrt_lut 参数)
-- 语义对齐 z3r_layer2.h: p = GF3_MUL[x][gamma]; out = ((p·rs + 32768) >> 16) mod 3
rmsNorm :: [Trit] -> [Trit] -> [Int32] -> Int -> [Trit]
rmsNorm x gamma rsqrtLut dim = concatMap normRow rows
  where
    rows = chunk dim x
    normRow row =
      let nz = length (filter (/= T0) row)
          rs = rsqrtLut !! min nz (length rsqrtLut - 1)
      in [ fromTritIndex (fromIntegral (((p * fromIntegral rs + 32768) `div` 65536) `mod` 3))
         | (xi, gi) <- zip row (cycle gamma)
         , let p = toIntegral (mulT xi gi) ]
    toIntegral = fromIntegral . toTritIndex :: Trit -> Int

chunk :: Int -> [a] -> [[a]]
chunk _ [] = []
chunk k xs = take k xs : chunk k (drop k xs)

-- 环元素向量加法 (l2_ops::vector_add)
ringVectorAdd :: [RingElement] -> [RingElement] -> [RingElement]
ringVectorAdd = zipWith addR

-- 环元素点积 (l2_ops::dot_product)
ringDotProduct :: [RingElement] -> [RingElement] -> RingElement
ringDotProduct a b = foldl (\acc (xi, yj) -> addR acc (mulR xi yj)) ringZero (zip a b)

-- A4 四面体翻转操作 (l2_matmul::A4Op)
data A4Op = C3Cw | C3Ccw | Automorphism | ChiralExchange
  deriving (Eq, Show)

-- A4 三元组翻转: 类型上恰 3 个 Trit, 操作封闭
a4FlipTriple :: A4Op -> (Trit, Trit, Trit) -> (Trit, Trit, Trit)
a4FlipTriple op (a, b, c) = case op of
  C3Cw          -> (c3cw a, c3cw b, c3cw c)
  C3Ccw         -> (c3ccw a, c3ccw b, c3ccw c)
  Automorphism  -> (negT a, negT b, negT c)   -- (6−v) mod 3 = −v
  ChiralExchange-> (a, b, c)                  -- 手性交换: 恒等 (对齐 rust)
