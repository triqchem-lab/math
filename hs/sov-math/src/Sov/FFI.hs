-- | Sov.FFI — 标准 C ABI 的 Haskell 实现 (cabal foreign-library → libsovffi.so)
--
-- 与 cpp/rust 实现逐符号一致; ABI 契约见 cpp/include/sov_ffi.h。
-- 全部函数以 foreign export ccall 导出; 输出经调用方缓冲区回传 (无分配语义)。
--
-- 注意: Haskell 侧参数用 CInt/CUChar/CUInt64/CHar 等 C 类型, 指针用 Ptr;
-- 状态码与 cpp/rust 完全一致 (0 OK / -1 TRIT / -2 RANGE / -4 NULL)。
module Sov.FFI
  ( -- 元信息
    sov_abi_version_wrapper
  , sov_runtime_init_wrapper
  , sov_runtime_fini_wrapper
    -- L1
  , sov_trit_add_wrapper
  , sov_trit_mul_wrapper
  , sov_trit_sun_wrapper
  , sov_trit_c3_wrapper
    -- GF(9)
  , sov_gf9_mul_wrapper
  , sov_gf9_norm_wrapper
    -- 环
  , sov_ring_add_wrapper
  , sov_ring_mul_wrapper
  , sov_ring_double_wrapper
  , sov_ring_to_tryte_wrapper
    -- DC
  , sov_dc_mixedop_wrapper
  , sov_dc_sigma_wrapper
  , sov_dc_crt_wrapper
  , sov_dc_translate_wrapper
    -- 桥
  , sov_lcm_step_wrapper
    -- 宽定点
  , sov_wide_rho_wrapper
  , sov_wide_const_pi_wrapper
  , sov_wide_const_sqrt3_wrapper
    -- 陈数
  , sov_chern_inject_wrapper
  ) where

import Data.Bits    (shiftL, shiftR, (.&.), (.|.))
import Data.Int     (Int32, Int64)
import Data.Word    (Word64, Word8, Word16)
import Foreign
import Foreign.C.String (CString)
import Foreign.C.Types

import qualified Sov.Math.DuodecClock as DC
import qualified Sov.Math.Trit as T
import qualified Sov.Math.WideConstants as WC
import qualified Sov.Math.WideFixed as WF

-- ── 状态码 ──
sovOk, sovErrTrit, sovErrRange, sovErrNull :: Int32
sovOk       = 0
sovErrTrit  = -1
sovErrRange = -2
sovErrNull  = -4

huangzhong, lcmTotal :: Word64
huangzhong = 177147
lcmTotal   = 11609505792

-- ── 内部: 环元素权重 ──
ringWeight :: [Word8] -> Word64
ringWeight = go 1
  where
    go _  []     = 0
    go w (x:xs)  = fromIntegral x * w + go (w * 3) xs

ringFromWeight :: Word64 -> [Word8]
ringFromWeight = go
  where
    go v = let (q, r) = v `divMod` 3 in fromIntegral r : (if q == 0 then [] else go q)

-- 补齐到 11 位
pad11 :: [Word8] -> [Word8]
pad11 xs = take 11 (xs ++ repeat 0)

-- ═══ 元信息 ═══

sov_abi_version_wrapper :: IO CInt
sov_abi_version_wrapper = return (fromIntegral (((1 :: Int) `shiftL` 16) .|. 0))

foreign export ccall "sov_abi_version" sov_abi_version_wrapper :: IO CInt

-- ── 运行时初始化: 调用 RTS 的 hs_init / hs_exit ──
foreign import ccall unsafe "hs_init" c_hs_init :: Ptr CInt -> Ptr (Ptr CString) -> IO ()
foreign import ccall unsafe "hs_exit" c_hs_exit :: IO ()

sov_runtime_init_wrapper :: IO CInt
sov_runtime_init_wrapper = do
  c_hs_init nullPtr nullPtr
  return 0
