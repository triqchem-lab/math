-- | precision-dump — 导出 hs 侧全部 approx 投影的真实输出 (供 mpmath 高精度审计)
module Main (main) where

import Sov.Math.Topology.Soliton
import Sov.Math.Guard.L7L8
import Sov.Math.FixedComplex
import Sov.Math.Constants
import Sov.Math.Topology.Tower

main :: IO ()
main = do
  -- 1. soliton rho 全域 4500 点 (approx: Q32-Taylor → Q16)
  mapM_ (\s -> putStrLn ("RHO," ++ show s ++ "," ++ show (solitonRhoQ16Approx s)))
        [0 .. 4499]
  -- 2. 全息标度 approx (Q32 对数常数)
  mapM_ (\(w, a, st, z) ->
    putStrLn ("HOLO," ++ show w ++ "," ++ show a ++ "," ++ show st ++ "," ++ show z
              ++ "," ++ show (computeHolographicApprox (fromIntegral w) (fromIntegral a) st z)))
    [(16558, 0, 31000, 2583 :: Int), (0, 0, 96, 0), (0, 0, 1200, 100), (6624, 6624, 6624, 6624)]
  -- 3. Q16 常数投影 (定点近似值本身)
  mapM_ (\(n, v) -> putStrLn (n ++ "," ++ show v))
    [ ("PI_HALF_Q32", show piHalfQ32)
    , ("OMEGA_IM_Q16", show omegaImQ16)
    , ("OMEGA2_IM_Q16", show omega2ImQ16)
    , ("DELTA_Q16", show deltaQ16)
    , ("CHIRAL_COLLAPSE_Q16", show chiralCollapse)
    , ("SQRT3_FLOOR_Q16", show sqrt3Q16Floor)
    ]
  -- 4. 真空尾数 (尾数 Q16, 指数) 表示
  mapM_ (\(n, m, e) -> putStrLn (n ++ "," ++ show m ++ "," ++ show e))
    [ ("VAC_LEONOV_L", 48497 :: Int, -25 :: Int)
    , ("VAC_LEONOV_RHO", 232653, 75)
    , ("VAC_HARAMEIN", 338166, 96)
    ]
  -- 5. 塔高 Q16 floor (整数幂下界)
  mapM_ (\zc -> putStrLn ("TOWER," ++ show zc ++ "," ++ show (towerHeightQ16 zc)))
        [0, 41, 100, 2583, 6624]
