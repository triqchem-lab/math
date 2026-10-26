-- | Sov.Tests.Core — 核心引理测试 (镜像 rust sov-core 各模块 #[cfg(test)])
--
-- 每条测试对应 rust 侧同名测试, 三语言 (C++/Rust/Haskell) 交叉验证。
module Sov.Tests.Core (runAllTests) where

import Data.List (nub, permutations, sort)

import Sov.Math.Adc
import Sov.Math.Bridge
import qualified Sov.Math.Constants as C
import Sov.Math.Eis
import Sov.Math.A4
import Sov.Math.CarrierA4
import qualified Sov.Math.RepA4 as RA4
import qualified Sov.Math.Rep2A4 as R2
import qualified Sov.Math.Cover2A4 as CV
import qualified Sov.Math.Complex2A4 as C2
import qualified Sov.Math.SL23 as SL
import qualified Sov.Math.SpinHopf as SH
import qualified Sov.Math.Field as FD
import Sov.Math.Hopf
import Sov.Math.FixedComplex
import Sov.Math.Layer1
import Sov.Math.L2Ops
import Sov.Math.Block
import Sov.Math.Guard.Chern
import Sov.Math.Guard.L7L8
import Sov.Math.Chiral
import qualified Sov.Math.Topology.T6 as T6
import qualified Sov.Math.Topology.TimeCrystal as TC
import qualified Sov.Math.Topology.Soliton as Sol
import qualified Sov.Math.Topology.Tower as Tw
import qualified Sov.Validation.Zomega as ZW
import qualified Sov.Validation.ChiInner as CI
import qualified Sov.Validation.GF9Diag as G9
import qualified Sov.Validation.GF4Magic as G4
import qualified Sov.Validation.Magic as MG
import qualified Sov.Validation.EM as EM
import qualified Sov.Math.Guard.NSE as NS
import qualified Sov.Math.DuodecClock as DC
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
  , check "holographic"  (computeHolographicApprox 0 0 96 0 > 0)             -- z=96/12=8 → 正标度
  , check "band_ladder"  (spectralBand 100000 == Audio && spectralBand 700000 == Infrared
                          && spectralBand 2000000 == UltraHigh)
  , check "digital_root" (digitalRoot 38 == 2 && digitalRoot 81 == 9 && digitalRoot 9 == 9)

  -- ── Chiral (sov-chiral 6 项 rust 测试) ──
  , check "chiral_conj"      (chiralConj T0 == T0 && chiralConj T1 == T2 && chiralConj T2 == T1)
  , check "chiral_involutive" (all (\t -> chiralConj (chiralConj t) == t) allTrits)
  , check "self_conj"        (isSelfConj T0 && not (isSelfConj T1) && not (isSelfConj T2))
  , check "wuxing_structure" -- 对角+相生环=1, 相克=ω, 其余=ω²; 且 1+ω+ω² 每行 5 格齐备
      (let t = wuxingTable
       in all (\(i, j) -> let v = cellOf t i j in
               if i == j || j == (i + 1) `mod` 5 then v == qcOne
               else if (i + 2) `mod` 5 == j then v == qcOmega
               else v == qcOmega2)
              [(i, j) | i <- [0 .. 4], j <- [0 .. 4]])
  , check "coupling"         (couplingFromPower 0 == Idle && couplingFromPower 1 == Meshed
                              && couplingFromPower 6 == Decoupled)
  , check "separation"       (isChiralSeparated 4 && not (isChiralSeparated 1))

  -- ── T6 / ZhongLv (sov-topology 纯整数) ──
  , check "t6_index"         (T6.t6Index 0 0 == 0 && T6.t6Index 0 1 == 1 && T6.t6Index 144 46 == 0)
  , check "t6_total"         (T6.t6Total == 6624)
  , check "zhonglv_ratio"    (T6.c3ZhonglvRatio == 125)
  , check "zhonglv_step" -- 0 恒 0; 8 步迭代 1→2→5→13→35→94→254→686→1854 (手算对照)
      (T6.zhonglvStep 0 11609505792 == 0 && T6.zhonglvStep 1 11609505792 == 1854)
  , check "harmonics_exact"  -- Integer 精确: base=1, maxA=2, maxB=1 → 6 项, max = 2²×3 = 12
      (T6.harmonicsAnalysis 1 2 1 == (6, 12) && T6.frequencyOctave 3 4 == 48)

  -- ── TimeCrystal / N14 ──
  , check "time_crystal"     (TC.timeCrystalPhaseStep 0 == 0 && TC.timeCrystalPhaseStep 500 == 500
                              && TC.timeCrystalPhaseStep 1000 == 1000)
  , check "phase_jump"       (TC.phaseJump 0 == 500 && TC.phaseJump 500 == 1000 && TC.phaseJump 1000 == 0)
  , check "dv_norm"          (TC.dvNormSq (TC.DynamicVectorQ16 2 3 6) == 49)
  , check "active_force"     -- base=81 (step 0): x=81, y=54
      (let f = TC.getActiveForce 0 in TC.dvX f == 81 && TC.dvY f == 54)
  , check "n14_closure"      (TC.n14IsClosureZero C.grandPump && not (TC.n14IsClosureZero 1)
                              && TC.n14VirtualPhase C.grandPump == 0)

  -- ── Soliton (零浮点改造, oracle 对账回执 ba93f0e3) ──
  , check "rho_endpoints"    (Sol.solitonRhoQ16Approx 0 == 0 && Sol.solitonRhoQ16Approx Sol.phaseAnchorSteps == 65536)
  , check "rho_monotone"     -- sin² 在 [0,π/2] 单调 → 定点表单调
      (and [ Sol.solitonRhoQ16Approx s <= Sol.solitonRhoQ16Approx (s + 1) | s <- [0, 100 .. 4399] ])
  , check "rho_anchor"       -- 锚点 s=2250 (π/4): sin²=1/2 → 32768 ± 1
      (abs (Sol.solitonRhoQ16Approx 2250 - 32768) <= 1)
  , check "phase_boundaries"
      (Sol.determinePhase 0 == Sol.SolidFrozen
       && Sol.determinePhase 24903 == Sol.LiquidForming   -- 0.38 边界
       && Sol.determinePhase 49152 == Sol.CurieTransition -- 0.75 边界
       && Sol.determinePhase 65536 == Sol.Superfluid)
  , check "standing_nodes"   (all Sol.isStandingNode [0, 3, 6, 9] && not (Sol.isStandingNode 1))
  , check "soliton_collapse" (Sol.solitonCollapse 113507 && Sol.solitonCollapse (-113507)
                              && not (Sol.solitonCollapse 113506))

  -- ── Tower (零浮点改造, oracle 对账回执 86c6aca2) ──
  , check "tower_edge_41"    -- h>2 的最小 zc = 41 (oracle 全域对账边界)
      (not (Tw.towerHeightExceeds 40 2 1) && Tw.towerHeightExceeds 41 2 1)
  , check "tower_2583"       -- rust test_tower: towerHeight(2583) > 2 (f64 版 3.6607)
      (Tw.towerHeightExceeds 2583 2 1 && Tw.towerHeightExceeds 2583 3 1
       && not (Tw.towerHeightExceeds 2583 4 1))  -- f64 精确值 3.66 → >3 且 <4
  , check "tower_floor"      -- 反演下界: h≤2 的最大 zc = 40 (与 edge 41 自洽)
      (Tw.zhonglvFromTowerFloor 2 1 == 40)
  , check "tower_height_q16" -- zc=0: fl=2.6355 < 12 → 高度 0
      (Tw.towerHeightQ16 0 == 0 && Tw.towerHeightQ16 2583 >= 3 * 65536)

  -- ── Validation: Z[ω,i] (zomega.rs) ──
  , check "zwi_identities"
      (ZW.zwiMul ZW.zwiOmega ZW.zwiOmega == ZW.zwiAdd (ZW.zwiNeg ZW.zwiOne) (ZW.zwiNeg ZW.zwiOmega)
       && ZW.zwiMul ZW.zwiI ZW.zwiI == ZW.zwiNeg ZW.zwiOne
       && ZW.zwiMul ZW.zwiOmega ZW.zwiI == ZW.zwiOmegaI
       && ZW.zwiMul ZW.zwiI ZW.zwiOmega == ZW.zwiOmegaI
       && ZW.zwiMul ZW.zwiOmegaI ZW.zwiOmegaI == ZW.zwiAdd ZW.zwiOne ZW.zwiOmega)
  , check "zwi_assoc_81_pow3"  -- 完整论域 81³ = 531441 组, 非抽样
      ZW.zwiAssocCheck81

  -- ── Validation: 杨辉三角 Lucas (yanghui.rs) ──
  , check "lucas_instances"
      (ZW.binomialModP 5 2 3 == 1 && ZW.binomialModP 4 2 3 == 0
       && ZW.binomialModP 6 3 3 == 2)
  , check "lucas_boundary"
      (ZW.binomialModP 10 0 3 == 1 && ZW.binomialModP 10 10 3 == 1)
  , check "yanghui_row7"     -- 第 7 行 mod 3: [1,0,0,1,0,0,1,1]... 以 Lucas 重算为准
      (ZW.yanghuiRow 7 3 == map (\k -> ZW.binomialModP 7 k 3) [0 .. 7]
       && ZW.yanghuiRow 0 3 == [1])

  -- ── Validation: 2·A₄ 特征标 (chi_inner.rs) ──
  , check "chi_irreducible"  CI.verifyChiIrreducible
  , check "chi_inner_exact"  -- ⟨χ₂',χ₂'⟩·|G| = (24,0) 精确
      (CI.innerNum CI.chi2' == Eis 24 0)

  -- ── Validation: GF(9) 幻方链 (gf9_diag.rs) ──
  , check "gf9_diag_all"     G9.verifyAll9
  , check "gf9_diag_split"   -- 五节独立可见: 判据分裂三条件
      (G9.verifyDiagonalClassification && G9.verifyAntiDiagonalEncoding
       && G9.verifyOrthogonalMagicSquare && G9.verifyDegenerateCases)

  -- ── Validation: GF(4) 幻方 (gf4_magic.rs 九节) ──
  , check "gf4_enum240"      G4.verifyEnum240
  , check "gf4_cellwise"     G4.verifyCellwiseOrthogonal
  , check "gf4_sigma_fail"   G4.verifySigmaVariantsFail
  , check "gf4_semi"         G4.verifySemiMagic
  , check "gf4_superpose"    G4.verifySuperposeM4
  , check "gf4_diag_lemma"   G4.verifyDiagonalLemma
  , check "gf4_frobenius"    G4.verifyFrobeniusConjugation
  , check "gf4_transpose"    G4.verifyTransposeSym
  , check "gf4_durer"        G4.verifyDurerNegative
  , check "gf4_all"          G4.verifyAll4

  -- ── Validation: 幻方 (magic_square + ternary_magic) ──
  , check "magic3_lo_shu_8"  (MG.magic3Count == 8)  -- 9! 全枚举
  , check "latin3_orthogonal"
      (MG.isLatinN 3 MG.latin1_3 && MG.isLatinN 3 MG.latin2_3
       && MG.areOrthogonalN 3 MG.latin1_3 MG.latin2_3)
  , check "gf9_mols_28"      MG.verifyMolsPairwiseOrthogonal
  , check "euler_9_magic"    -- λ₁=α, λ₂=2α 叠加 → 369
      (MG.isMagicSquare (MG.eulerSuperpose9 (MG.gf9Mols !! 0) (MG.gf9Mols !! 1)) == (True, 369))
  , check "doubly_even_12"   MG.verifyDoublyEven12
  , check "doubly_even_144"  MG.verifyDoublyEven144
  , check "m4_euler_34"      MG.verifyM4Euler
  , check "magic_all"        MG.verifyAllMagic

  -- ── Validation: 电磁学 (em_* 15+9 节) ──
  , check "em_alpha_powers"    EM.verifyAlphaPowers
  , check "em_norm_collapse"   EM.verifyNormCollapseTable
  , check "em_norm_conj"       EM.verifyNormViaConjugation
  , check "em_sigma_inv"       EM.verifySigmaInvolutive
  , check "em_norm_mult"       EM.verifyNormMultiplicative
  , check "em_freq_fractions"  (EM.inverseInverseFreqs == [(1,1,1,1),(1,4,1,2),(1,9,1,3),(1,16,1,4)])
  , check "em_gauge_assoc"     EM.verifyGaugeAssoc64
  , check "em_gauge_id_inv"    EM.verifyGaugeIdentityInverse
  , check "em_gauge_norm"      EM.verifyGaugeNormOne
  , check "em_gf3_lemmas"      EM.verifyGF3Lemmas
  , check "em_delta_laplace"   EM.verifyDeltaSEqualsLaplacian
  , check "em_noether"         EM.verifyNoetherIdentity
  , check "em_diff_comm"       EM.verifyDiffComm
  , check "em_all_freq"        EM.verifyAllEMFreq
  , check "em_all_action"      EM.verifyAllEMAction

  -- ── Guard.NSE (sov-guard 运行时层) ──
  , check "grid_zero"        (NS.gridCount NS.gridZero == 0 && NS.gridDensityQ16 NS.gridZero == 0)
  , check "grid_couette"     (NS.gridCount (NS.gridInitCouette NS.gridZero) > 0)
  , check "nse_step"         (NS.nseStep (NS.nseStepOnce NS.nseNew) == 1)  -- rust test_nse
  , check "nse_zhonglv"      -- 12 步后仲吕闭合计数 +1
      (NS.nseZhonglv (foldl (\s (_ :: Int) -> NS.nseStepOnce s) NS.nseNew [1 .. 13]) == 1)
  , check "sov_serial"       -- 序列化 + 头校验 (头 4-8 字节须为 magic/version, 语义同 rust load_sov)
      (let hdr = NS.le32Pub NS.sovMagic ++ NS.le32Pub NS.sovVersion ++ replicate 64 0
           bytes = NS.sovSerialize hdr [1, 2, 3]
       in NS.sovCheckHeader bytes && length bytes == 72 + 3 + 16)
  , check "vacuum_q16"       -- 尾数 Q16 对账: 0.74×65536=48497, 3.55×65536=58353, 5.16×65536=67291
      (NS.vacuumLeonovLQ0 == (48497, -25) && NS.vacuumLeonovRho0 == (232653, 75)
       && NS.vacuumHarameinRhoV == (338166, 96))
  , check "holo_progress"    (NS.lcmProgressQ16 NS.holoNew == 0)
  , check "constitution_audit" NS.fullConstitutionalAudit
  , check "counter_examples" (length (NS.counterExamples) == 5)

  -- ── DC 联合时钟定义层 (DuodecClock/DCGroup 镜像, 零冥族) ──
  , check "dc_zero_oblivion"  -- 五字段 + σ 对合 + 生成元周期, 全枚举
      (all snd DC.zeroOblivion)
  , check "dc_char3"          DC.dcChar3
  , check "dc_joint_period"   DC.jointPeriod12
  , check "dc_gen_order_12"   DC.jointGeneratorPeriod
  , check "dc_sigma_inv"      (all (\p -> DC.sigmaDC (DC.sigmaDC p) == p) DC.allPoints)
  , check "dc_sigma_on_alpha" -- σ_DC(T,αᵏ) = (T,α⁻ᵏ): σ(α)=α³, σ(α²)=α²
      (DC.sigmaDC (DC.DP T0 DC.A1) == DC.DP T0 DC.A3
       && DC.sigmaDC (DC.DP T0 DC.A2) == DC.DP T0 DC.A2)
  , check "dc_mixedop_assoc"  -- 结合律 12³ = 1728 case 全枚举 (对齐 mixedOp-assoc)
      (and [ DC.mixedOp (DC.mixedOp p q) r == DC.mixedOp p (DC.mixedOp q r)
           | p <- DC.allPoints, q <- DC.allPoints, r <- DC.allPoints ])

  -- ── DC CRT 投影 (Duodecimal.agda crt12) ──
  , check "dc_crt_roundtrip"  DC.crt12Roundtrip    -- 12 + 12 case
  , check "dc_crt_hom_144"    DC.crt12Hom144       -- 144 case 全枚举
  , check "dc_crt_anchors"    -- CRT 锚点: e→0, g=(T₁,a₁)→1 (=+1), (T₀,α)→9 (≡0m3,≡1m4), (T₁,a₀)→4
      (DC.toDuodec DC.duodecE == 0 && DC.toDuodec DC.jointGenerator == 1
       && DC.toDuodec (DC.DP T0 DC.A1) == 9
       && DC.toDuodec (DC.DP T1 DC.A0) == 4)
  , check "dc_successor_12"   -- +1¹² = id (记数法层后继归零)
      (all (\x -> iterate DC.successor12 x !! 12 == x) [0 .. 11])
  , check "dc_doz_digits"     -- Doz 单位数字: A=10, B=11
      (DC.dozDigit 10 == "A" && DC.dozDigit 11 == "B" && DC.dozDigit 3 == "3")
  ]

-- ═══ 辅助 ═══

toEnum' :: Int -> Trit
toEnum' i = toEnum (i `mod` 3)

fromJust' :: Maybe a -> a
fromJust' (Just x) = x
fromJust' Nothing  = error "test: expected Just"

cellOf :: Vec N5 (Vec N5 Q16Complex) -> Int -> Int -> Q16Complex
cellOf tbl i j =
  let row = vecAt i tbl
  in vecAt j row
  where
    vecAt :: Int -> Vec n a -> a
    vecAt k v = maybe (error "test: vec index") id (indexV k v)

approxEq :: Q16Complex -> Q16Complex -> Bool
approxEq (Q16Complex a b) (Q16Complex c d) =
  abs (unQ16 a - unQ16 c) <= 16 && abs (unQ16 b - unQ16 d) <= 16

--------------------------------------------------------------------------------
-- C[A₄] 载体 + Plancherel 内积（2026-10-26 新增）
--
-- 判据：载体是**函数空间**（12 维），不是点集；内积是 Plancherel 配对，
-- 不是点积。基 δ_g 在该配对下正交归一（144 对全检）。
--------------------------------------------------------------------------------

carrierTests :: [Test]
carrierTests =
  [ check "a4_card"              (length allA4 == a4Card && a4Card == 12)
  , check "a4_perm_distinct"     (length (nub (map a4Perm allA4)) == 12)
  , check "a4_perm_even"         (all (evenPerm4 . a4Perm) allA4)
  , check "a4_identity_l"        (all (\g -> mulA4 idA4 g == g) allA4)
  , check "a4_identity_r"        (all (\g -> mulA4 g idA4 == g) allA4)
  , check "a4_inverse"           (all (\g -> mulA4 g (invA4 g) == idA4
                                       && mulA4 (invA4 g) g == idA4) allA4)
  , check "a4_assoc"             (all (\(g, h, k) -> mulA4 (mulA4 g h) k
                                                   == mulA4 g (mulA4 h k)) triples)
  , check "carrier_dim"          (dimCarrier == 12)
  , check "carrier_basis_orthonormal"
      (all (\(g, h) -> dotC (basisVec g) (basisVec h)
                        == (if g == h then eisOne else eisZero)) pairs)
  , check "carrier_parseval_delta" (all (\g -> normC2 (basisVec g) == eisOne) allA4)
  , check "plancherel_identity"  (plancherelSum == 12 && plancherelSum == dimCarrier)
  , check "a4_rot_fixes_its_vertex"
      (all (\i -> all (\j -> pAt (Rot i j) (f4i i) == f4i i) [G0, G1]) [F0, F1, F2, F3])
  , check "carrier_regular_action_hom"
      (all (\(g, h) -> all (\i -> regAct g (regAct h (basisVec (elemAt i)))
                              == regAct (mulA4 g h) (basisVec (elemAt i))) [0 .. 11]) pairs)
  , check "carrier_unitary"
      (all (\g -> all (\(i, j) -> dotC (regAct g (basisVec (elemAt i))) (regAct g (basisVec (elemAt j)))
                                == dotC (basisVec (elemAt i)) (basisVec (elemAt j))) idxPairs) allA4)
  , check "a4_generated_by_two_elements" a4Generated
  , check "a4_perm_set_equals_alt4"
      (sort (map a4Perm allA4) == sort [ q | q <- permutations [0 .. 3], evenPerm4 q ])
  , check "a4_flip_two_transpositions"
      (all (\k -> all (\i -> pAt (Flip k) i /= i
                            && pAt (Flip k) (pAt (Flip k) i) == i) [0 .. 3])
           [H0, H1, H2])
  ]
  where
    pairs   = [ (g, h) | g <- allA4, h <- allA4 ]
    triples = [ (g, h, k) | g <- allA4, h <- allA4, k <- allA4 ]
    f4i F0 = 0; f4i F1 = 1; f4i F2 = 2; f4i F3 = 3
    elemAt k = case drop k allA4 of
                 (x : _) -> x
                 []      -> Id
    idxPairs = [ (i, j) | i <- [0 .. 11], j <- [0 .. 11] ]
    pAt g k = case drop k (a4Perm g) of
                (v : _) -> v
                []      -> -1
    -- 偶置换判定：逆序数奇偶
    evenPerm4 p = even (inversions p)
    inversions p = length [ () | i <- [0 .. 3], j <- [i + 1 .. 3]
                               , at p i > at p j ]
    at xs k = maybe 0 id (safeIdx k xs)
    safeIdx :: Int -> [a] -> Maybe a
    safeIdx _ []       = Nothing
    safeIdx 0 (x : _)  = Just x
    safeIdx k (_ : xs) = safeIdx (k - 1) xs

--------------------------------------------------------------------------------
-- 离散 Hopf 纤维化 T⁶ → T⁶/A₄（2026-10-26 新增）
--
-- 判据：底空间 135 类；投影在 A₄ 轨道上不变；纤维大小分布
-- {1:27, 4:54, 6:27, 12:27}（全枚举，非估计）。
--------------------------------------------------------------------------------

hopfTests :: [Test]
hopfTests =
  [ check "hopf_points_729"        (length allT6Points == 729)
  , check "hopf_orbit_invariant"   (all (\(g, x) -> projClass (actOn g x) == projClass x) gxPairs)
  , check "hopf_base_card_135"     (length (nub (map projClass allT6Points)) == baseCard
                                    && baseCard == 135)
  , check "hopf_fiber_distribution"
      (fiberSizeDistribution == [(1, 27), (4, 54), (6, 27), (12, 27)])
  , check "hopf_fiber_sum_729"
      (sum [ s * n | (s, n) <- fiberSizeDistribution ] == 729)
  , check "hopf_no_54_orbit"       (all (\(s, _) -> s <= 12) fiberSizeDistribution)
  , check "hopf_key_complete"
      (all (\x -> let same = [ y | y <- allT6Points, projClass y == projClass x ]
                      orb  = orbitOf x
                  in length same == length orb && all (`elem` orb) same) allT6Points)
  ]
  where
    gxPairs = [ (g, x) | g <- allA4, x <- allT6Points ]

--------------------------------------------------------------------------------
-- 口径声明（T6）：以下所有 "全枚举" 验证（49 对正交、729 点 key 完备性、
-- 1728 三元组结合律、24 个偶置换比对、576 对覆盖同态）都是**有限对象的全枚举**，
-- 与归纳证明等价但不等于归纳证明；引用时必须写"全枚举"。
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- A₄ 表示论：特征标表 + 正交性（2026-10-26 新增）
--
-- 判据：行正交 Σ_g χ_ρ conj χ_σ = 12·δ；正则多重度 = 12·d_ρ；
-- 列正交在单位元处 ⇒ Σ d_ρ² = 12（Plancherel 的结构证明）；三个 1 维表示非忠实。
--------------------------------------------------------------------------------

repA4Tests :: [Test]
repA4Tests =
  [ check "rep_chi_orthonormal"
      (all (\(a, b) -> RA4.chiOrth a b == (if a == b then Eis 12 0 else eisZero))
           irrPairs)
  , check "rep_regular_multiplicity"
      (all (\r -> RA4.regularMultiplicityNum r == Eis (fromIntegral (12 * RA4.dimI r)) 0)
           RA4.allIrreps)
  , check "rep_col_orth_at_id_12"   (RA4.colOrthAtId == Eis 12 0)
  , check "rep_one_dim_non_faithful" RA4.oneDimRepsNonFaithful
  , check "rep_one_dim_hom"
      (all (\r -> all (\(g, h) -> RA4.chi r (mulA4 g h)
                                  == mulE (RA4.chi r g) (RA4.chi r h)) ghPairs)
           [RA4.R1, RA4.R1', RA4.R1''])
  , check "rep_three_dim_values"
      (RA4.chi RA4.R3 Id == Eis 3 0
        && RA4.chi RA4.R3 (Flip H0) == Eis (-1) 0
        && RA4.chi RA4.R3 (Rot F0 G0) == eisZero)
  , check "rep_chi_at_identity_is_dim"
      (all (\r -> RA4.chi r Id == Eis (fromIntegral (RA4.dimI r)) 0) RA4.allIrreps)
  , check "rep_class_partition_4_3_4_4"
      (length [ () | g <- allA4, RA4.classOf g == RA4.CE  ] == 1
    && length [ () | g <- allA4, RA4.classOf g == RA4.CV4 ] == 3
    && length [ () | g <- allA4, RA4.classOf g == RA4.CA  ] == 4
    && length [ () | g <- allA4, RA4.classOf g == RA4.CB  ] == 4)
  , check "rep_class_conj_invariant"
      (all (\(g, h) -> RA4.classOf (mulA4 (mulA4 h g) (invA4 h)) == RA4.classOf g) ghPairs)
  ]
  where
    irrPairs = [ (a, b) | a <- RA4.allIrreps, b <- RA4.allIrreps ]
    ghPairs  = [ (g, h) | g <- allA4, h <- allA4 ]

--------------------------------------------------------------------------------
-- 2A₄ = SL(2,F₃)：旋量/相位那条腿（2026-10-26 新增）
--
-- 判据：24 元素、忠实（24 个互不相同）、W=(σ₁σ₂)³ = −I、−I 中心且阶 2、
-- 存在 4 阶元（A₄ 没有 ⇒ 相位必须上双层覆盖）、在 8 个非零向量上传递 ⇒ 9 = 1+8。
--------------------------------------------------------------------------------

sl23Tests :: [Test]
sl23Tests =
  [ check "sl23_card_24"          (length SL.allSL23 == SL.sl23Card
                                    && length (nub SL.allSL23) == 24)
  , check "sl23_det_one"          (all (\m -> SL.matDet m == T1) SL.allSL23)
  , check "sl23_closed"           (all (\(a, b) -> SL.matMul a b `elem` SL.allSL23) pairs)
  , check "sl23_identity"         (all (\m -> SL.matMul m SL.matId == m
                                          && SL.matMul SL.matId m == m) SL.allSL23)
  , check "sl23_inverse"          (all (\m -> SL.matMul m (SL.matInv m) == SL.matId
                                          && SL.matMul (SL.matInv m) m == SL.matId) SL.allSL23)
  , check "sl23_sigma_orders"     (SL.orderOf SL.sigma1 == 3 && SL.orderOf SL.sigma2 == 3)
  , check "sl23_W_is_negI"        (SL.wElement == SL.negI)
  , check "sl23_negI_order2"      (SL.orderOf SL.negI == 2)
  , check "sl23_negI_central"     (all (\m -> SL.matMul m SL.negI == SL.matMul SL.negI m) SL.allSL23)
  , check "sl23_has_order4"       (not (null SL.elemsOfOrder4) && length SL.elemsOfOrder4 == 6)
  , check "sl23_a4_has_no_order4" (all (\g -> orderOfA4 g /= 4) allA4)
  , check "sl23_vec_transitive_8" (length (SL.orbitOfVec (T1, T0)) == 8
                                    && length (nub (concatMap SL.orbitOfVec SL.nonzeroVectors)) == 8)
  , check "sl23_zero_fixed"       (all (\m -> SL.actVec m (T0, T0) == (T0, T0)) SL.allSL23)
  , check "sl23_nine_is_1_plus_8" (1 + length (SL.orbitOfVec (T1, T0)) == 9)
  , check "sl23_all_order4_square_negI"
      (all (\m -> SL.matMul m m == SL.negI) SL.elemsOfOrder4)
  ]
  where
    pairs = [ (a, b) | a <- SL.allSL23, b <- SL.allSL23 ]
    -- A₄ 元素阶：用群乘法反复左乘，直到回到单位元（总函数，12 步内必停）
    orderOfA4 :: A4 -> Int
    orderOfA4 g = go 1 g
      where
        go k x
          | x == idA4 = k
          | k > 12    = 0
          | otherwise = go (k + 1) (mulA4 x g)

--------------------------------------------------------------------------------
-- 旋量 Hopf 丛 T⁶ × GF(3)²（2026-10-26 新增）
--
-- 判据：α 阶 4；α² = −I；−I 在旋量纤维上取负；W=(σ₁σ₂)³ 在同一纤维上**同样**取负；
-- 投影在全自旋作用下不变；8 个非零向量在 ⟨α⟩ 下分成 2 条 4 元轨道。
--------------------------------------------------------------------------------

spinHopfTests :: [Test]
spinHopfTests =
  [ check "spin_fiber_card_9"      (SH.spinFiberCard == 9)
  , check "spin_total_card_6561"   (SH.spinTotalCard == 729 * 9 && SH.spinTotalCard == 6561)
  , check "spin_alpha_order4"      (SL.orderOf SH.alphaMat == 4)
  , check "spin_alpha_sq_is_negI"  (SH.alphaPow 2 == SL.negI)
  , check "spin_alpha_fourth_id"   (SH.alphaPow 4 == SL.matId)
  , check "spin_negI_negates"      (all (\v -> SH.phaseAct 2 v == negPair v) allVecs)
  , check "spin_W_negates"         (all (\v -> SL.actVec SL.wElement v == negPair v) allVecs)
  , check "spin_alpha_fixes_zero"  (SH.phaseAct 1 (T0, T0) == (T0, T0))
  , check "spin_proj_invariant"
      (all (\(g, k, x) -> SH.spinProj (SH.spinAct (g, k) (x, (T1, T0))) == projClass x)
           gkx)
  , check "spin_phase_orbits_2x4"  (map length SH.phaseOrbits == [4, 4])
  , check "spin_orbit_reps_135"    (length SH.spinOrbitReps == 135)
  ]
  where
    allVecs = [ (x, y) | x <- allTrits, y <- allTrits ]
    gkx     = [ (g, k, x) | g <- allA4, k <- [0 .. 3], x <- allT6Points ]
    negPair (x, y) = (negT x, negT y)

--------------------------------------------------------------------------------
-- 截面空间 + 扭转缺陷（2026-10-26 新增）
--
-- 判据：平纤维维数 135×12=1620；有效维数 Σ|fiber|=729；缺陷 891。
-- 「891 = 扭转类大小」是**解释**，不是定理；这里钉住的是等式。
--------------------------------------------------------------------------------

fieldTests :: [Test]
fieldTests =
  [ check "field_base_135"        (FD.baseDim == 135)
  , check "field_fiber_12"        (FD.fiberDim == 12)
  , check "field_flat_1620"       (FD.flatDim == 1620 && FD.flatDim == 135 * 12)
  , check "field_effective_729"   (FD.effectiveDim == 729)
  , check "field_overcount_891"       (FD.overcount == 891
                                    && FD.flatDim - FD.effectiveDim == FD.overcount)
  , check "field_free_orbits_27"  (FD.freeOrbitCount == 27)
  , check "field_nonfree_108"     (FD.nonFreeOrbitCount == 108
                                    && FD.freeOrbitCount + FD.nonFreeOrbitCount == 135)
  , check "field_section_length"  (case FD.mkSection (replicate 135 zeroC) of
                                     Just _  -> True
                                     Nothing -> False)
  , check "field_section_reject"  (case FD.mkSection (replicate 134 zeroC) of
                                     Just _  -> False
                                     Nothing -> True)
  , check "field_zero_norm"       (case FD.mkBaseKey 0 of
                                     Just k  -> FD.sectionNorm FD.zeroSection k == Just eisZero
                                     Nothing -> False)
  , check "field_basekey_rejects_out_of_range"
      (FD.mkBaseKey 135 == Nothing && FD.mkBaseKey (-1) == Nothing
        && FD.mkBaseKey 0 /= Nothing && length FD.allBaseKeys == 135)
  , check "field_min_fiber_1"     (FD.minFiberSize == 1 && FD.maxFiberSize == 12)
  , check "field_uniform_coords_not_faithful"
      (FD.uniformCoordsFaithful == False)
  , check "field_overcount_by_orbit"
      (FD.overcountByOrbit == [(1, 27, 297), (4, 54, 432), (6, 27, 162), (12, 27, 0)])
  , check "field_overcount_decomposition"  FD.overcountDecomposition
  , check "field_no_equivariant_section"
      (FD.equivariantSectionPossible == False)
  ]

--------------------------------------------------------------------------------
-- 2A₄ 旋量特征标 + 正交性（2026-10-26 新增）
--
-- 判据：7 类大小 Σ=24；三个 2 维旋量两两正交且各自范数 24；
-- 维数恒等式 Σd²=24，由此剩余四个 irrep 的维数被唯一确定为 {1,1,1,3}。
--------------------------------------------------------------------------------

rep2A4Tests :: [Test]
rep2A4Tests =
  [ check "rep2_class_sizes_sum_24" (R2.classSizesSum == 24
                                      && R2.classSizesSum == R2.groupOrder)
  , check "rep2_spinor_orthogonality" R2.spinorOrthogonality
  , check "rep2_spinor_distinct"
      (length (nub [ R2.spinorChar s | s <- R2.allSpinors ]) == 3)
  , check "rep2_spinor_dim_2"
      (all (\s -> nthOf (R2.identityClassIndex) (R2.spinorChar s) == Eis 2 0) R2.allSpinors)
  , check "rep2_dims_identity_24"    R2.dimsIdentity
  , check "rep2_predicted_dims"
      (R2.predictedDims == [1, 1, 1, 2, 2, 2, 3])
  , check "rep2_class_structure_computed"  R2.classStructureOk
  , check "rep2_class_count_7"             (length R2.sl23Classes == 7)
  , check "rep2_all_chars_orthogonal"      R2.allCharsOrthogonal
  , check "rep2_all_chars_distinct"        R2.allCharsDistinct
  , check "rep2_full_table_dims"
      (map (nthOf R2.identityClassIndex) R2.allChars == replicate 3 (Eis 1 0)
        ++ replicate 3 (Eis 2 0) ++ [Eis 3 0])
  , check "rep2_identity_class_size_order"
      (nthPair R2.identityClassIndex R2.classInfoC == (1, 1))
  , check "rep2_class_order_signature"
      (R2.classSignature == R2.expectedSignature)
  , check "rep2_neg_class_involutive"
      (all (\i -> R2.negClassOf (R2.negClassOf i) == i) [0 .. 6])
  , check "rep2_one_dim_homs_3"      (length R2.oneDimHoms == 3)
  , check "rep2_chars_orthogonal_computed" R2.allCharsOrthogonal
  , check "rep2_regular_multiplicity" R2.regularMultiplicityOk
    -- ── T1（P0）：SL(2,F₃) → A₄ 覆盖映射 ──
  , check "cover_homomorphism"
      (all (\(g, h) -> CV.permOfM (SL.matMul g h) == CV.composeP (CV.permOfM g) (CV.permOfM h))
           [ (g, h) | g <- SL.allSL23, h <- SL.allSL23 ])
  , check "cover_image_even"        (all CV.isEvenP CV.imagePerms)
  , check "cover_image_is_a4"
      (sort CV.imagePerms == sort (map a4Perm allA4))
  , check "cover_kernel_is_pm_I"    (length CV.kernelOfCover == 2
                                      && SL.matId `elem` CV.kernelOfCover
                                      && SL.negI `elem` CV.kernelOfCover)
  , check "cover_quotient_12"       (length CV.imagePerms == 12)
  , check "cover_ok"                CV.coverOk
    -- ── T2：2 维复表示（Z[ω] 显式矩阵），χ = tr ──
  , check "c2a4_model_ok"           C2.modelOk
  , check "c2a4_class_signature_matches"
      (C2.classInfoM == sort R2.classInfoC)   -- classInfoM 已排序；classInfoC 是计算序
  , check "c2a4_24_elements_distinct"
      (length C2.group24 == 24 && length (nub C2.group24) == 24)
  , check "c2a4_one_dim_index_24"    (length C2.mIndex == 24)
  , check "c2a4_tensor_reps_hom"     C2.tensorRepsHom
  , check "c2a4_tensor_reps_faithful" C2.tensorRepsFaithful
  , check "rep2_spinor_matches_model"
      (all (\(sz, od) -> R2.spinorValue od == C2.spinorCharFromModel sz od
                          && R2.spinorValue od /= Eis 99 0) R2.classInfoC)
  , check "rep2_neg_pairs_3_6"
      (all (\i -> case snd (nthPair i R2.classInfoC) of
                     3 -> snd (nthPair (R2.negClassOf i) R2.classInfoC) == 6
                     6 -> snd (nthPair (R2.negClassOf i) R2.classInfoC) == 3
                     4 -> R2.negClassOf i == i
                     _ -> True) [0 .. 6])
  ]
  where
    -- 取第 k 类上的值（计算序下的类索引）
    nthOf :: Int -> [Eis] -> Eis
    nthOf k xs = case drop k xs of
                   (x : _) -> x
                   []      -> Eis 0 0
    nthPair :: Int -> [(Int, Int)] -> (Int, Int)
    nthPair k xs = case drop k xs of
                     (x : _) -> x
                     []      -> (0, 0)

runAllTests :: IO ()
runAllTests = do
  let results = allTests ++ carrierTests ++ hopfTests ++ repA4Tests ++ rep2A4Tests
                  ++ sl23Tests ++ spinHopfTests ++ fieldTests
      failed  = [ n | (n, ok) <- results, not ok ]
  mapM_ (\(n, ok) -> putStrLn ((if ok then "✅ " else "🔴 ") ++ n)) results
  putStrLn $ replicate 40 '-'
  if null failed
    then putStrLn $ "✅ 全部通过: " ++ show (length results) ++ "/" ++ show (length results)
    else putStrLn $ "🔴 失败 " ++ show (length failed) ++ " 项: " ++ show failed
  -- 宪法常数自检 (编译期常数, 运行期断言)
  putStrLn $ "huangzhong=" ++ show C.huangzhong ++ " lcmTotal=" ++ show C.lcmTotal
