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
  , sov_dc_phase_vec_wrapper
  , sov_dc_rotate_vec_wrapper
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
  , sov_sunyi_remainder_wrapper
  , sov_sunyi_gain_wrapper
  , sov_sunyi_boundary_wrapper
  , sov_eis_mul_wrapper
  , sov_eis_norm_wrapper
  , sov_chern_detect_wrapper
  , sov_holographic_wrapper
  , sov_digital_root_wrapper
  , sov_digital_root_mod9_wrapper
  , sov_doz_digit_wrapper
  , sov_trit_pack5_wrapper
  , sov_trit_unpack5_wrapper
  , sov_tryte_value_wrapper
  , sov_chern_inject_monopole_wrapper
  , sov_chern_compute_wrapper
    -- 第三批
  , sov_adc64_wrapper
  , sov_zhonglv_step_wrapper
  , sov_zhonglv_closure_wrapper
  , sov_sovereign_lcm_wrapper
  , sov_t6_index_wrapper
  , sov_is_standing_node_wrapper
  , sov_chiral_collapse_wrapper
    -- 第四批
  , sov_format_doz_wrapper
  , sov_parse_doz_wrapper
  , sov_tryte_unpack_wrapper
    -- 第五批
  , sov_soliton_sin_q32_wrapper
  , sov_soliton_rho_q16_wrapper
  , sov_soliton_phase_wrapper
    -- 第六批
  , sov_u384_add_wrapper
  , sov_u384_mul_u64_wrapper
  , sov_u384_shr16_wrapper
    -- 第七批
  , sov_nse_new_wrapper
  , sov_nse_step_wrapper
  , sov_nse_density_q16_wrapper
  , sov_nse_step_count_wrapper
  , sov_nse_cell_wrapper
  , sov_nse_component_wrapper
  , sov_nse_set_component_wrapper
  , sov_nse_free_wrapper
    -- 第八批
  , sov_dayan_multiplier_wrapper
  , sov_crt2_wrapper
    -- 第十批
  , sov_width_capacity_trits_wrapper
  , sov_width_capacity_bits_wrapper
  , sov_width_trits_for_bits_wrapper
  , sov_width_bits_for_trits_wrapper
  ) where

import Data.Bits    (shiftL, shiftR, (.&.), (.|.))
import qualified Data.Bits
import qualified Data.Bits as Data.Bits
import Data.Int     (Int32, Int64)
import Data.Word    (Word64, Word8, Word16)
import Foreign
import Foreign.C.String (CString)
import Foreign.C.Types
import Unsafe.Coerce (unsafeCoerce)

