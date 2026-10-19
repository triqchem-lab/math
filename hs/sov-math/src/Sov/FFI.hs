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
    -- 第二批
  , sov_sunyi_length_wrapper
  , sov_sunyi_gain_wrapper
  , sov_sunyi_boundary_wrapper
  , sov_eis_mul_wrapper
  , sov_eis_norm_wrapper
  , sov_chern_detect_wrapper
  , sov_holographic_wrapper
  , sov_digital_root_wrapper
  , sov_doz_digit_wrapper
  , sov_trit_pack5_wrapper
  , sov_trit_unpack5_wrapper
  , sov_tryte_value_wrapper
  , sov_chern_inject_monopole_wrapper
  , sov_chern_compute_wrapper
  ) where

import Data.Bits    (shiftL, shiftR, (.&.), (.|.))
import Data.Int     (Int32, Int64)
import Data.Word    (Word64, Word8, Word16)
import Foreign
import Foreign.C.String (CString)
import Foreign.C.Types

import qualified Sov.Math.DuodecClock as DC
import qualified Sov.Math.Trit as T
import Sov.Math.Trit (Trit(..))
import qualified Sov.Math.WideConstants as WC
import qualified Sov.Math.WideFixed as WF
import qualified Sov.Math.SunYi as SY
import qualified Sov.Math.Eis as EIS
import qualified Sov.Math.Guard.L7L8 as L78
import qualified Sov.Math.Guard.Chern as CH
import Foreign.C.String (CString, withCString)
import Data.List (foldl')
import System.IO.Unsafe (unsafePerformIO)
import qualified Sov.Math.Vec as V
import qualified Sov.Math.Trit as T
import Sov.Math.Trit (Trit(..))

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


-- ═══ 第二批: 剩余层 ═══

sov_sunyi_length_wrapper :: CUChar -> Ptr CUShort -> IO CInt
sov_sunyi_length_wrapper tone out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral tone >= (12 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      poke out (fromIntegral (SY.twelveLengths !! fromIntegral tone))
      return 0
foreign export ccall "sov_sunyi_length" sov_sunyi_length_wrapper :: CUChar -> Ptr CUShort -> IO CInt

sov_sunyi_gain_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_sunyi_gain_wrapper step out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let g = SY.stepLossGain (fromIntegral step)
      poke out (fromIntegral (case g of SY.Sun -> 0 :: Int; SY.Yi -> 1))
      return 0
foreign export ccall "sov_sunyi_gain" sov_sunyi_gain_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_sunyi_boundary_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_sunyi_boundary_wrapper step out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if SY.isZhonglvBoundary (fromIntegral step) then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_sunyi_boundary" sov_sunyi_boundary_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_eis_mul_wrapper :: CLLong -> CLLong -> CLLong -> CLLong -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_eis_mul_wrapper a b c d re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let r = EIS.mulE (EIS.Eis (fromIntegral a) (fromIntegral b))
                       (EIS.Eis (fromIntegral c) (fromIntegral d))
      poke re (fromIntegral (EIS.eisA r))
      poke im (fromIntegral (EIS.eisB r))
      return 0
foreign export ccall "sov_eis_mul" sov_eis_mul_wrapper :: CLLong -> CLLong -> CLLong -> CLLong -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_eis_norm_wrapper :: CLLong -> CLLong -> Ptr CLLong -> IO CInt
sov_eis_norm_wrapper a b out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (fromIntegral (EIS.normE (EIS.Eis (fromIntegral a) (fromIntegral b))))
      return 0
foreign export ccall "sov_eis_norm" sov_eis_norm_wrapper :: CLLong -> CLLong -> Ptr CLLong -> IO CInt

sov_chern_detect_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_chern_detect_wrapper injected num den
  | num == nullPtr || den == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke num (if injected /= 0 then -432 else 0)
      poke den 216
      return 0
foreign export ccall "sov_chern_detect" sov_chern_detect_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_holographic_wrapper :: CUInt -> CUInt -> Ptr CInt -> IO CInt
sov_holographic_wrapper steps closure out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (fromIntegral (L78.computeHolographicApprox 0 0 (fromIntegral steps) (fromIntegral closure)))
      return 0
foreign export ccall "sov_holographic" sov_holographic_wrapper :: CUInt -> CUInt -> Ptr CInt -> IO CInt

sov_digital_root_wrapper :: CULLong -> Ptr CUChar -> IO CInt
sov_digital_root_wrapper n out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (fromIntegral (L78.digitalRoot (fromIntegral n)))
      return 0
foreign export ccall "sov_digital_root" sov_digital_root_wrapper :: CULLong -> Ptr CUChar -> IO CInt

sov_doz_digit_wrapper :: CUChar -> Ptr CChar -> IO CInt
sov_doz_digit_wrapper d out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | d' >= 12 = return (fromIntegral sovErrRange)
  | otherwise = do
      poke out (toEnum (if d' < 10 then fromEnum '0' + d' else if d' == 10 then fromEnum 'A' else fromEnum 'B'))
      return 0
  where d' = fromIntegral d :: Int
foreign export ccall "sov_doz_digit" sov_doz_digit_wrapper :: CUChar -> Ptr CChar -> IO CInt

sov_trit_pack5_wrapper :: CUChar -> CUChar -> CUChar -> CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_trit_pack5_wrapper t0 t1 t2 t3 t4 out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | any (> 2) ts = return (fromIntegral sovErrTrit)
  | otherwise = do
      poke out (fromIntegral (sum [ ts !! i * 3 ^ i | i <- [0 .. 4] ]))
      return 0
  where ts = map fromIntegral [t0, t1, t2, t3, t4] :: [Int]
foreign export ccall "sov_trit_pack5" sov_trit_pack5_wrapper :: CUChar -> CUChar -> CUChar -> CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_trit_unpack5_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_trit_unpack5_wrapper packed out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | p' > 242 = return (fromIntegral sovErrRange)
  | otherwise = do
      let ds = [81, 27, 9, 3, 1] :: [Int]
      mapM_ (\(i, dv) -> pokeElemOff out i (fromIntegral ((p' `div` dv) `mod` 3))) (zip [0 ..] ds)
      return 0
  where p' = fromIntegral packed :: Int
foreign export ccall "sov_trit_unpack5" sov_trit_unpack5_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_tryte_value_wrapper :: Ptr CUChar -> Ptr CUShort -> IO CInt
sov_tryte_value_wrapper t6 out
  | t6 == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ts <- mapM (\i -> fromIntegral <$> peekElemOff t6 i) [0 .. 5] :: IO [Int]
      if any (> 2) ts
        then return (fromIntegral sovErrTrit)
        else do
          poke out (fromIntegral (sum [ ts !! i * 3 ^ i | i <- [0 .. 5] ]))
          return 0
foreign export ccall "sov_tryte_value" sov_tryte_value_wrapper :: Ptr CUChar -> Ptr CUShort -> IO CInt


-- ── sov_strerror: 静态错误串 (与 cpp/rust 逐字一致) ──
{-# NOINLINE strerrorTable #-}
strerrorTable :: [(Int, CString)]
strerrorTable = unsafePerformIO $ mapM (\(c, m) -> withCString m (\p -> return (c, p)))
  [ (0,  "ok")
  , (-1, "illegal trit (not in {0,1,2})")
  , (-2, "range error")
  , (-3, "illegal state")
  , (-4, "null pointer")
  ]

sov_strerror_wrapper :: CInt -> IO CString
sov_strerror_wrapper code =
  case lookup (fromIntegral code) strerrorTable of
    Just p  -> return p
    Nothing -> withCString "unknown error" return
foreign export ccall "sov_strerror" sov_strerror_wrapper :: CInt -> IO CString

-- ═══ 陈数扁平缓冲接口 (864 = 12 head × 12 cell × 6 trit) ═══

-- | 注入 C=2 单极到 864 字节缓冲; 返回注入胞腔数 (72)
sov_chern_inject_monopole_wrapper :: Ptr CUChar -> CSize -> IO CInt
sov_chern_inject_monopole_wrapper buf len
  | buf == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral len < (864 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      let (proto, injected) = CH.injectC2Monopole CH.protoZero
          tritAt3 h c i = case V.indexV h (CH.protoTrits proto) >>= V.indexV c >>= V.indexV i of
                            Just t  -> fromIntegral (fromEnum t) :: Int
                            Nothing -> 0
          flat = concat [ [ tritAt3 h c i | i <- [0 .. 5] ]
                        | h <- [0 .. 11], c <- [0 .. 11] ]
      mapM_ (\(i, v) -> pokeElemOff buf i (fromIntegral v :: CUChar)) (zip [0 ..] flat)
      return (fromIntegral injected)
foreign export ccall "sov_chern_inject_monopole" sov_chern_inject_monopole_wrapper :: Ptr CUChar -> CSize -> IO CInt

-- | 从 864 字节缓冲计算精确陈数 C = num/den
sov_chern_compute_wrapper :: Ptr CUChar -> CSize -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_chern_compute_wrapper dataPtr len num den
  | dataPtr == nullPtr || num == nullPtr || den == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral len < (864 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      cells <- mapM (\i -> (fromIntegral <$> peekElemOff dataPtr i :: IO Int)) [0 .. 863]
      if any (> 2) cells
        then return (fromIntegral sovErrTrit)
        else do
          let proto = foldl' (\p (h, c) ->
                        let base = (h * 12 + c) * 6
                            t6 = take 6 (drop base cells)
                        in CH.setCell h c (toVec6 t6) p) CH.protoZero
                      [ (h, c) | h <- [0 .. 11], c <- [0 .. 11] ]
              (n, d) = CH.computeChernExact proto
          poke num (fromIntegral n)
          poke den (fromIntegral d)
          return 0
foreign export ccall "sov_chern_compute" sov_chern_compute_wrapper :: Ptr CUChar -> CSize -> Ptr CLLong -> Ptr CLLong -> IO CInt

-- | [Int] → Vec N6 Trit (GADT 推断长度 6; 元素 ∈ {0,1,2})
toVec6 xs = toTrit (xs !! 0) V.:> toTrit (xs !! 1) V.:> toTrit (xs !! 2) V.:>
            toTrit (xs !! 3) V.:> toTrit (xs !! 4) V.:> toTrit (xs !! 5) V.:> V.VNil
  where toTrit 0 = T0
        toTrit 1 = T1
        toTrit _ = T2
