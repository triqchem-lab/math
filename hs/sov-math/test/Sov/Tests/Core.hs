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