import qualified Sov.Math.DuodecClock as DC
import qualified Sov.Math.Trit as T
import Sov.Math.Trit (Trit(..))
import qualified Sov.Math.WideConstants as WC
import qualified Sov.Math.WideFixed as WF
import qualified Sov.Math.SunYi as SY
import qualified Sov.Math.Eis as EIS
import qualified Sov.Math.Guard.L7L8 as L78
import qualified Sov.Math.Guard.Chern as CH
-- ABI 1.1: A₄ / 2A₄ 族
import qualified Sov.Math.A4 as A4
import qualified Sov.Math.SL23 as SL23
import qualified Sov.Math.Cover2A4 as CV
import qualified Sov.Math.RepA4 as RA4
import qualified Sov.Math.Rep2A4 as R2
import qualified Sov.Math.Hopf as HOPF
import qualified Sov.Math.SpinHopf as SH
import qualified Sov.Math.Spin2 as S2
import qualified Sov.Math.Spin3 as S3
import qualified Sov.Math.CarrierA4 as CA4
import qualified Sov.Math.Carrier2A4 as K2
import qualified Sov.Math.Field as FD
import qualified Sov.Math.Tryte as TRY
import qualified Sov.Math.Topology.Soliton as SOL
import Foreign.C.String (CString, withCString)
import Data.List (foldl')
import System.IO.Unsafe (unsafePerformIO)
import qualified Data.Set as Set
import Data.IORef (IORef, newIORef, readIORef, writeIORef, modifyIORef')
import Foreign.StablePtr (StablePtr, newStablePtr, deRefStablePtr, freeStablePtr)
import Data.Array.Unboxed (UArray, listArray, (!), elems, (//))
import qualified Sov.Math.Vec as V
import qualified Sov.Math.Trit as T
import Sov.Math.Trit (Trit(..))

-- ── 状态码 ──
sovOk, sovErrTrit, sovErrRange, sovErrState, sovErrNull :: Int32
sovOk       = 0
sovErrTrit  = -1
sovErrRange = -2
sovErrState = -3

-- | 活句柄注册表: 使 sov_nse_free 可重复调用 (第二次返回 sovErrState, 非 UB)
{-# NOINLINE nseLive #-}
nseLive :: IORef (Set.Set WordPtr)
nseLive = unsafePerformIO (newIORef Set.empty)
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
sov_abi_version_wrapper = return (fromIntegral (((1 :: Int) `shiftL` 16) .|. 1))

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

-- ── 相位几何表示 (ℓ=1 实球谐双峰, 精确整数) ──
sov_dc_phase_vec_wrapper :: CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_dc_phase_vec_wrapper k x y
  | x == nullPtr || y == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral k > (3 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      let vx = [1, 0, 2, 0] :: [Int]   -- GF(3): 2 ≡ −1
          vy = [0, 1, 0, 2] :: [Int]
      poke x (fromIntegral (vx !! fromIntegral k))
      poke y (fromIntegral (vy !! fromIntegral k))
      return 0
foreign export ccall "sov_dc_phase_vec" sov_dc_phase_vec_wrapper :: CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_dc_rotate_vec_wrapper :: CUChar -> CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_dc_rotate_vec_wrapper k x y xo yo
  | xo == nullPtr || yo == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral k > (3 :: Int) = return (fromIntegral sovErrRange)
  | fromIntegral x > (2 :: Int) || fromIntegral y > (2 :: Int) = return (fromIntegral sovErrTrit)
  | otherwise = do
      let rotN 0 p = p
          rotN n (a, b) = rotN (n - 1) ((2 * b) `mod` 3, a)   -- R = [[0,2],[1,0]] in GF(3)
          (a', b') = rotN (fromIntegral k) (fromIntegral x, fromIntegral y)
      poke xo (fromIntegral a' :: CUChar)
      poke yo (fromIntegral b' :: CUChar)
      return 0
foreign export ccall "sov_dc_rotate_vec" sov_dc_rotate_vec_wrapper :: CUChar -> CUChar -> CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

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

-- | LCM 余数投影 (Agda SunyiChain.LCM_REMAINDERS, exact × 3^7)
sov_sunyi_remainder_wrapper :: CUChar -> Ptr CUInt -> IO CInt
sov_sunyi_remainder_wrapper tone out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral tone >= (12 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      poke out (fromIntegral (remainderTable !! fromIntegral tone))
      return 0
  where remainderTable = [177147,118098,157464,104976,139968,93312,
                          124416,82944,110592,73728,98304,65536] :: [Word32]
foreign export ccall "sov_sunyi_remainder" sov_sunyi_remainder_wrapper :: CUChar -> Ptr CUInt -> IO CInt

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

-- | 模 9 数字根 (与 Agda Foundation/DigitalRoot.digitalRoot 逐点相同: n % 9)
sov_digital_root_mod9_wrapper :: CULLong -> Ptr CUChar -> IO CInt
sov_digital_root_mod9_wrapper n out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral (n `mod` 9) :: CUChar); return 0
foreign export ccall "sov_digital_root_mod9" sov_digital_root_mod9_wrapper :: CULLong -> Ptr CUChar -> IO CInt

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


-- ═══ 第三批 ═══

-- | 64 位加法 + 进位链
sov_adc64_wrapper :: CULLong -> CULLong -> CUChar -> Ptr CULLong -> Ptr CUChar -> IO CInt
sov_adc64_wrapper a b carryIn sumOut carryOut
  | sumOut == nullPtr || carryOut == nullPtr = return (fromIntegral sovErrNull)
  | carryIn > 1 = return (fromIntegral sovErrRange)
  | otherwise = do
      let wa = fromIntegral a :: Word64
          wb = fromIntegral b :: Word64
          wc = fromIntegral carryIn :: Word64
          s1 = wa + wb
          c1 = if s1 < wa then 1 else 0 :: Word8
          s2 = s1 + wc
          c2 = if s2 < s1 then 1 else 0 :: Word8
      poke sumOut (fromIntegral s2)
      poke carryOut (fromIntegral (c1 Data.Bits..|. c2))
      return 0
foreign export ccall "sov_adc64" sov_adc64_wrapper :: CULLong -> CULLong -> CUChar -> Ptr CULLong -> Ptr CUChar -> IO CInt

-- | 单次仲吕闭合 (Agda FixedPointQ16.zhonglvClosure)
sov_zhonglv_closure_wrapper :: CULLong -> Ptr CULLong -> IO CInt
sov_zhonglv_closure_wrapper acc out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let q = (toInteger acc * 177147) `div` 65536
      if q > 18446744073709551615 then return (fromIntegral sovErrRange)
        else do poke out (fromIntegral q :: CULLong); return 0
foreign export ccall "sov_zhonglv_closure" sov_zhonglv_closure_wrapper :: CULLong -> Ptr CULLong -> IO CInt

-- | SOVEREIGN_LCM = 3^11 × 2^16
sov_sovereign_lcm_wrapper :: Ptr CULLong -> IO CInt
sov_sovereign_lcm_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out 11609505792; return 0
foreign export ccall "sov_sovereign_lcm" sov_sovereign_lcm_wrapper :: Ptr CULLong -> IO CInt

-- | 仲吕步进: 8 次 a ← (a×177147)/2^16, 每次对 lcm 取模
sov_zhonglv_step_wrapper :: CLLong -> CLLong -> Ptr CLLong -> IO CInt
sov_zhonglv_step_wrapper acc lcm out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | lcm <= 0 = return (fromIntegral sovErrRange)
  | otherwise = do
      let l = toInteger lcm
          step a = let a' = (a * 177147) `quot` 65536 in ((a' `mod` l) + l) `mod` l
          r = iterate step (toInteger acc) !! 8
      poke out (fromIntegral r)
      return 0
foreign export ccall "sov_zhonglv_step" sov_zhonglv_step_wrapper :: CLLong -> CLLong -> Ptr CLLong -> IO CInt

-- | T⁶ 环面索引 (144 × 46)
sov_t6_index_wrapper :: CUInt -> CUInt -> Ptr CUShort -> IO CInt
sov_t6_index_wrapper p t out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (fromIntegral (((fromIntegral p `mod` 144) * 46) + (fromIntegral t `mod` 46) :: Int))
      return 0
foreign export ccall "sov_t6_index" sov_t6_index_wrapper :: CUInt -> CUInt -> Ptr CUShort -> IO CInt

-- | 驻波节点 (tone 是 3 的倍数)
sov_is_standing_node_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_is_standing_node_wrapper tone out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if fromIntegral tone `mod` (3 :: Int) == 0 then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_is_standing_node" sov_is_standing_node_wrapper :: CUChar -> Ptr CUChar -> IO CInt

-- | 手性扭矩坍缩 (阈值 113506 = 2×|T_max|)
sov_chiral_collapse_wrapper :: CInt -> Ptr CUChar -> IO CInt
sov_chiral_collapse_wrapper torque out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let tv = fromIntegral torque :: Int
      poke out (if tv > 113506 || tv < -113506 then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_chiral_collapse" sov_chiral_collapse_wrapper :: CInt -> Ptr CUChar -> IO CInt


-- ═══ 第四批: Doz 记数法 + Tryte 解包 ═══

dozChar :: Integer -> Char
dozChar d | d < 10 = toEnum (fromEnum '0' + fromIntegral d)
          | d == 10 = 'A'
          | otherwise = 'B'

-- | 无符号 → Doz 字符串 (高位在前, NUL 结尾, written 回传字符数)
sov_format_doz_wrapper :: CULLong -> Ptr CChar -> CSize -> Ptr CSize -> IO CInt
sov_format_doz_wrapper v buf cap written
  | buf == nullPtr || written == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let digits = reverse (go (toInteger v) :: [Char])
          n = length digits
      if fromIntegral cap < n + 1
        then return (fromIntegral sovErrRange)
        else do
          mapM_ (\(i, c) -> pokeElemOff buf i (toEnum (fromEnum c))) (zip [0 ..] digits)
          pokeElemOff buf n (toEnum 0)
          poke written (fromIntegral n)
          return 0
  where go x | x < 12 = [dozChar x]
             | otherwise = dozChar (x `mod` 12) : go (x `quot` 12)
foreign export ccall "sov_format_doz" sov_format_doz_wrapper :: CULLong -> Ptr CChar -> CSize -> Ptr CSize -> IO CInt

-- | Doz 字符串 → 无符号 (拒绝小写/非法字符/空串/溢出)
sov_parse_doz_wrapper :: Ptr CChar -> CSize -> Ptr CULLong -> IO CInt
sov_parse_doz_wrapper buf len out
  | buf == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | len == 0 = return (fromIntegral sovErrRange)
  | otherwise = do
      cs <- mapM (\i -> toEnum . fromIntegral <$> (peekElemOff buf i :: IO CChar)) [0 .. fromIntegral len - 1]
      let dv c | c >= '0' && c <= '9' = Just (toInteger (fromEnum c - fromEnum '0'))
               | c == 'A' = Just 10
               | c == 'B' = Just 11
               | otherwise = Nothing
          step (Just acc) c = case dv c of
                    Just d | acc <= (18446744073709551615 - d) `quot` 12 -> Just (acc * 12 + d)
                    _ -> Nothing
          step Nothing _ = Nothing
      case foldl step (Just 0) cs of
        Nothing -> return (fromIntegral sovErrRange)
        Just acc -> do
          poke out (fromIntegral acc)
          return 0
foreign export ccall "sov_parse_doz" sov_parse_doz_wrapper :: Ptr CChar -> CSize -> Ptr CULLong -> IO CInt

-- | Tryte 值 → 6 trit (小端)
sov_tryte_unpack_wrapper :: CUShort -> Ptr CUChar -> IO CInt
sov_tryte_unpack_wrapper v t6
  | t6 == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral v >= (729 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      mapM_ (\(i, d) -> pokeElemOff t6 i (fromIntegral d :: CUChar))
            (zip [0 ..] (take 6 (go (fromIntegral v))))
      return 0
  where go 0 = repeat (0 :: Int)
        go x = fromIntegral (x `mod` 3) : go (x `quot` 3)
foreign export ccall "sov_tryte_unpack" sov_tryte_unpack_wrapper :: CUShort -> Ptr CUChar -> IO CInt


-- ═══ 第五批: 孤子相变 (L5) ═══

-- | Q32 整数正弦原语 (复用 Soliton.sinQ32; 定义域 [0, π/2×2^32])
sov_soliton_sin_q32_wrapper :: CLLong -> Ptr CLLong -> IO CInt
sov_soliton_sin_q32_wrapper x out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | x' < 0 || x' > 6746518852 = return (fromIntegral sovErrRange)
  | otherwise = do
      poke out (fromIntegral (SOL.sinQ32 (toInteger x')))
      return 0
  where x' = fromIntegral x :: Integer
foreign export ccall "sov_soliton_sin_q32" sov_soliton_sin_q32_wrapper :: CLLong -> Ptr CLLong -> IO CInt

-- | ρ(s) 的 Q16 定点 (s ≥ 4500 → 65536)
sov_soliton_rho_q16_wrapper :: CUInt -> Ptr CInt -> IO CInt
sov_soliton_rho_q16_wrapper step out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (fromIntegral (SOL.solitonRhoQ16Approx (fromIntegral step)))
      return 0
foreign export ccall "sov_soliton_rho_q16" sov_soliton_rho_q16_wrapper :: CUInt -> Ptr CInt -> IO CInt

-- | 相态判定 (阈值 24904 / 49152 / 65536 → 0 / 2 / 1 / 3, 沿用 cpp/rust 既有语义)
sov_soliton_phase_wrapper :: CInt -> Ptr CUChar -> IO CInt
sov_soliton_phase_wrapper rho out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let r = fromIntegral rho :: Int
          p | r < 24904 = 0
            | r < 49152 = 2
            | r < 65536 = 1
            | otherwise = 3
      poke out (fromIntegral p :: CUChar)
      return 0
foreign export ccall "sov_soliton_phase" sov_soliton_phase_wrapper :: CInt -> Ptr CUChar -> IO CInt


-- ═══ 第六批: 384 位定点原语 ═══

readU384 :: Ptr CULLong -> IO [Word64]
readU384 p = mapM (\i -> fromIntegral <$> (peekElemOff p i :: IO CULLong)) [0 .. 5]

writeU384 :: Ptr CULLong -> [Word64] -> IO ()
writeU384 p xs = mapM_ (\(i, v) -> pokeElemOff p i (fromIntegral v :: CULLong)) (zip [0 ..] xs)

-- | out = a + b (mod 2^384), carry_out = 溢出位
sov_u384_add_wrapper :: Ptr CULLong -> Ptr CULLong -> Ptr CULLong -> Ptr CUChar -> IO CInt
sov_u384_add_wrapper a b out carryOut
  | a == nullPtr || b == nullPtr || out == nullPtr || carryOut == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readU384 a; bv <- readU384 b
      let go [] [] c = ([], c)
          go (x:xs) (y:ys) c =
            let t = toInteger x + toInteger y + toInteger c
                lo = fromIntegral (t `mod` 18446744073709551616)
                hi = t `quot` 18446744073709551616
            in let (rest, cf) = go xs ys hi in (lo : rest, cf)
          (sums, carry) = go av bv (0 :: Integer)
      writeU384 out sums
      poke carryOut (fromIntegral carry :: CUChar)
      return 0
foreign export ccall "sov_u384_add" sov_u384_add_wrapper :: Ptr CULLong -> Ptr CULLong -> Ptr CULLong -> Ptr CUChar -> IO CInt

-- | out = a × k (mod 2^384), carry_out = 溢出到 2^384 之上的高 64 位
sov_u384_mul_u64_wrapper :: Ptr CULLong -> CULLong -> Ptr CULLong -> Ptr CULLong -> IO CInt
sov_u384_mul_u64_wrapper a k out carryOut
  | a == nullPtr || out == nullPtr || carryOut == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readU384 a
      let kk = toInteger (fromIntegral k :: Word64)
          go [] c = ([], c)
          go (x:xs) c =
            let t = toInteger x * kk + toInteger c
                lo = fromIntegral (t `mod` 18446744073709551616)
                hi = t `quot` 18446744073709551616
            in let (rest, cf) = go xs hi in (lo : rest, cf)
          (prod, carry) = go av (0 :: Integer)
      writeU384 out prod
      poke carryOut (fromIntegral carry :: CULLong)
      return 0
foreign export ccall "sov_u384_mul_u64" sov_u384_mul_u64_wrapper :: Ptr CULLong -> CULLong -> Ptr CULLong -> Ptr CULLong -> IO CInt

-- | out = a >> 16 (跨 limb 整体右移)
sov_u384_shr16_wrapper :: Ptr CULLong -> Ptr CULLong -> IO CInt
sov_u384_shr16_wrapper a out
  | a == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      av <- readU384 a
      let shifted = [ (av !! i) `shiftR` 16 .|. (if i + 1 < 6 then (av !! (i + 1)) `shiftL` 48 else 0)
                    | i <- [0 .. 5] ]
      writeU384 out shifted
      return 0
foreign export ccall "sov_u384_shr16" sov_u384_shr16_wrapper :: Ptr CULLong -> Ptr CULLong -> IO CInt


-- ═══ 第七批: NSE 句柄式 ABI (唯一有分配语义的设施) ═══

data NseState = NseState
  { nseCells :: UArray Int Word8     -- mode 0/1: 6624
  , nseComp  :: UArray Int Word8     -- mode 2: 6×729 (布局 i*729 + x)
  , nseStep  :: Word32
  , nseMode  :: Word8
  }

type NsePtr = StablePtr (IORef NseState)

nseNonzero :: UArray Int Word8 -> Int
nseNonzero a = length (filter (/= 0) (elems a))

nseRhoQ16 :: UArray Int Word8 -> Int
nseRhoQ16 a = nseNonzero a * 65536 `div` 6624

sov_nse_new_wrapper :: CUChar -> Ptr (Ptr ()) -> IO CInt
sov_nse_new_wrapper mode out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral mode > (2 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      ref <- newIORef (NseState
              (listArray (0, 6623) (replicate 6624 0))
              (listArray (0, 4373) (replicate 4374 0))
              0 (fromIntegral mode))
      sp <- newStablePtr ref
      modifyIORef' nseLive (Set.insert (ptrToWordPtr (castPtr (unsafeCoerce sp :: Ptr ()))))
      poke (castPtr out) (castPtr (unsafeCoerce sp :: Ptr ()))
      return 0
foreign export ccall "sov_nse_new" sov_nse_new_wrapper :: CUChar -> Ptr (Ptr ()) -> IO CInt

sov_nse_step_wrapper :: Ptr () -> IO CInt
sov_nse_step_wrapper h
  | h == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      if nseMode st == 2
        then do
          writeIORef ref st { nseComp = formalisedStep (nseComp st)
                            , nseStep = nseStep st + 1 }
          return 0
        else do
          writeIORef ref st { nseCells = pumpStep st
                            , nseStep = nseStep st + 1 }
          return 0

-- | mode 0/1 的启发式泵浦一步
pumpStep :: NseState -> UArray Int Word8
pumpStep st = listArray (0, 6623) [ cellAt p t | p <- [0 .. 143], t <- [0 .. 45] ]
  where cells = nseCells st
        s = nseStep st
        mode = nseMode st
        viscous = nseRhoQ16 cells >= 24904
        cellAt p t =
          let idx = p * 46 + t
              old = fromIntegral (cells ! idx) :: Int
              par = if mode == 1 then fromIntegral s + p + t else fromIntegral s
              nv0 = (old + (if even par then 1 else 2)) `mod` 3
              c3 = (fromIntegral s + p * 7 + t * 3) `mod` 1500
              nv | not viscous = nv0
                 | c3 < 500 = (nv0 + 1) `mod` 3
                 | c3 < 1000 = (nv0 + 2) `mod` 3
                 | otherwise = nv0
          in fromIntegral nv :: Word8

foreign export ccall "sov_nse_step" sov_nse_step_wrapper :: Ptr () -> IO CInt

sov_nse_density_q16_wrapper :: Ptr () -> Ptr CInt -> IO CInt
sov_nse_density_q16_wrapper h out
  | h == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      poke out (fromIntegral (nseRhoQ16 (nseCells st)))
      return 0
foreign export ccall "sov_nse_density_q16" sov_nse_density_q16_wrapper :: Ptr () -> Ptr CInt -> IO CInt

sov_nse_step_count_wrapper :: Ptr () -> Ptr CUInt -> IO CInt
sov_nse_step_count_wrapper h out
  | h == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      poke out (fromIntegral (nseStep st))
      return 0
foreign export ccall "sov_nse_step_count" sov_nse_step_count_wrapper :: Ptr () -> Ptr CUInt -> IO CInt

sov_nse_cell_wrapper :: Ptr () -> CUShort -> CUShort -> Ptr CUChar -> IO CInt
sov_nse_cell_wrapper h p t out
  | h == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral p >= (144 :: Int) || fromIntegral t >= (46 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      poke out (fromIntegral ((nseCells st) ! (fromIntegral p * 46 + fromIntegral t)))
      return 0
foreign export ccall "sov_nse_cell" sov_nse_cell_wrapper :: Ptr () -> CUShort -> CUShort -> Ptr CUChar -> IO CInt

sov_nse_free_wrapper :: Ptr () -> IO CInt
sov_nse_free_wrapper h
  | h == nullPtr = return 0     -- free(NULL) 合法且无操作
  | otherwise = do
      let key = ptrToWordPtr (castPtr h)
      live <- readIORef nseLive
      if not (Set.member key live)
        then return (fromIntegral sovErrState)      -- 重复释放/外来指针
        else do
          modifyIORef' nseLive (Set.delete key)
          freeStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
          return 0
foreign export ccall "sov_nse_free" sov_nse_free_wrapper :: Ptr () -> IO CInt


-- ── 形式化 nsStep (Agda NSEOnT6: nsStep v = v +F grad (div v)) ──
taddH :: Word8 -> Word8 -> Word8
taddH a b = fromIntegral ((fromIntegral a + fromIntegral b) `mod` (3 :: Int))

tnegH :: Word8 -> Word8
tnegH a = fromIntegral ((3 - fromIntegral a) `mod` (3 :: Int))

shiftAtH :: Int -> Int -> Int
shiftAtH i x = x + ((digit + 1) `mod` 3) * base - digit * base
  where base = 3 ^ i
        digit = (x `div` base) `mod` 3

formalisedStep :: UArray Int Word8 -> UArray Int Word8
formalisedStep c = listArray (0, 4373) [ nxt i x | i <- [0 .. 5], x <- [0 .. 728] ]
  where
    n = 729
    at x i = c ! (i * n + x)
    diffF axis comp x = taddH (at (shiftAtH axis x) comp) (tnegH (at x comp))
    d = [ foldl taddH 0 [ diffF (i `mod` 3) i x | i <- [0 .. 5] ] | x <- [0 .. 728] ]
    nxt i x = let axis = i `mod` 3
                  gradD = taddH (d !! shiftAtH axis x) (tnegH (d !! x))
              in taddH (at x i) gradD


sov_nse_component_wrapper :: Ptr () -> CUShort -> CUChar -> Ptr CUChar -> IO CInt
sov_nse_component_wrapper h x i out
  | h == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      if nseMode st /= 2 then return (fromIntegral sovErrState)
        else if fromIntegral x >= (729 :: Int) || fromIntegral i >= (6 :: Int)
          then return (fromIntegral sovErrRange)
          else do poke out (fromIntegral (nseComp st ! (fromIntegral i * 729 + fromIntegral x)) :: CUChar); return 0
foreign export ccall "sov_nse_component" sov_nse_component_wrapper :: Ptr () -> CUShort -> CUChar -> Ptr CUChar -> IO CInt

sov_nse_set_component_wrapper :: Ptr () -> CUShort -> CUChar -> CUChar -> IO CInt
sov_nse_set_component_wrapper h x i v
  | h == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      ref <- deRefStablePtr (unsafeCoerce (castPtr h) :: NsePtr)
      st <- readIORef ref
      if nseMode st /= 2 then return (fromIntegral sovErrState)
        else if fromIntegral x >= (729 :: Int) || fromIntegral i >= (6 :: Int)
          then return (fromIntegral sovErrRange)
          else if fromIntegral v > (2 :: Int) then return (fromIntegral sovErrTrit)
          else do
            let arr = nseComp st
                idx = fromIntegral i * 729 + fromIntegral x
            writeIORef ref st { nseComp = arr // [(idx, fromIntegral v)] }
            return 0
foreign export ccall "sov_nse_set_component" sov_nse_set_component_wrapper :: Ptr () -> CUShort -> CUChar -> CUChar -> IO CInt


-- ═══ 第八批: 大衍求一术 (带符号系数) + 广义 CRT ═══

gcdU :: Integer -> Integer -> Integer
gcdU a 0 = abs a
gcdU a b = gcdU b (a `mod` b)

-- | 秦九韶递互除乘 (系数 ℤ): (lt,lb,rt,rb) → (lb − q·lt, lt, r, rt)
dayanSigned :: Integer -> Integer -> Maybe Integer
dayanSigned qi ding = go 1 0 qi ding
  where
    go lt lb rt rb
      | rt == 0 = Nothing
      | rt == 1 = Just (lt `mod` ding)
      | otherwise =
          let q = rb `div` rt
              r = rb `mod` rt
          in go (lb - q * lt) lt r rt

sov_dayan_multiplier_wrapper :: CUInt -> CUInt -> Ptr CUInt -> IO CInt
sov_dayan_multiplier_wrapper qi ding out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | ding == 0 = return (fromIntegral sovErrRange)
  | otherwise =
      case dayanSigned (toInteger qi `mod` toInteger ding) (toInteger ding) of
        Nothing -> return (fromIntegral sovErrState)
        Just k  -> do poke out (fromIntegral k); return 0
foreign export ccall "sov_dayan_multiplier" sov_dayan_multiplier_wrapper :: CUInt -> CUInt -> Ptr CUInt -> IO CInt

sov_crt2_wrapper :: CUInt -> CUInt -> CUInt -> CUInt -> Ptr CUInt -> IO CInt
sov_crt2_wrapper a1 m1 a2 m2 out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | m1 == 0 || m2 == 0 = return (fromIntegral sovErrRange)
  | gcdU (toInteger m1) (toInteger m2) /= 1 = return (fromIntegral sovErrState)
  | otherwise =
      case dayanSigned (toInteger m1 `mod` toInteger m2) (toInteger m2) of
        Nothing -> return (fromIntegral sovErrState)
        Just inv -> do
          let m2' = toInteger m2
              diff = ((toInteger a2 - toInteger a1) `mod` m2' + m2') `mod` m2'
              t = (diff * inv) `mod` m2'
              prod = toInteger m1 * m2'
              x = ((toInteger a1 + toInteger m1 * t) `mod` prod + prod) `mod` prod
          poke out (fromIntegral x)
          return 0
foreign export ccall "sov_crt2" sov_crt2_wrapper :: CUInt -> CUInt -> CUInt -> CUInt -> Ptr CUInt -> IO CInt


-- ═══ 第十批: 位宽转换 (二进制 ⇄ 三进制; Agda DigitField 纯容量算术) ═══

powU :: Integer -> Int -> Integer
powU b e = b ^ e

sov_width_capacity_trits_wrapper :: CUChar -> Ptr CULLong -> IO CInt
sov_width_capacity_trits_wrapper trits out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral trits > (40 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do poke out (fromIntegral (powU 3 (fromIntegral trits)) :: CULLong); return 0
foreign export ccall "sov_width_capacity_trits" sov_width_capacity_trits_wrapper :: CUChar -> Ptr CULLong -> IO CInt

sov_width_capacity_bits_wrapper :: CUChar -> Ptr CULLong -> IO CInt
sov_width_capacity_bits_wrapper bits out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral bits > (63 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do poke out (fromIntegral (2 ^ (fromIntegral bits :: Integer)) :: CULLong); return 0
foreign export ccall "sov_width_capacity_bits" sov_width_capacity_bits_wrapper :: CUChar -> Ptr CULLong -> IO CInt

sov_width_trits_for_bits_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_width_trits_for_bits_wrapper bits out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral bits > (63 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      let cap = 2 ^ (fromIntegral bits :: Integer) :: Integer
          k = length (takeWhile (\p -> p <= cap) (iterate (* 3) 1)) - 1
      poke out (fromIntegral k :: CUChar)
      return 0
foreign export ccall "sov_width_trits_for_bits" sov_width_trits_for_bits_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_width_bits_for_trits_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_width_bits_for_trits_wrapper trits out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | fromIntegral trits > (40 :: Int) = return (fromIntegral sovErrRange)
  | otherwise = do
      let need = 3 ^ (fromIntegral trits :: Integer) :: Integer
          b = length (takeWhile (\c -> c < need) (iterate (* 2) 1))
      poke out (fromIntegral b :: CUChar)
      return 0
foreign export ccall "sov_width_bits_for_trits" sov_width_bits_for_trits_wrapper :: CUChar -> Ptr CUChar -> IO CInt

-- ═══════════════════════════════════════════════════════════════════════
-- 第十一批（ABI 1.1）：A₄ / 2A₄ / 覆盖 / 表示论 / Hopf 商 / 载体
-- ═══════════════════════════════════════════════════════════════════════

idxMay :: [a] -> Int -> Maybe a
idxMay [] _ = Nothing
idxMay (x : _) 0 = Just x
idxMay (_ : xs) k = idxMay xs (k - 1)

a4At :: CUChar -> Maybe A4.A4
a4At v = idxMay A4.allA4 (fromIntegral v)

sl23At :: Ptr CUChar -> IO (Maybe SL23.M2)
sl23At p = do
  bs <- peekArray 4 p
  let vs = map (fromIntegral :: CUChar -> Int) bs
  case vs of
    [a, b, c, d]
      | a < 3 && b < 3 && c < 3 && d < 3 ->
          return (Just (SL23.M2 (toT a) (toT b) (toT c) (toT d)))
    _ -> return Nothing
  where
    toT k = T.fromTritIndex (fromIntegral k)

isT1 :: Trit -> Bool
isT1 t = t == T1

sl23Member :: SL23.M2 -> Bool
sl23Member m = isT1 (SL23.matDet m)

m2Bytes :: SL23.M2 -> [Word8]
m2Bytes (SL23.M2 a b c d) = map T.toTritIndex [a, b, c, d]

pokeBytes :: Ptr CUChar -> [Word8] -> IO ()
pokeBytes p vs = pokeArray p (map (fromIntegral :: Word8 -> CUChar) (take 4 (vs ++ repeat 0)))

pokeEis :: Ptr CLLong -> Ptr CLLong -> EIS.Eis -> IO ()
pokeEis re im e = do
  poke re (fromIntegral (EIS.eisA e) :: CLLong)
  poke im (fromIntegral (EIS.eisB e) :: CLLong)

irrepAt :: CUChar -> Maybe RA4.Irrep
irrepAt v = case fromIntegral v :: Int of
  0 -> Just RA4.R1
  1 -> Just RA4.R1'
  2 -> Just RA4.R1''
  3 -> Just RA4.R3
  _ -> Nothing

-- ── A₄ 群 ─────────────────────────────────────────────────────────────

sov_a4_card_wrapper :: Ptr CUChar -> IO CInt
sov_a4_card_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral A4.a4Card :: CUChar); return 0
foreign export ccall "sov_a4_card" sov_a4_card_wrapper :: Ptr CUChar -> IO CInt

sov_a4_perm_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_a4_perm_wrapper elem out4
  | out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case a4At elem of
      Nothing -> return (fromIntegral sovErrRange)
      Just g -> do pokeBytes out4 (map (fromIntegral :: Int -> Word8) (A4.a4Perm g)); return 0
foreign export ccall "sov_a4_perm" sov_a4_perm_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_a4_index_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_a4_index_wrapper perm4 out
  | perm4 == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      bs <- peekArray 4 perm4
      let ps = map (fromIntegral :: CUChar -> Int) bs
          isPerm = length ps == 4 && all (\k -> k `elem` ps) [0, 1, 2, 3]
          inv = length [ () | (i, x) <- zip [0 ..] ps, (j, y) <- zip [0 ..] ps, i < j, x > y ]
      if not isPerm || odd inv
        then return (fromIntegral sovErrRange)
        else do poke out (fromIntegral (A4.a4Index (A4.a4OfPerm ps)) :: CUChar); return 0
foreign export ccall "sov_a4_index" sov_a4_index_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_a4_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt
sov_a4_mul_wrapper g h out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case (a4At g, a4At h) of
      (Just ga, Just ha) -> do poke out (fromIntegral (A4.a4Index (A4.mulA4 ga ha)) :: CUChar); return 0
      _ -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_mul" sov_a4_mul_wrapper :: CUChar -> CUChar -> Ptr CUChar -> IO CInt

sov_a4_inv_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_a4_inv_wrapper g out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case a4At g of
      Just ga -> do poke out (fromIntegral (A4.a4Index (A4.invA4 ga)) :: CUChar); return 0
      Nothing -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_inv" sov_a4_inv_wrapper :: CUChar -> Ptr CUChar -> IO CInt

sov_a4_order_wrapper :: CUChar -> Ptr CUChar -> IO CInt
sov_a4_order_wrapper g out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case a4At g of
      Just ga -> do poke out (fromIntegral (a4OrderLocal ga) :: CUChar); return 0
      Nothing -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_order" sov_a4_order_wrapper :: CUChar -> Ptr CUChar -> IO CInt

a4OrderLocal :: A4.A4 -> Int
a4OrderLocal g = go 1 g
  where
    go k x
      | x == A4.idA4 = k
      | k > 12 = 0
      | otherwise = go (k + 1) (A4.mulA4 x g)

-- ── SL(2,F₃) ──────────────────────────────────────────────────────────

sov_sl23_card_wrapper :: Ptr CUShort -> IO CInt
sov_sl23_card_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral SL23.sl23Card :: CUShort); return 0
foreign export ccall "sov_sl23_card" sov_sl23_card_wrapper :: Ptr CUShort -> IO CInt

sov_sl23_mul_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_sl23_mul_wrapper g4 h4 out4
  | g4 == nullPtr || h4 == nullPtr || out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      mh <- sl23At h4
      case (mg, mh) of
        (Just g, Just h) -> do pokeBytes out4 (m2Bytes (SL23.matMul g h)); return 0
        _ -> return (fromIntegral sovErrTrit)
foreign export ccall "sov_sl23_mul" sov_sl23_mul_wrapper :: Ptr CUChar -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_sl23_det_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_sl23_det_wrapper g4 out
  | g4 == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      case mg of
        Just g -> do poke out (fromIntegral (T.toTritIndex (SL23.matDet g)) :: CUChar); return 0
        Nothing -> return (fromIntegral sovErrTrit)
foreign export ccall "sov_sl23_det" sov_sl23_det_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_sl23_inv_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_sl23_inv_wrapper g4 out4
  | g4 == nullPtr || out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      case mg of
        Nothing -> return (fromIntegral sovErrTrit)
        Just g
          | not (sl23Member g) -> return (fromIntegral sovErrRange)
          | otherwise -> do pokeBytes out4 (m2Bytes (SL23.matInv g)); return 0
foreign export ccall "sov_sl23_inv" sov_sl23_inv_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_sl23_pow_wrapper :: Ptr CUChar -> CUInt -> Ptr CUChar -> IO CInt
sov_sl23_pow_wrapper g4 n out4
  | g4 == nullPtr || out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      case mg of
        Nothing -> return (fromIntegral sovErrTrit)
        Just g
          | not (sl23Member g) -> return (fromIntegral sovErrRange)
          | otherwise -> do pokeBytes out4 (m2Bytes (SL23.matPow (fromIntegral n) g)); return 0
foreign export ccall "sov_sl23_pow" sov_sl23_pow_wrapper :: Ptr CUChar -> CUInt -> Ptr CUChar -> IO CInt

sov_sl23_order_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_sl23_order_wrapper g4 out
  | g4 == nullPtr || out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      case mg of
        Nothing -> return (fromIntegral sovErrTrit)
        Just g
          | not (sl23Member g) -> return (fromIntegral sovErrRange)
          | otherwise -> do poke out (fromIntegral (SL23.orderOf g) :: CUChar); return 0
foreign export ccall "sov_sl23_order" sov_sl23_order_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_sl23_order4_count_wrapper :: Ptr CUChar -> IO CInt
sov_sl23_order4_count_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral (length SL23.elemsOfOrder4) :: CUChar); return 0
foreign export ccall "sov_sl23_order4_count" sov_sl23_order4_count_wrapper :: Ptr CUChar -> IO CInt

sov_sl23_w_is_neg_i_wrapper :: Ptr CUChar -> IO CInt
sov_sl23_w_is_neg_i_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if SL23.wElement == SL23.negI then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_sl23_w_is_neg_i" sov_sl23_w_is_neg_i_wrapper :: Ptr CUChar -> IO CInt

-- ── 覆盖映射 ──────────────────────────────────────────────────────────

sov_cover_perm_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt
sov_cover_perm_wrapper g4 out4
  | g4 == nullPtr || out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      mg <- sl23At g4
      case mg of
        Nothing -> return (fromIntegral sovErrTrit)
        Just g
          | not (sl23Member g) -> return (fromIntegral sovErrRange)
          | otherwise -> do
              pokeBytes out4 (map (fromIntegral :: Int -> Word8) (CV.permOfM g))
              return 0
foreign export ccall "sov_cover_perm" sov_cover_perm_wrapper :: Ptr CUChar -> Ptr CUChar -> IO CInt

sov_cover_kernel_size_wrapper :: Ptr CUChar -> IO CInt
sov_cover_kernel_size_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral (length CV.kernelOfCover) :: CUChar); return 0
foreign export ccall "sov_cover_kernel_size" sov_cover_kernel_size_wrapper :: Ptr CUChar -> IO CInt

sov_cover_ok_wrapper :: Ptr CUChar -> IO CInt
sov_cover_ok_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (if CV.coverOk then 1 else 0 :: CUChar); return 0
foreign export ccall "sov_cover_ok" sov_cover_ok_wrapper :: Ptr CUChar -> IO CInt

-- ── 表示论 ────────────────────────────────────────────────────────────

sov_a4_chi_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_a4_chi_wrapper irrep elem re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case (irrepAt irrep, a4At elem) of
      (Just r, Just g) -> do pokeEis re im (RA4.chi r g); return 0
      _ -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_chi" sov_a4_chi_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_a4_chi_orth_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_a4_chi_orth_wrapper rho sigma re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case (irrepAt rho, irrepAt sigma) of
      (Just a, Just b) -> do pokeEis re im (RA4.chiOrth a b); return 0
      _ -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_chi_orth" sov_a4_chi_orth_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_a4_regular_mult_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_a4_regular_mult_wrapper irrep re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case irrepAt irrep of
      Just r -> do pokeEis re im (RA4.regularMultiplicityNum r); return 0
      Nothing -> return (fromIntegral sovErrRange)
foreign export ccall "sov_a4_regular_mult" sov_a4_regular_mult_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_a4_col_orth_at_id_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
sov_a4_col_orth_at_id_wrapper re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do pokeEis re im RA4.colOrthAtId; return 0
foreign export ccall "sov_a4_col_orth_at_id" sov_a4_col_orth_at_id_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt

sov_a4_one_dim_nonfaithful_wrapper :: Ptr CUChar -> IO CInt
sov_a4_one_dim_nonfaithful_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if RA4.oneDimRepsNonFaithful then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_a4_one_dim_nonfaithful" sov_a4_one_dim_nonfaithful_wrapper :: Ptr CUChar -> IO CInt

sov_rep2_class_sizes_wrapper :: Ptr CUChar -> IO CInt
sov_rep2_class_sizes_wrapper out7
  | out7 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let vs = map (fromIntegral :: Integer -> CUChar) (take 7 R2.classSizes7)
      pokeArray out7 vs
      return 0
foreign export ccall "sov_rep2_class_sizes" sov_rep2_class_sizes_wrapper :: Ptr CUChar -> IO CInt

sov_rep2_class_info_wrapper :: Ptr CUChar -> IO CInt
sov_rep2_class_info_wrapper out14
  | out14 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let vs = concatMap (\(a, b) -> [fromIntegral a, fromIntegral b]) (take 7 R2.classInfoC)
      pokeArray out14 (map (fromIntegral :: Int -> CUChar) vs)
      return 0
foreign export ccall "sov_rep2_class_info" sov_rep2_class_info_wrapper :: Ptr CUChar -> IO CInt

sov_rep2_char_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_rep2_char_wrapper row cls re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise =
      let r = fromIntegral row :: Int
          c = fromIntegral cls :: Int
      in if r >= 7 || c >= 7
           then return (fromIntegral sovErrRange)
           else case (idxMay R2.allChars r, idxMay R2.allChars r >>= idxMay <*> pure c) of
                  _ -> case idxMay R2.allChars r of
                         Just line -> case idxMay line c of
                           Just e -> do pokeEis re im e; return 0
                           Nothing -> return (fromIntegral sovErrRange)
                         Nothing -> return (fromIntegral sovErrRange)
foreign export ccall "sov_rep2_char" sov_rep2_char_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_rep2_char_orth_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_rep2_char_orth_wrapper rowA rowB re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise =
      let a = fromIntegral rowA :: Int
          b = fromIntegral rowB :: Int
      in if a >= 7 || b >= 7
           then return (fromIntegral sovErrRange)
           else case (idxMay R2.allChars a, idxMay R2.allChars b) of
                  (Just la, Just lb) -> do pokeEis re im (R2.charPairingL la lb); return 0
                  _ -> return (fromIntegral sovErrRange)
foreign export ccall "sov_rep2_char_orth" sov_rep2_char_orth_wrapper :: CUChar -> CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_rep2_dims_wrapper :: Ptr CUChar -> IO CInt
sov_rep2_dims_wrapper out7
  | out7 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let vs = take 7 R2.predictedDims
      pokeArray out7 (map (fromIntegral :: Integer -> CUChar) vs)
      return 0
foreign export ccall "sov_rep2_dims" sov_rep2_dims_wrapper :: Ptr CUChar -> IO CInt

sov_rep2_spinor_dim_wrapper :: Ptr CUChar -> IO CInt
sov_rep2_spinor_dim_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (2 :: CUChar); return 0
foreign export ccall "sov_rep2_spinor_dim" sov_rep2_spinor_dim_wrapper :: Ptr CUChar -> IO CInt

sov_spin2_det_commutator_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
sov_spin2_det_commutator_wrapper re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do pokeEis re im S2.detCommutator2; return 0
foreign export ccall "sov_spin2_det_commutator" sov_spin2_det_commutator_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt

sov_spin2_no_common_eigenvector_wrapper :: Ptr CUChar -> IO CInt
sov_spin2_no_common_eigenvector_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if S2.noCommonEigenvector2 then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_spin2_no_common_eigenvector" sov_spin2_no_common_eigenvector_wrapper :: Ptr CUChar -> IO CInt

sov_spin3_det_rows_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
sov_spin3_det_rows_wrapper re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do pokeEis re im S3.detRows; return 0
foreign export ccall "sov_spin3_det_rows" sov_spin3_det_rows_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt

sov_spin3_no_common_eigenvector_wrapper :: Ptr CUChar -> IO CInt
sov_spin3_no_common_eigenvector_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if S3.noCommonEigenvector3 then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_spin3_no_common_eigenvector" sov_spin3_no_common_eigenvector_wrapper :: Ptr CUChar -> IO CInt

-- ── Hopf 商 / 旋量 / 截面 ─────────────────────────────────────────────

sov_hopf_base_card_wrapper :: Ptr CUShort -> IO CInt
sov_hopf_base_card_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral HOPF.baseCard :: CUShort); return 0
foreign export ccall "sov_hopf_base_card" sov_hopf_base_card_wrapper :: Ptr CUShort -> IO CInt

sov_hopf_proj_wrapper :: CUShort -> Ptr CUShort -> IO CInt
sov_hopf_proj_wrapper point out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case TRY.mkTryte (fromIntegral point) of
      Nothing -> return (fromIntegral sovErrRange)
      Just t -> do poke out (fromIntegral (HOPF.projClass t) :: CUShort); return 0
foreign export ccall "sov_hopf_proj" sov_hopf_proj_wrapper :: CUShort -> Ptr CUShort -> IO CInt

sov_hopf_fiber_size_wrapper :: CUShort -> Ptr CUChar -> IO CInt
sov_hopf_fiber_size_wrapper point out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case TRY.mkTryte (fromIntegral point) of
      Nothing -> return (fromIntegral sovErrRange)
      Just t -> do poke out (fromIntegral (HOPF.fiberSizeOf t) :: CUChar); return 0
foreign export ccall "sov_hopf_fiber_size" sov_hopf_fiber_size_wrapper :: CUShort -> Ptr CUChar -> IO CInt

sov_hopf_fiber_distribution_wrapper :: Ptr CUChar -> IO CInt
sov_hopf_fiber_distribution_wrapper out8
  | out8 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let vs = concatMap (\(a, b) -> [a, b]) (take 4 HOPF.fiberSizeDistribution)
      pokeArray out8 (map (fromIntegral :: Int -> CUChar) vs)
      return 0
foreign export ccall "sov_hopf_fiber_distribution" sov_hopf_fiber_distribution_wrapper :: Ptr CUChar -> IO CInt

sov_spin_fiber_card_wrapper :: Ptr CUChar -> IO CInt
sov_spin_fiber_card_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral SH.spinFiberCard :: CUChar); return 0
foreign export ccall "sov_spin_fiber_card" sov_spin_fiber_card_wrapper :: Ptr CUChar -> IO CInt

sov_spin_total_card_wrapper :: Ptr CUInt -> IO CInt
sov_spin_total_card_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral SH.spinTotalCard :: CUInt); return 0
foreign export ccall "sov_spin_total_card" sov_spin_total_card_wrapper :: Ptr CUInt -> IO CInt

sov_spin_phase_act_wrapper :: CUInt -> Ptr CUChar -> Ptr CUChar -> IO CInt
sov_spin_phase_act_wrapper k v2 out2
  | v2 == nullPtr || out2 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      bs <- peekArray 2 v2
      let vs = map (fromIntegral :: CUChar -> Int) bs
      case vs of
        [x, y]
          | x < 3 && y < 3 -> do
              let (r1, r2) = SH.phaseAct (fromIntegral k) (T.fromTritIndex (fromIntegral x), T.fromTritIndex (fromIntegral y))
              pokeArray out2 (map (fromIntegral :: Word8 -> CUChar) [T.toTritIndex r1, T.toTritIndex r2])
              return 0
        _ -> return (fromIntegral sovErrTrit)
foreign export ccall "sov_spin_phase_act" sov_spin_phase_act_wrapper :: CUInt -> Ptr CUChar -> Ptr CUChar -> IO CInt

sov_spin_alpha_pow_wrapper :: CUInt -> Ptr CUChar -> IO CInt
sov_spin_alpha_pow_wrapper k out4
  | out4 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      pokeBytes out4 (m2Bytes (SH.alphaPow (fromIntegral k)))
      return 0
foreign export ccall "sov_spin_alpha_pow" sov_spin_alpha_pow_wrapper :: CUInt -> Ptr CUChar -> IO CInt

sov_section_dims_wrapper :: Ptr CUInt -> IO CInt
sov_section_dims_wrapper out5
  | out5 == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      pokeArray out5 (map (fromIntegral :: Int -> CUInt)
        [FD.baseDim, FD.fiberDim, FD.flatDim, FD.effectiveDim, FD.overcount])
      return 0
foreign export ccall "sov_section_dims" sov_section_dims_wrapper :: Ptr CUInt -> IO CInt

-- ── 载体 ──────────────────────────────────────────────────────────────

sov_carrier_a4_dim_wrapper :: Ptr CUChar -> IO CInt
sov_carrier_a4_dim_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral CA4.dimCarrier :: CUChar); return 0
foreign export ccall "sov_carrier_a4_dim" sov_carrier_a4_dim_wrapper :: Ptr CUChar -> IO CInt

sov_carrier_a4_plancherel_wrapper :: Ptr CUShort -> IO CInt
sov_carrier_a4_plancherel_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral CA4.plancherelSum :: CUShort); return 0
foreign export ccall "sov_carrier_a4_plancherel" sov_carrier_a4_plancherel_wrapper :: Ptr CUShort -> IO CInt

sov_carrier_a4_basis_norm_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt
sov_carrier_a4_basis_norm_wrapper elem re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = case a4At elem of
      Just g -> do pokeEis re im (CA4.normC2 (CA4.basisVec g)); return 0
      Nothing -> return (fromIntegral sovErrRange)
foreign export ccall "sov_carrier_a4_basis_norm" sov_carrier_a4_basis_norm_wrapper :: CUChar -> Ptr CLLong -> Ptr CLLong -> IO CInt

sov_carrier_2a4_dim_wrapper :: Ptr CUChar -> IO CInt
sov_carrier_2a4_dim_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do poke out (fromIntegral K2.dimField :: CUChar); return 0
foreign export ccall "sov_carrier_2a4_dim" sov_carrier_2a4_dim_wrapper :: Ptr CUChar -> IO CInt

sov_carrier_2a4_bilinear_schur_wrapper :: Ptr CUChar -> IO CInt
sov_carrier_2a4_bilinear_schur_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      poke out (if K2.bilinearSchur then 1 else 0 :: CUChar)
      return 0
foreign export ccall "sov_carrier_2a4_bilinear_schur" sov_carrier_2a4_bilinear_schur_wrapper :: Ptr CUChar -> IO CInt

sov_carrier_2a4_fs_indicator_wrapper :: Ptr CLLong -> IO CInt
sov_carrier_2a4_fs_indicator_wrapper out
  | out == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do
      let e = K2.frobeniusSchurNum
      poke out (fromIntegral (EIS.eisA e `div` 24) :: CLLong)
      return 0
foreign export ccall "sov_carrier_2a4_fs_indicator" sov_carrier_2a4_fs_indicator_wrapper :: Ptr CLLong -> IO CInt

sov_carrier_2a4_coefficient_gram_det_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
sov_carrier_2a4_coefficient_gram_det_wrapper re im
  | re == nullPtr || im == nullPtr = return (fromIntegral sovErrNull)
  | otherwise = do pokeEis re im K2.gramDet; return 0
foreign export ccall "sov_carrier_2a4_coefficient_gram_det" sov_carrier_2a4_coefficient_gram_det_wrapper :: Ptr CLLong -> Ptr CLLong -> IO CInt
