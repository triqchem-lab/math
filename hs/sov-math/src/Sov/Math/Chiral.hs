-- | Sov.Math.Chiral — L3 手性几何 + 五行振幅 (镜像 rust sov-chiral/lib.rs)
--
-- Q16.16 定点复数复用 Sov.Math.FixedComplex (不重写)。
-- 强类型化: ChiralCoupling 5 构造子枚举 (替代裸 i32 match);
-- 五行振幅表 = Vec N5 (Vec N5 Q16Complex) — 5×5 长度在类型上。
module Sov.Math.Chiral
  ( chiralConj
  , isSelfConj
  , wuxingTable
  , ChiralCoupling(..)
  , couplingFromPower
  , couplingFromPowerMay
  , isChiralSeparated
  ) where

import Sov.Math.FixedComplex
import Sov.Math.Nat
import Sov.Math.Trit
import Sov.Math.Vec

-- 手性共轭: T1↔T2 (LUT [0,2,1] 的枚举版 — 不可能有非法下标)
chiralConj :: Trit -> Trit
chiralConj T0 = T0
chiralConj T1 = T2
chiralConj T2 = T1

-- 自共轭: 仅 T0
isSelfConj :: Trit -> Bool
isSelfConj t = t == T0

-- 五行振幅表: 相生→+1 (对角 + 环 4→0), 相克→ω, 克制→ω²
wuxingTable :: Vec N5 (Vec N5 Q16Complex)
wuxingTable = generateV n5 genRow
  where
    genRow i = generateV n5 (genCell i)
    genCell i j
      | i == j                    = qcOne
      | j == (i + 1) `mod` 5      = qcOne       -- 相生环: 0→1→2→3→4→0
      | (i + 2) `mod` 5 == j      = qcOmega    -- 相克: 间隔 2
      | otherwise                 = qcOmega2   -- 其余: 克制

-- 离合器状态 — 5 构造子枚举 (rust 裸 i32 match 的类型升级)
data ChiralCoupling = Idle | Meshed | HalfLink | Slipping | Decoupled
  deriving (Eq, Show)

-- 部分函数版: 未知功率码 → Nothing (rust 侧落入 Decoupled)
couplingFromPowerMay :: Int -> Maybe ChiralCoupling
couplingFromPowerMay 0 = Just Idle
couplingFromPowerMay 1 = Just Meshed
couplingFromPowerMay 3 = Just HalfLink
couplingFromPowerMay 4 = Just Slipping
couplingFromPowerMay _ = Nothing

-- 总函数版 (对齐 rust 语义: 未知码 → Decoupled)
couplingFromPower :: Int -> ChiralCoupling
couplingFromPower a = maybe Decoupled id (couplingFromPowerMay a)

-- 手性分离判定
isChiralSeparated :: Int -> Bool
isChiralSeparated a = a >= 4
