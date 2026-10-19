-- | Sov.Math.Vec — DataKinds 定长向量
--
-- 长度编码进类型: Vec n a 的 n 是类型级 Nat。
-- zip / add / mul 等运算要求两侧长度在类型上相等 — 错位数无法编译。
-- fromList 返回 Maybe: 运行期列表转定长向量显式处理失败。
{-# LANGUAGE DataKinds, GADTs, KindSignatures, TypeOperators #-}
module Sov.Math.Vec
  ( Vec(..)
  , vecLength
  , toList
  , fromListN     -- 定长构造: 给 SNat 证据
  , fromListMay
  , mapV
  , zipWithV
  , zipV
  , foldrV
  , indexV        -- 有限下标访问 (0 起的普通 Int, 越界返回 Nothing)
  , replicateV
  , generateV
  , allV
  , headV
  , tailV
  , appendV
  ) where

import Data.Kind (Type)
import Sov.Math.Nat

-- | 定长向量: 构造子 VNil / (:>) 长度在类型层精确
infixr 5 :>
data Vec (n :: Nat) (a :: Type) where
  VNil  :: Vec 'Z a
  (:>)  :: a -> Vec n a -> Vec ('S n) a

instance Show a => Show (Vec n a) where
  show = show . toList

instance Eq a => Eq (Vec n a) where
  VNil   == VNil   = True
  (x :> xs) == (y :> ys) = x == y && xs == ys

vecLength :: SNat n -> Vec n a -> Int
vecLength _ v = go v 0
  where
    go :: Vec m a -> Int -> Int
    go VNil acc     = acc
    go (_ :> xs) acc = go xs (acc + 1)

toList :: Vec n a -> [a]
toList VNil     = []
toList (x :> xs) = x : toList xs

-- | 从已知长度的列表构造 (SNat 提供类型级长度证据)
fromListN :: SNat n -> [a] -> Maybe (Vec n a)
fromListN SZ     []       = Just VNil
fromListN SZ     _        = Nothing
fromListN (SS _) []       = Nothing
fromListN (SS n) (x : xs) = (x :>) <$> fromListN n xs

-- | 运行期列表转定长向量 (长度由向量自己声明, 列表按需取头/补尾)
fromListMay :: SNat n -> [a] -> Maybe (Vec n a)
fromListMay = fromListN

mapV :: (a -> b) -> Vec n a -> Vec n b
mapV _ VNil       = VNil
mapV f (x :> xs)  = f x :> mapV f xs

zipWithV :: (a -> b -> c) -> Vec n a -> Vec n b -> Vec n c
zipWithV _ VNil         VNil       = VNil
zipWithV f (x :> xs) (y :> ys) = f x y :> zipWithV f xs ys

zipV :: Vec n a -> Vec n b -> Vec n (a, b)
zipV = zipWithV (,)

foldrV :: (a -> b -> b) -> b -> Vec n a -> b
foldrV _ z VNil      = z
foldrV f z (x :> xs) = f x (foldrV f z xs)

indexV :: Int -> Vec n a -> Maybe a
indexV _ VNil      = Nothing
indexV 0 (x :> _)  = Just x
indexV k (_ :> xs) = indexV (k - 1) xs

replicateV :: SNat n -> a -> Vec n a
replicateV SZ     _ = VNil
replicateV (SS n) x = x :> replicateV n x

generateV :: SNat n -> (Int -> a) -> Vec n a
generateV SZ     _ = VNil
generateV (SS n) f = f 0 :> generateV n (f . (+ 1))

allV :: (a -> Bool) -> Vec n a -> Bool
allV p = foldrV (\x acc -> p x && acc) True

headV :: Vec ('S n) a -> a
headV (x :> _) = x

tailV :: Vec ('S n) a -> Vec n a
tailV (_ :> xs) = xs

appendV :: Vec n a -> Vec m a -> Vec (Plus n m) a
appendV VNil     ys = ys
appendV (x :> xs) ys = x :> appendV xs ys

-- 类型级加法 (appendV 的返回长度)
type family Plus (n :: Nat) (m :: Nat) :: Nat where
  Plus 'Z     m = m
  Plus ('S n) m = 'S (Plus n m)
