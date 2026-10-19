-- | wide-bench — rho(s) s∈[0,4500) 全域计算, FNV 校验和 (强制求值)
module Main (main) where

import Data.Bits   (shiftR, xor, (.&.))
import Data.List   (foldl')
import Data.Word   (Word64)
import Sov.Math.WideFixed
import System.Environment (getArgs)
import Data.Char (digitToInt)

main :: IO ()
main = do
  args <- getArgs
  let passes = case args of
        (a : _) -> foldl' (\n c -> n * 10 + digitToInt c) 0 a
        _       -> 1
      checksum = foldl' step 1469598103934665603
                   [ fromIntegral ((s + p * 37) `mod` 4500)
                   | p <- [0 :: Int .. passes - 1], s <- [0 .. 4499 :: Int] ]
      step acc s =
        let W m = wideRho s
            acc' = foldl' (\a w -> (a `xor` w) * prime) acc (limbs m)
        in acc'
      prime = 1099511628211 :: Word64
      limbs :: Integer -> [Word64]
      limbs m = [ fromIntegral ((m `shiftR` (64 * k)) .&. mask) | k <- [0 .. 7] ]
      mask = 0xFFFFFFFFFFFFFFFF :: Integer
  putStrLn ("CHECKSUM," ++ show checksum)
