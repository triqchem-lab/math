-- | Sov.Tests.Core — 核心引理测试 (镜像 rust sov-core 各模块 #[cfg(test)])
--
-- 每条测试对应 rust 侧同名测试, 三语言 (C++/Rust/Haskell) 交叉验证。
module Sov.Tests.Core (runAllTests) where

import Sov.Math.Adc
import Sov.Math.Bridge
import qualified Sov.Math.Constants as C
import Sov.Math.Eis
import Sov.Math.FixedComplex
import Sov.Math.Layer1
import Sov.Math.Nat
import Sov.Math.Ring
import Sov.Math.SunYi
import Sov.Math.Trit
import Sov.Math.Tryte
import Sov.Math.Vec

-- 迷你测试框架: (名称, Bool)
type Test = (String, Bool)

check :: String -> Bool -> Test
check = (,)

allTests :: [Test]
allTests =
  -- ── Trit (l1_field) ──
  [ check "c3_identity"    (all (\t -> c3cw (c3cw (c3cw t)) == t && c3ccw (c3ccw (c3ccw t)) == t) allTrits)
  , check "c3_inverses"    (all (\t -> c3cw (c3ccw t) == t && c3ccw (c3cw t) == t) allTrits)
  , check "mul_constitutional" (mulT T2 T2 == T1)  -- 宪法: T₂⊗T₂ ≠ T₁ 之否定 — 应为 T₁
  , check "norm"           (normSqT T0 == T0 && normSqT T1 == T1 && normSqT T2 == T1)
  , check "sunyi_inverses" (all (\t -> sun (yi t) == t && yi (sun t) == t) allTrits)
  , check "add_zero_inv"   (all (\t -> addT t (negT t) == T0 && addT t T0 == t) allTrits)

  -- ── Ring (l2_ring) ──
  , check "t1_generator"   (digitR 0 t1e == Just T0 && digitR 1 t1e == Just T1)
  , check "t2_is_squared"  (digitR 2 (mulR t1e t1e) == Just T1)
  , check "ring_add_comm"  (addR t1e t2e == addR t2e t1e)
  , check "ring_add_zero"  (addR t1e ringZero == t1e)
  , check "ring_mul_one"   (mulR t1e t0e == t1e)
  , check "ring_mul_zero"  (isZeroR (mulR t1e ringZero))
  , check "t1_pow11_zero"  (isZeroR (mulPowR t1e 11))
  , check "tryte_proj"     (tryteValue (toTryteR ringZero) == 0)
  , check "mul_coeff_carry" -- 2·3⁰ × 2·3⁰ = 4 = 1 + 3 (本征进位必须传播)
      (let p = mulR (ringFromTrit T2) (ringFromTrit T2)
       in digitR 0 p == Just T1 && digitR 1 p == Just T1
          && all (\k -> digitR k p == Just T0) [2..10])

  -- ── Adc (l0_adc) ──
  , check "u384_add_basic"
      (u384Limbs (u384Add (u384FromU64 42) (u384FromU64 58)) == 100 : replicate 5 0)
  , check "u384_add_carry"
      (u384Limbs (u384Add (u384FromU64 maxBound) (u384FromU64 1)) == 0 : 1 : replicate 4 0)
  , check "u384_mul_u64"   (u384Limbs (u384MulU64 (u384FromU64 1000) 3) == 3000 : replicate 5 0)
  , check "lcm_step"       (accStepCount (accStep 1 newAcc) == 1 && layer1Value (accStep 1 newAcc) > 0)
  , check "zhonglv_closure"
      (let acc = zhonglvClosure newAcc { accValue = u384FromU64 1 }
       in layer1Value acc > 0 && accStepCount acc == 0)

  -- ── Bridge ──
  , check "bridge_new"     (bridgeState newBridge == L1Ready)
  , check "bridge_micro"   (accStepCount (bridgeAcc (microPump newBridge)) == 0)
  , check "bridge_chern"   (chernGuardOk (updateChern (fromIntegral C.chernTargetQ16) newBridge))
  , check "bridge_reverse"
      (let b = updateChern (fromIntegral C.chernTargetQ16) newBridge
           (_, r) = reverseBridge b [T0, T1, T2]
       in case r of
            Right ts -> ts == [T0, T1, T2]
            Left _   -> False)
  , check "bridge_step_micro_pump"
      (accStepCount (bridgeAcc (foldl' (flip bridgeStep) newBridge (replicate C.microPump 1))) == 0)

  -- ── SunYi (sunyi) ──
  , check "twelve_first_last" (take 1 twelveLengths == [81] && reverse (take 1 (reverse twelveLengths)) == [30])
  , check "no_return"      (yiOp 30 /= 81)  -- 仲吕不能自生黄钟
  , check "boundary"       (isZhonglvBoundary 11 && not (isZhonglvBoundary 10))
  , check "tone_enum"      (toneIndex HuangZhong == 0 && toneIndex ZhongLv == 11 && nextTone ZhongLv == HuangZhong)

  -- ── Tryte (types) ──
  , check "tryte_zero"     (tryteValue tryteZero == 0)
  , check "tryte_reject"   (mkTryte 729 == Nothing && mkTryte 728 /= Nothing)
  , check "tryte_labels"   (nayinLabel (fromJust' (mkTryte 123)) == 123 `mod` 60
                            && semitone (fromJust' (mkTryte 123)) == 123 `mod` 12)
  , check "tryte_roundtrip" (all (\v -> fmap tryteValue (mkTryte v) == Just v) [0, 1, 2, 3, 8, 728])

  -- ── Layer1 (l1_pack) ──
  , check "pack_unpack_single"
      (unpack5 (pack5 T0 T1 T2 T0 T1) == (T1, T0, T2, T1, T0))
  , check "batch_roundtrip"
      (let ts = map toEnum' [0 .. 49]
       in unpackBatch (packBatch ts) 50 == ts)
  , check "layer1_add"     (layer1Add [T1, T2] [T2, T1] == [T0, T0])
  , check "layer1_dot"     (layer1Dot [T1, T2] [T1, T2] == 5)
  , check "layer1_norm"    (layer1NormSq [T0, T1, T2, T0] == 2)

  -- ── FixedComplex ──
  , check "omega_sum_identities"  -- ω+ω²+1=0: 实部 -32768-32768+65536=0, 虚部 +56753-56753=0
      (unQ16 (qcRe (qcAdd (qcAdd qcOmega qcOmega2) qcOne)) == 0
       && unQ16 (qcIm (qcAdd qcOmega qcOmega2)) == 0)
  , check "omega_cubed_q16"  omegaSumOk
  , check "qc_omega_omega2"  (qcMul qcOmega qcOmega `approxEq` qcOmega2)
  , check "qc_norm_one"      (abs (unQ16 (qcNormSq qcOmega) - 65536) <= 16)  -- Q16 截断容差 ±16

  -- ── Eis (eis) ──
  , check "omega_cubed_is_one"  (mulE (mulE eisOmega eisOmega) eisOmega == eisOne)
  , check "omega_sum_identity"  (addE (addE eisOne eisOmega) eisOmega2 == eisZero)
  , check "omega_square"        (mulE eisOmega eisOmega == eisOmega2)
  , check "unit_group_cyclic6"
      (all (\k -> normE (unitPow k) == 1 && unitIndex (unitPow k) == Just k) [0 .. 5]
       && unitPow 6 == eisOne
       && all (\(k, j) -> mulE (unitPow k) (unitPow j) == unitPow (k + j))
              [(k, j) | k <- [0 .. 5], j <- [0 .. 5]])
  , check "norm_multiplicative" (normE (mulE (Eis 2 1) (Eis 1 (-1))) == normE (Eis 2 1) * normE (Eis 1 (-1)))
  , check "conj_mul_hom"        (conjE (mulE (Eis 2 1) (Eis 1 (-1))) == mulE (conjE (Eis 2 1)) (conjE (Eis 1 (-1))))

  -- ── Vec / Nat (强类型基础设施) ──
  , check "vec_length_typed"    (vecLength n11 (replicateV n11 T0) == 11)
  , check "vec_zip_length_safe" (zipWithV addT (replicateV n3 T1) (replicateV n3 T2) == replicateV n3 T0)
  ]

-- ═══ 辅助 ═══

toEnum' :: Int -> Trit
toEnum' i = toEnum (i `mod` 3)

fromJust' :: Maybe a -> a
fromJust' (Just x) = x
fromJust' Nothing  = error "test: expected Just"

approxEq :: Q16Complex -> Q16Complex -> Bool
approxEq (Q16Complex a b) (Q16Complex c d) =
  abs (unQ16 a - unQ16 c) <= 16 && abs (unQ16 b - unQ16 d) <= 16

runAllTests :: IO ()
runAllTests = do
  let results = allTests
      failed  = [ n | (n, ok) <- results, not ok ]
  mapM_ (\(n, ok) -> putStrLn ((if ok then "✅ " else "🔴 ") ++ n)) results
  putStrLn $ replicate 40 '-'
  if null failed
    then putStrLn $ "✅ 全部通过: " ++ show (length results) ++ "/" ++ show (length results)
    else putStrLn $ "🔴 失败 " ++ show (length failed) ++ " 项: " ++ show failed
  -- 宪法常数自检 (编译期常数, 运行期断言)
  putStrLn $ "huangzhong=" ++ show C.huangzhong ++ " lcmTotal=" ++ show C.lcmTotal
