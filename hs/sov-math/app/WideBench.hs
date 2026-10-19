-- | wide-bench — rho(s) s∈[0,4500) 全域计算, FNV 校验和 (强制求值)
module Main (main) where

import Data.Bits   (shiftR, xor, (.&.))
import Data.List   (foldl')
import Data.Word   (Word64)
import Sov.Math.WideFixed

main :: IO ()
main = do
  let checksum = foldl' step 1469598103934665603 [0 .. 4499 :: Int]
      step acc s =
        let W m = wideRho s
            acc' = foldl' (\a w -> (a `xor` fromIntegral (w `shiftR` k0)) `mul` prime) acc (limbs m)
        in acc'
      mul x y = x * y
      prime = 1099511628211 :: Word64
      k0 = 0 :: Int
      limbs :: Integer -> [Word64]
      limbs m = [ fromIntegral ((m `shiftR` (64 * k)) .&. mask) | k <- [0 .. 7] ]
      mask = 0xFFFFFFFFFFFFFFFF :: Integer
  putStrLn ("CHECKSUM," ++ show checksum)
