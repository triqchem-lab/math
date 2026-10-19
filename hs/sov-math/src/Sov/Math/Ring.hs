-- | Sov.Math.Ring — L2 Z/3¹¹Z 环 (镜像 rust l2_ring.rs / C++ z3r_ring.h)
--
-- RingElement = Vec N11 Trit (小端: 索引 0 = d₀, 位权 3^k)。
-- 长度 11 编码在类型里: 位数错误无法编译。
-- 加法 = 逐位 trit 加 + 逢三进一 (carry); 乘法 = 系数积含本征进位传播
-- (2×2=4 → 本位 1 + 进位 1, 交叉验证回归 rust test_mul_coeff_carry)。
--
-- 宪法: 3¹¹ = T₁₁ × … 为零 — T₁ 的 11 次幂 = 0 (test_t1_pow11_zero)。
module Sov.Math.Ring
  ( RingElement(..)
  , ringZero
  , ringFromTrit       -- 标量 trit → 环元素 (低 0 位)
  , t0e, t1e, t2e, t3e, t4e, t5e, t6e
  , isZeroR
  , isOneR
  , digitR             -- 第 k 位 trit (越界 Nothing)
  , toTryteR           -- 低 6 位 → Tryte
  , addR
  , mulR
  , mulPowR            -- 幂: 重复乘
  , ringFromList       -- [Trit] → Maybe RingElement (长度必须恰 11)
  ) where

import Sov.Math.Nat
import Sov.Math.Trit
import Sov.Math.Tryte
import Sov.Math.Vec

newtype RingElement = RingElement { unRing :: Vec N11 Trit }
  deriving (Eq, Show)

ringZero :: RingElement
ringZero = RingElement (replicateV n11 T0)

-- 标量 trit: d₀ = t, 其余 0
ringFromTrit :: Trit -> RingElement
ringFromTrit t = RingElement (t :> replicateV n10 T0)
  where n10 :: SNat ('S ('S ('S ('S ('S ('S ('S ('S ('S ('S N0))))))))))
        n10 = SS (SS (SS (SS (SS (SS (SS (SS (SS (SS n0)))))))))

-- 第 k 位设值 (长度不变, 类型层安全)
setDigit :: Int -> Trit -> Vec n Trit -> Vec n Trit
setDigit _ _ VNil     = VNil
setDigit 0 t (_ :> xs) = t :> xs
setDigit k t (x :> xs) = x :> setDigit (k - 1) t xs

-- 3^k 生成元表: t0e=1, t1e=3, t2e=9, …, t6e=729
t0e, t1e, t2e, t3e, t4e, t5e, t6e :: RingElement
t0e = ringFromTrit T1
t1e = RingElement (setDigit 1 T1 (replicateV n11 T0))
t2e = mulR t1e t1e
t3e = mulR t2e t1e
t4e = mulR t3e t1e
t5e = mulR t4e t1e
t6e = mulR t5e t1e

isZeroR :: RingElement -> Bool
isZeroR = allV (== T0) . unRing

isOneR :: RingElement -> Bool
isOneR (RingElement v) = headV v == T1 && allV (== T0) (tailV v)

digitR :: Int -> RingElement -> Maybe Trit
digitR k (RingElement v) = indexV k v

-- 低 6 位 → Tryte (投影: 与 rust to_tryte 一致)
toTryteR :: RingElement -> Tryte
toTryteR (RingElement v) = tryteFromVec (take6 v)
  where
    take6 :: Vec N11 Trit -> Vec N6 Trit
    take6 (a :> b :> c :> d :> e :> f :> _) = a :> b :> c :> d :> e :> f :> VNil

-- 加法: 逢三进一 (carry 链), 完全对齐 rust impl Add
addR :: RingElement -> RingElement -> RingElement
addR (RingElement a) (RingElement b) = RingElement (go T0 a b)
  where
    go :: Trit -> Vec n Trit -> Vec n Trit -> Vec n Trit
    go _  VNil     VNil     = VNil
    go c  (x :> xs) (y :> ys) =
      let s   = addT (addT x y) c
          -- 进位: 三值和 ≥ 3 时 → 1, 否则 0
          -- 三值分量: x,y,c ∈ {0,1,2}, 和 ∈ [0,6]; carry = floor(和/3)
          carry = if toTritIndex x + toTritIndex y + toTritIndex c >= 3 then T1 else T0
      in s :> go carry xs ys

-- 乘法: 系数积 aᵢ·bⱼ 含本征进位传播 (2×2=4 → 1 + 3·1)。
-- 实现走位权整数: 环元素 ≅ Σ dᵢ·3ⁱ (i ∈ [0,11)), 乘积 mod 3¹¹ 后按基 3 重编码。
-- 与 rust 逐位进位传播算法等价: 基 3 进位链保持值 mod 3¹¹ 不变
-- (cᵢ·3ⁱ ≡ (cᵢ mod 3)·3ⁱ + 3^{i+1}·⌊cᵢ/3⌋), 且两端同样截断 ≥3¹¹ 的部分,
-- 归约到同一规范代表元 — 由 testMulCoeffCarry / testT1Pow11Zero 交叉验证。
mulR :: RingElement -> RingElement -> RingElement
mulR x y = fromWeight ((toWeight x * toWeight y) `mod` ((3 :: Integer) ^ (11 :: Int)))
  where
    toWeight :: RingElement -> Integer
    toWeight (RingElement v) = goW 1 (toList v)
      where goW :: Integer -> [Trit] -> Integer
            goW _  []     = 0
            goW w (t : ts) = fromIntegral (toTritIndex t) * w + goW (w * 3) ts

    fromWeight :: Integer -> RingElement
    fromWeight w = RingElement (goF w n11)
      where
        goF :: Integer -> SNat m -> Vec m Trit
        goF _  SZ     = VNil
        goF v (SS n)  = fromTritIndex (fromIntegral (v `mod` 3)) :> goF (v `div` 3) n

-- | 幂: 重复乘 (n ≥ 0; n = 0 → 1)
mulPowR :: RingElement -> Int -> RingElement
mulPowR x n
  | n <= 0    = t0e
  | otherwise = foldl' mulR x (replicate (n - 1) x)

-- | 从列表构造: 长度必须恰 11 (类型层定长, 运行期列表不匹配 → Nothing)
ringFromList :: [Trit] -> Maybe RingElement
ringFromList ts = RingElement <$> fromListN n11 ts
