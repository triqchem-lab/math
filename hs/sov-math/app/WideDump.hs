-- | wide-dump — 导出 Q(128.384) 宽定点常数与 rho, 尾数用 Doz (十二进制) 表示
--
-- 本体论纪律 (docs/duodecimal/08-terminology.md §Doz): 十二进制这里是**记数法层**,
-- 不改变数值本体 (尾数 = 值 × 2^384 的整数); 禁止与 DC/C₁₂/R₁₂ 混写。
-- 位形: A=10, B=11, 高位在前 (22-doz-formalization.md §render)。
module Main (main) where

import Sov.Math.WideFixed
import Sov.Math.WideConstants

-- | Doz 渲染: 高位在前, A=10, B=11 (非负整数; 负数加前缀 '-')
doz :: Integer -> String
doz n
  | n < 0     = '-' : dozPos (negate n)
  | otherwise = dozPos n
  where
    dozPos 0 = "0"
    dozPos v = go v ""
    go 0 acc = acc
    go v acc = go (v `div` 12) (digit (v `mod` 12) : acc)
    digit d = case d of
      10 -> 'A'
      11 -> 'B'
      _  -> toEnum (fromEnum '0' + fromIntegral d)

main :: IO ()
main = do
  -- DOZ 行 (主表示) + DEC 行 (交叉核对, oracle 双格式验证)
  mapM_ (\(n, v) -> do
    putStrLn ("CONST_DOZ," ++ n ++ "," ++ doz v)
    putStrLn ("CONST_DEC," ++ n ++ "," ++ show v))
    [ ("PI", pi_w), ("SQRT3", sqrt3_w), ("SQRT3_HALF", sqrt3_half_w) ]
  mapM_ (\s -> do
    let m = mant (wideRho s)
    putStrLn ("WRHO_DOZ," ++ show s ++ "," ++ doz m)
    putStrLn ("WRHO_DEC," ++ show s ++ "," ++ show m))
    [0, 1, 450, 1125, 2250, 3000, 4499, 4500 :: Int]
  where
    mant (W m) = m
