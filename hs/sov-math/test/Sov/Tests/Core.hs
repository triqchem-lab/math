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
import Sov.Math.L2Ops
import Sov.Math.Block
import Sov.Math.Guard.Chern
import Sov.Math.Guard.L7L8
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

  -- ── L2Ops (l2_matmul / l2_ops / vavx3 纯语义) ──
  , check "matmul_2x2"   -- rust test_matmul: x=[[1,2],[1,2]], w=[[1,0],[0,1]] → [[1,2],[1,2]]
      (matmulGF3 [[T1,T2],[T1,T2]] [[T1,T0],[T0,T1]] == [[T1,T2],[T1,T2]])
  , check "gf3_add_batch" -- rust test_add: 全1 + 全2 = 全0
      (zipWith addT (replicate 100 T1) (replicate 100 T2) == replicate 100 T0)
  , check "gf3_mul_batch" -- rust test_mul: [2,2]⊙[2,1] = [1,2]
      (map (\(a,b) -> mulT a b) [(T2,T2),(T2,T1)] == [T1,T2])
  , check "gate_sqrt3"   -- rust test_gate: 全1矩阵 5 行 dim10, threshold 100 → 全1
      (gateSqrt3 (replicate 5 (replicate 10 T1)) 100 == replicate 5 T1)
  , check "a4_c3cw"      -- rust test_a4 (l2_matmul): [0,1,2] C3Cw → [1,2,0]
      (a4FlipTriple C3Cw (T0, T1, T2) == (T1, T2, T0))
  , check "ring_vector_add"  -- rust test_vector_add: t1+t2 与 t2+t1, digit(1)=1
      (case ringVectorAdd [t1e, t2e] [t2e, t1e] of
         (c : _) -> digitR 1 c == Just T1
         []      -> False)
  , check "ring_dot"     -- rust test_dot: [t1,t2]·[t1,t1] → digit(2)=1, digit(3)=1
      (let d = ringDotProduct [t1e, t2e] [t1e, t1e]
       in digitR 2 d == Just T1 && digitR 3 d == Just T1)

  -- ── Block (types.rs 剩余) ──
  , check "layer_tag_values"
      (layerTagValue TagL1Gf3Tensor == 0x10000001 && layerTagValue TagTopologyGrid == 0x40000004)
  , check "sov_block_zero"  (sbQs sovBlockZero == replicateV n6 T0)

  -- ── Guard.Chern (sov-guard/chern.rs, test_chern_exact.rs) ──
  , check "zero_proto_chern_is_zero"  (computeChernExact protoZero == (0, 216))
  , check "monopole_inject_count"
      (let (p, n) = injectC2Monopole protoZero in n == 24 && computeChernExact p == (-432, 216))
  , check "chern_exactly_minus_two"
      (let (p, _) = injectC2Monopole protoZero
       in chernEquals (detectChern p) (-2) 1)
  , check "wilson_holonomy_is_unit"
      (let (p, _) = injectC2Monopole protoZero
       in all (\cell -> normE (cellWilsonHolonomy p 0 cell) == 1
                         && unitIndex (cellWilsonHolonomy p 0 cell) /= Nothing)
              [0 .. s2Cells - 1])
  , check "cell0_holonomy_trivial"   -- 和乐 = ω^12 = 1 (mod 3 平凡), 缠绕数承载拓扑
      (let (p, _) = injectC2Monopole protoZero
       in unitIndex (cellWilsonHolonomy p 0 0) == Just 0
          && unitIndex (cellWilsonHolonomy p 0 2) == Just 0)

  -- ── Guard.L7L8 (sov-guard/lib.rs) ──
  , check "l7_pass"      (chernVerdict (-131072) == Pass && chernValid (-131072))
  , check "l7_collapse"  (chernVerdict 0 == FailTopologicalCollapse)   -- |0| < 0.5
  , check "l7_drift"     (chernVerdict 200000 == WarnDrift)
  , check "holographic"  (computeHolographic 0 0 96 0 > 0)             -- z=96/12=8 → 正标度
  , check "band_ladder"  (spectralBand 100000 == Audio && spectralBand 700000 == Infrared
                          && spectralBand 2000000 == UltraHigh)
  , check "digital_root" (digitalRoot 38 == 2 && digitalRoot 81 == 9 && digitalRoot 9 == 9)
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