foreign export ccall "sov_runtime_init" sov_runtime_init_wrapper :: IO CInt

sov_runtime_fini_wrapper :: IO CInt
sov_runtime_fini_wrapper = do
  c_hs_exit
  return 0
foreign export ccall "sov_runtime_fini" sov_runtime_fini_wrapper :: IO CInt

-- ═══ L1 GF(3) ═══

tritOk :: Word8 -> Bool
tritOk v = v < 3

sov_trit_add_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_trit_add_wrapper a b out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk (fromIntegral a)) || not (tritOk (fromIntegral b)) = return (fromIntegral sovErrTrit)
  | otherwise = do
      poke out (fromIntegral ((fromIntegral a + fromIntegral b) `mod` (3 :: Int)))
      return 0
foreign export ccall "sov_trit_add" sov_trit_add_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_trit_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_trit_mul_wrapper a b out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk (fromIntegral a)) || not (tritOk (fromIntegral b)) = return (fromIntegral sovErrTrit)
  | otherwise = do
      poke out (fromIntegral ((fromIntegral a * fromIntegral b) `mod` (3 :: Int)))
      return 0
foreign export ccall "sov_trit_mul" sov_trit_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_trit_sun_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_trit_sun_wrapper a out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk (fromIntegral a)) = return (fromIntegral sovErrTrit)
  | otherwise = do
      poke out (fromIntegral ((fromIntegral a + 2) `mod` (3 :: Int)))
      return 0
foreign export ccall "sov_trit_sun" sov_trit_sun_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_trit_c3_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_trit_c3_wrapper a out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk (fromIntegral a)) = return (fromIntegral sovErrTrit)
  | otherwise = do
      poke out (fromIntegral ((fromIntegral a + 1) `mod` (3 :: Int)))
      return 0
foreign export ccall "sov_trit_c3" sov_trit_c3_wrapper :: CUChar -> Ptr CUChar -> IO CInt

-- ═══ GF(9) ═══

sov_gf9_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_gf9_mul_wrapper a b out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | a' >= 9 || b' >= 9 = return (fromIntegral sovErrRange)
  | otherwise = do
      let (x, y, u, v) = (a' `mod` 3, a' `div` 3, b' `mod` 3, b' `div` 3)
          re = (x*u + 2*y*v) `mod` 3
          im = (x*v + y*u) `mod` 3
      poke out (fromIntegral (re + 3*im))
      return 0
  where a' = fromIntegral a :: Int
        b' = fromIntegral b :: Int
foreign export ccall "sov_gf9_mul" sov_gf9_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_gf9_norm_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_gf9_norm_wrapper a out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | a' >= 9 = return (fromIntegral sovErrRange)
  | otherwise = do
      let (x, y) = (a' `mod` 3, a' `div` 3)
      poke out (fromIntegral ((x*x + y*y) `mod` 3))
      return 0
  where a' = fromIntegral a :: Int
foreign export ccall "sov_gf9_norm" sov_gf9_norm_wrapper :: CUChar -> Ptr CUChar -> IO CInt

-- ═══ Z/3^11 环 (11 个小端 trit) ═══

readRing :: Ptr CUChar -> IO [Word8]
readRing p = mapM (\i -> fromIntegral <$> peekElemOff p i) [0 .. 10]

writeRing :: Ptr CUChar -> [Word8] -> IO ()
writeRing p xs = mapM_ (\(i, v) -> pokeElemOff p i (fromIntegral v :: CUChar)) (zip [0 ..] (pad11 xs))

sov_ring_add_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_ring_add_wrapper a b out
  | a == nullPtr || b == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readRing a
      bv <- readRing b
      if any (> 2) (av ++ bv)
        then return (fromIntegral sovErrTrit)
        else do
          let w = (ringWeight av + ringWeight bv) `mod` huangzhong
          writeRing out (ringFromWeight w)
          return 0
