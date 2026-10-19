-- | Sov.Math.Guard.Chern — S² 12 胞腔 C=2 陈数构造器 (镜像 rust sov-guard/chern.rs)
--
-- Z[ω] 精确整数版 (零浮点)。代数同构: 相位 ∈ {0, ±2π/3} = 三次单位根 {1, ω, ω²}
-- ⊂ Z[ω] 单位群, Wilson loop 连乘 = Z[ω] 乘法 (Sov.Math.Eis), 无需 cos/sin/atan2。
--
-- 缠绕数 W = 净旋转圈数: 每 trit 链接 Δθ ∈ {0, ±2π/3} → 单位指数 ±1,
-- C = TOTAL_SIGNED / 216 (12 heads × 18 trit)。
-- 关键事实: 浮点版 C = −1.9999999999999998, 精确版 C = −432/216 = **−2 精确**。
--
-- 强类型化: Proto = Vec N12 (Vec N12 (Vec N6 Trit)) — head/cell/trit 三层长度全部
-- 编码在类型里, 索引越界与位数错误不可表达; 胞腔邻接走 mod 12 总函数。
module Sov.Math.Guard.Chern
  ( Proto(..)
  , protoZero
  , cellAdj
  , c3OrbitHead
  , setCell
  , injectC2Monopole
  , tritLink
  , edgeHolonomy
  , cellLoop
  , cellWilsonHolonomy
  , computeChernExact
  , ChernExact(..)
  , detectChern
  , chernEquals
  , s2Cells
  , headsCount
  ) where

import Data.Int   (Int64)
import Sov.Math.Eis
import Sov.Math.Nat
import Sov.Math.Trit
import Sov.Math.Vec

s2Cells, headsCount :: Int
s2Cells    = 12
headsCount = 12

-- | Tryte 原型矩阵: [12 head][12 cell][6 trit]
newtype Proto = Proto { protoTrits :: Vec N12 (Vec N12 (Vec N6 Trit)) }

protoZero :: Proto
protoZero = Proto (replicateV n12 (replicateV n12 (replicateV n6 T0)))

-- | 胞腔邻接: cell → (c3cw, c3ccw, mugong) — mod 12 总函数
cellAdj :: Int -> (Int, Int, Int)
cellAdj cell = adjList !! (cell `mod` 12)
  where
    adjList =
      [ (4, 8, 3), (5, 9, 4), (6, 10, 5), (7, 11, 6)
      , (8, 0, 7), (9, 1, 8), (10, 2, 9), (11, 3, 10)
      , (0, 4, 11), (1, 5, 0), (2, 6, 1), (3, 7, 2) ]

-- | C₃ 轨道头: orbitIdx → 首胞腔 (4 组 × 3 胞腔)
c3OrbitHead :: Int -> Int
c3OrbitHead o = [0, 1, 2, 3] !! (o `mod` 4)

-- | 设定 (head, cell) 的 6-trit 原型
setCell :: Int -> Int -> Vec N6 Trit -> Proto -> Proto
setCell h c t (Proto v) =
  Proto (replaceAt h (replaceAt c t (atIdx h v)) v)
  where
    atIdx :: Int -> Vec n a -> a
    atIdx i xs = maybe (error "setCell: index out of range") id (indexV i xs)

-- 定点替换: 位置 i 的元素换成 new, 长度不变 (类型层安全)
replaceAt :: Int -> a -> Vec n a -> Vec n a
replaceAt _ _ VNil          = VNil
replaceAt 0 new (_ :> rest) = new :> rest
replaceAt k new (x :> rest) = x :> replaceAt (k - 1) new rest

