-- | Sov.Math.Nat — 类型级自然数与单项式 (singleton)
--
-- 强类型化基础设施: 定长向量 Vec n a 的长度索引。
-- 长度在类型层, 运行期不可错位。
{-# LANGUAGE DataKinds, GADTs, KindSignatures, TypeOperators,
             ScopedTypeVariables, StandaloneKindSignatures #-}
module Sov.Math.Nat
  ( Nat(..)
  , SNat(..)
  , snatToNat
  , snatToNatValue
  , N0, N1, N2, N3, N4, N5, N6, N11, N12
  , eqNat
  , n0, n1, n2, n3, n4, n5, n6, n11, n12
  ) where

import Data.Type.Equality ((:~:)(Refl))

-- | 类型级 Peano 自然数 (kind Nat)
data Nat = Z | S Nat

-- | 单项式: 值与类型层长度一一对应, 不可能不一致
data SNat (n :: Nat) where
  SZ :: SNat 'Z
  SS :: SNat n -> SNat ('S n)

type N0  = 'Z
type N1  = 'S N0
type N2  = 'S N1
type N3  = 'S N2
type N4  = 'S N3
type N5  = 'S N4
type N6  = 'S N5
type N11 = 'S ('S ('S ('S ('S ('S ('S ('S ('S ('S ('S N0))))))))))
type N12 = 'S N11

n0 :: SNat N0
n0 = SZ

n1 :: SNat N1
n1 = SS n0

n2 :: SNat N2
n2 = SS n1

n3 :: SNat N3
n3 = SS n2

n4 :: SNat N4
n4 = SS n3

n5 :: SNat N5
n5 = SS n4

n6 :: SNat N6
n6 = SS n5

n11 :: SNat N11
n11 = SS (SS (SS (SS (SS (SS (SS (SS (SS (SS (SS n0))))))))))

n12 :: SNat N12
n12 = SS n11

snatToNat :: SNat n -> Nat
snatToNat SZ     = Z
snatToNat (SS n) = S (snatToNat n)

snatToNatValue :: SNat n -> Int
snatToNatValue SZ     = 0
snatToNatValue (SS n) = 1 + snatToNatValue n

-- 长度相等判定 (singleton 比较产生类型证据, 供 Vec 层使用)
eqNat :: SNat n -> SNat m -> Maybe (n :~: m)
eqNat SZ     SZ     = Just Refl
eqNat (SS a) (SS b) = case eqNat a b of
  Just Refl -> Just Refl
  Nothing   -> Nothing
eqNat _ _ = Nothing
