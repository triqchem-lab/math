-- | Sov.Math.Topology.Tower — 指数塔 (镜像 rust sov-topology L8 塔, 零浮点改造)
--
-- 宪法改造: rust towerHeightFromZhonglv 用 f64 log10 — 此处换成**精确整数交叉乘谓词**:
--   fl = 26355/10000 + zc·34541/10000 (有理数, 无舍入)
--   tower_height > num/den  ⟺  fl > 12^(num/den)  ⟺  fl_num^den > 12^num · 10000^den
-- 与 f64 版全域 zc∈[0,3000] 逐点对账一致, 边界 min{zc : h>2} = 41
-- (oracle 回执 86c6aca20a322a386a7bd5cd524eb8e59b389fac47026bffae76cf75a1546e10,
--  脚本 engineering/oracle_tower.py)。
module Sov.Math.Topology.Tower
  ( towerBase
  , towerA_Num, towerB_Num, towerDen
  , flNum
  , towerHeightExceeds
  , towerHeightQ16
  , zhonglvFromTowerFloor
  ) where


-- 常数 (rust: TOWER_BASE=12, 2.6355, 3.4541 — 有理化)
towerBase :: Integer
towerBase = 12

towerA_Num, towerB_Num, towerDen :: Integer
towerA_Num = 26355   -- 2.6355
towerB_Num = 34541   -- 3.4541
towerDen   = 10000

-- fl 的分子 (分母 towerDen): fl = flNum zc / towerDen
flNum :: Integer -> Integer
flNum zc = towerA_Num + towerB_Num * zc

-- | tower_height > num/den 的精确整数判定 (无对数, 无舍入)
towerHeightExceeds :: Integer -> Integer -> Integer -> Bool
towerHeightExceeds zc num den =
  flNum zc ^ den > towerBase ^ num * towerDen ^ den

-- | 塔高的 Q16 定点值: h = log12(fl) — 用有理数运算不可能, 改供**下界近似**:
-- 最大 n 使 fl ≥ 12^n (整数冪逼近, 精确判定), 返回 n × 65536
towerHeightQ16 :: Integer -> Integer
towerHeightQ16 zc = fromIntegral (go 0) * 65536
  where
    fl = flNum zc
    go :: Integer -> Integer
    -- fl ≥ 12^(n+1) ⟺ flNum ≥ 12^(n+1)·10000 (保持有理数精确, 无除法)
    go n | towerBase ^ (n + 1) * towerDen <= fl = go (n + 1)
         | otherwise                            = n

-- | rust zhonglvFromTower 的整数版: 最大 zc 使 tower_height ≤ num/den
-- 即 (flNum/10000)^den ≤ 12^num ⟺ flNum^den ≤ 12^num·10000^den (对 zc 单调 → 二分)
zhonglvFromTowerFloor :: Integer -> Integer -> Integer
zhonglvFromTowerFloor num den = bisect 0 hi
  where
    bound = towerBase ^ num * towerDen ^ den
    p zc  = flNum zc ^ den <= bound
    hi = bound  -- 安全上界 (flNum zc ≥ zc, 谓词在 zc=bound 处必假)
    bisect lo cur
      | cur - lo <= 1 = if p cur then cur else lo
      | p mid = bisect mid cur
      | otherwise = bisect lo mid
      where mid = (lo + cur) `div` 2