foreign export ccall "sov_ring_add" sov_ring_add_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_ring_mul_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_ring_mul_wrapper a b out
  | a == nullPtr || b == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readRing a
      bv <- readRing b
      if any (> 2) (av ++ bv)
        then return (fromIntegral sovErrTrit)
        else do
          let w = fromIntegral ((toInteger (ringWeight av) * toInteger (ringWeight bv)) `mod` toInteger huangzhong)
          writeRing out (ringFromWeight w)
          return 0
foreign export ccall "sov_ring_mul" sov_ring_mul_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_ring_double_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_ring_double_wrapper a out = sov_ring_add_wrapper a a out
foreign export ccall "sov_ring_double" sov_ring_double_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_ring_to_tryte_wrapper :: Ptr CUChar -> Ptr CUShort -> IO CInt
sov_ring_to_tryte_wrapper a out
  | a == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readRing a
      if any (> 2) av
        then return (fromIntegral sovErrTrit)
        else do
          let v = sum [ fromIntegral (av !! i) * (3 ^ i) | i <- [0 .. 5] ] :: Int
          poke out (fromIntegral v)
          return 0
foreign export ccall "sov_ring_to_tryte" sov_ring_to_tryte_wrapper :: Ptr CUChar -> Ptr CUShort -> IO CInt

-- ═══ DC 联合时钟 ═══

sov_dc_mixedop_wrapper :: CUChar -> CUChar -> CUChar -> CUChar
                       -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_dc_mixedop_wrapper t1 k1 t2 k2 tout kout
  | tout == nullPtr || kout == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk t1') || not (tritOk t2') = return (fromIntegral sovErrTrit)
  | k1' > 3 || k2' > 3 = return (fromIntegral sovErrRange)
  | otherwise = do
      let DC.DP t k = DC.mixedOp (DC.DP (toEnum (fromIntegral t1')) (toEnum (fromIntegral k1'))) (DC.DP (toEnum (fromIntegral t2')) (toEnum (fromIntegral k2')))
      poke tout (fromIntegral (fromEnum t))
      poke kout (fromIntegral (fromEnum k))
      return 0
  where t1' = fromIntegral t1; k1' = fromIntegral k1
        t2' = fromIntegral t2; k2' = fromIntegral k2
foreign export ccall "sov_dc_mixedop" sov_dc_mixedop_wrapper :: CUChar -> CUChar -> CUChar -> CUChar
                                              -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_dc_sigma_wrapper :: CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_dc_sigma_wrapper t k tout kout
  | tout == nullPtr || kout == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk t') = return (fromIntegral sovErrTrit)
  | k' > 3 = return (fromIntegral sovErrRange)
  | otherwise = do
      let DC.DP t2 k2 = DC.sigmaDC (DC.DP (toEnum (fromIntegral t')) (toEnum (fromIntegral k')))
      poke tout (fromIntegral (fromEnum t2))
      poke kout (fromIntegral (fromEnum k2))
      return 0
  where t' = fromIntegral t; k' = fromIntegral k
foreign export ccall "sov_dc_sigma" sov_dc_sigma_wrapper :: CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_dc_crt_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_dc_crt_wrapper t k out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk t') = return (fromIntegral sovErrTrit)
  | k' > 3 = return (fromIntegral sovErrRange)
  | otherwise = do
      poke out (fromIntegral (DC.toDuodec (DC.DP (toEnum (fromIntegral t')) (toEnum (fromIntegral k')))))
      return 0
  where t' = fromIntegral t; k' = fromIntegral k