-- | 注入 C=2 单极构型 (2 条 C₃ 轨道 × 12 head), 返回注入胞腔数 (=24)
injectC2Monopole :: Proto -> (Proto, Int)
injectC2Monopole p0 = go p0 0 0
  where
    go :: Proto -> Int -> Int -> (Proto, Int)
    go p h injected
      | h >= headsCount = (p, injected)
      | otherwise =
          let orbit o =
                let cell = c3OrbitHead o
                    (c1, _, _) = cellAdj cell
                    (c2, _, _) = cellAdj c1
                in [ (cell, [T0, T1, T2, T1, T2, T0])
                   , (c1,   [T1, T2, T0, T2, T0, T1])
                   , (c2,   [T1, T1, T1, T2, T2, T2]) ]
              cells = concatMap orbit [0, 1]
              p' = foldr (\(cell, ts) acc -> setCell h cell (toVec6 ts) acc) p cells
          in go p' (h + 1) (injected + 2)
    toVec6 :: [Trit] -> Vec N6 Trit
    toVec6 ts = case fromListN n6 ts of
      Just v  -> v
      Nothing -> error "injectC2Monopole: internal"

-- | trit 相位差 → Z[ω] 单位 + 缠绕指数: (a−b) mod 3 → {0→(1,0), 1→(ω,+1), 2→(ω²,−1)}
tritLink :: Trit -> Trit -> (Eis, Int64)
tritLink a b = case ((ti a + 3 - ti b) `mod` 3) of
  1 -> (eisOmega,  1)
  2 -> (eisOmega2, -1)
  _ -> (eisOne, 0)
  where ti = fromIntegral . toTritIndex :: Trit -> Int

-- | 边 (6-trit) 的和乐与缠绕指数: Π ω^(±1), Σ(±1)
edgeHolonomy :: Vec N6 Trit -> Vec N6 Trit -> (Eis, Int64)
edgeHolonomy a b = foldr step (eisOne, 0) (zipV' (toList a) (toList b))
  where
    zipV' = zip
    step (x, y) (u, w) = let (l, s) = tritLink x y in (mulE u l, w + s)

-- | 单胞腔 Wilson 回路: cell → c3cw → c3cw² → cell, 返回 (和乐单位, 缠绕指数和)
cellLoop :: Proto -> Int -> Int -> (Eis, Int64)
cellLoop proto h cell =
  let (c1, _, _) = cellAdj cell
      (c2, _, _) = cellAdj c1
      at i c = protoAt proto i c
      (u1, w1) = edgeHolonomy (at h cell) (at h c1)
      (u2, w2) = edgeHolonomy (at h c1)   (at h c2)
      (u3, w3) = edgeHolonomy (at h c2)   (at h cell)
  in (mulE u1 (mulE u2 u3), w1 + w2 + w3)

protoAt :: Proto -> Int -> Int -> Vec N6 Trit
protoAt (Proto v) h c =
  let row = maybe (error "protoAt: head") id (indexV h v)
  in maybe (error "protoAt: cell") id (indexV c row)

-- | 单胞腔 Wilson 和乐 (Z[ω] 单位) — 拓扑不变量载体
cellWilsonHolonomy :: Proto -> Int -> Int -> Eis
cellWilsonHolonomy proto h cell = fst (cellLoop proto h cell)

-- | 精确有理陈数: C = (缠绕符号总和, 216), 分母恒为 12 heads × 18 trit
computeChernExact :: Proto -> (Int64, Int64)
computeChernExact proto =
  ( sum [ snd (cellLoop proto h cell) | cell <- [0 .. s2Cells - 1]
                                      , h    <- [0 .. headsCount - 1] ]
  , fromIntegral (headsCount * 18) )

-- | 陈数检测结果 (全整数/有理)
data ChernExact = ChernExact { chernNum :: !Int64, chernDen :: !Int64 }
  deriving (Eq, Show)

detectChern :: Proto -> ChernExact
detectChern proto = uncurry ChernExact (computeChernExact proto)

-- | C 是否精确等于 tNum/tDen (交叉相乘比较, 无除法误差)
chernEquals :: ChernExact -> Int64 -> Int64 -> Bool
chernEquals c tNum tDen = chernNum c * tDen == tNum * chernDen c