foreign export ccall "sov_dc_crt" sov_dc_crt_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_dc_translate_wrapper :: CUInt -> CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_dc_translate_wrapper n t k tout kout
  | tout == nullPtr || kout == nullPtr = return (fromIntegral sovErrNull)
  | not (tritOk t') = return (fromIntegral sovErrTrit)
  | k' > 3 = return (fromIntegral sovErrRange)
  | otherwise = do
      let DC.DP t2 k2 = DC.translate (fromIntegral n) (DC.DP (toEnum (fromIntegral t')) (toEnum (fromIntegral k')))
      poke tout (fromIntegral (fromEnum t2))
      poke kout (fromIntegral (fromEnum k2))
      return 0
  where t' = fromIntegral t; k' = fromIntegral k
foreign export ccall "sov_dc_translate" sov_dc_translate_wrapper :: CUInt -> CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

-- ═══ LCM 桥 (值语义, 与 cpp/rust 同配方) ═══

mulSmall :: [Word64] -> Word64 -> [Word64]
mulSmall xs k = go xs 0
  where
    go []     _ = []
    go (x:rest) c =
      let t = toInteger x * toInteger k + toInteger c
          lo = fromIntegral (t .&. 0xFFFFFFFFFFFFFFFF)
          hi = fromIntegral (t `shiftR` 64)
      in lo : go rest hi

shrSmall :: [Word64] -> Int -> [Word64]
shrSmall xs bits = [ (x `shiftR` bits) .|. if i + 1 < length xs then (xs !! (i + 1)) `shiftL` (64 - bits) else 0
                   | (i, x) <- zip [0 ..] xs ]

sov_lcm_step_wrapper :: Ptr CULLong -> CULLong -> Ptr CUInt -> IO CInt
sov_lcm_step_wrapper limbs delta sc
  | limbs == nullPtr || sc == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ls <- mapM (\i -> fromIntegral <$> (peekElemOff limbs i :: IO CULLong)) [0 .. 5]
      d  <- peek sc
      let ls1 = mulSmall ls huangzhong
          low = head ls1 + fromIntegral delta
          carry = low `div` lcmTotal
          ls2 = (low `mod` lcmTotal) : (ls1 !! 1 + carry) : drop 2 ls1
          sc1 = d + 1
          (ls3, sc2) = if sc1 >= 12
                         then (shrSmall (mulSmall ls2 huangzhong) 16, 0)
                         else (ls2, sc1)
      mapM_ (\(i, v) -> pokeElemOff limbs i (fromIntegral v :: CULLong)) (zip [0 ..] ls3)
      poke sc sc2
      return 0
foreign export ccall "sov_lcm_step" sov_lcm_step_wrapper :: Ptr CULLong -> CULLong -> Ptr CUInt -> IO CInt

-- ═══ 宽定点 Q(128.384) ═══

writeWide :: Ptr CULLong -> Integer -> IO ()
writeWide out m = mapM_ (\(i, v) -> pokeElemOff out i (fromIntegral v))
  [ (i, (m `shiftR` (64 * i)) .&. 0xFFFFFFFFFFFFFFFF) | i <- [0 .. 7] ]

sov_wide_rho_wrapper :: CULLong -> Ptr CULLong -> IO CInt
sov_wide_rho_wrapper s out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let WF.W m = WF.wideRho (fromIntegral s)
      writeWide out m
      return 0
foreign export ccall "sov_wide_rho" sov_wide_rho_wrapper :: CULLong -> Ptr CULLong -> IO CInt

sov_wide_const_pi_wrapper :: Ptr CULLong -> IO CInt
sov_wide_const_pi_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = writeWide out WC.pi_w >> return 0
foreign export ccall "sov_wide_const_pi" sov_wide_const_pi_wrapper :: Ptr CULLong -> IO CInt

sov_wide_const_sqrt3_wrapper :: Ptr CULLong -> IO CInt
sov_wide_const_sqrt3_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = writeWide out WC.sqrt3_w >> return 0
foreign export ccall "sov_wide_const_sqrt3" sov_wide_const_sqrt3_wrapper :: Ptr CULLong -> IO CInt

-- ═══ 陈数 ═══

sov_chern_inject_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
sov_chern_inject_wrapper num den
  | num == nullPtr || den == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke num (-432)
      poke den 216
      return 0
foreign export ccall "sov_chern_inject" sov_chern_inject_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
