//! A₄/2A₄ 族三语言镜像测试（rust 侧）
//!
//! 与 `cpp/tests/test_a4_family.cpp`（122 条）和 `hs/sov-math/test/Sov/Tests/Core.hs`
//! 逐条对应；同一断言集在三语言上必须同为真。

use sov_a4::{a4, carrier_2a4, carrier_a4, complex2a4 as c2a4, cover, hopf, rep_2a4, rep_a4, section, sl23, spin, spin_hopf};
use sov_core::eis::Eis;

fn eis(a: i64, b: i64) -> Eis {
    Eis(a, b)
}

// ── A₄ 群 ────────────────────────────────────────────────────────────────

#[test]
fn a4_group() {
    assert_eq!(a4::ALL_A4.len(), a4::A4_CARD);
    assert_eq!(a4::A4_CARD, 12);

    let mut ps: Vec<[u8; 4]> = a4::ALL_A4.iter().map(|g| g.perm()).collect();
    ps.sort();
    ps.dedup();
    assert_eq!(ps.len(), 12, "a4_perm_distinct");
    assert!(ps.iter().all(a4::is_even_perm), "a4_perm_even");

    // 与全部 4 点偶置换逐项相等
    let mut evens: Vec<[u8; 4]> = Vec::new();
    for a in 0u8..4 {
        for b in 0u8..4 {
            if b == a {
                continue;
            }
            for c in 0u8..4 {
                if c == a || c == b {
                    continue;
                }
                let d = 6 - a - b - c;
                let q = [a, b, c, d];
                if a4::is_even_perm(&q) {
                    evens.push(q);
                }
            }
        }
    }
    evens.sort();
    assert_eq!(ps, evens, "a4_perm_set_equals_alt4");

    for g in a4::ALL_A4 {
        assert_eq!(a4::mul(a4::A4::Id, g), g, "a4_identity_l");
        assert_eq!(a4::mul(g, a4::A4::Id), g, "a4_identity_r");
        assert_eq!(a4::mul(g, a4::inv(g)), a4::A4::Id, "a4_inverse_l");
        assert_eq!(a4::mul(a4::inv(g), g), a4::A4::Id, "a4_inverse_r");
        assert_eq!(a4::from_index(g.index()), g, "a4_index_roundtrip");
    }
    for g in a4::ALL_A4 {
        for h in a4::ALL_A4 {
            for k in a4::ALL_A4 {
                assert_eq!(a4::mul(a4::mul(g, h), k), a4::mul(g, a4::mul(h, k)), "a4_assoc");
            }
        }
    }
    assert!(a4::generated_by_two_elements(), "a4_generated_by_two_elements");

    for i in a4::ALL_F4 {
        for j in a4::ALL_F2 {
            let p = a4::A4::Rot(i, j).perm();
            assert_eq!(p[i as usize], i as u8, "a4_rot_fixes_its_vertex");
        }
    }
    for k in a4::ALL_F3 {
        let p = a4::A4::Flip(k).perm();
        for i in 0..4usize {
            assert_ne!(p[i], i as u8, "a4_flip_no_fixed_point");
            assert_eq!(p[p[i] as usize], i as u8, "a4_flip_involution");
        }
    }
    assert_eq!(a4::order(a4::A4::Id), 1);
    assert_eq!(a4::order(a4::A4::Rot(a4::F4::F0, a4::F2::G0)), 3);
    assert_eq!(a4::order(a4::A4::Flip(a4::F3::H0)), 2);
}

// ── SL(2,F₃) ─────────────────────────────────────────────────────────────

#[test]
fn sl23_group() {
    let grp = sl23::all_sl23();
    assert_eq!(grp.len(), sl23::SL23_CARD, "sl23_card_24");
    assert!(grp.iter().all(|m| sl23::mat_det(*m) == sov_core::types::Trit::T1), "sl23_det_one");

    for g in grp {
        assert_eq!(sl23::mat_mul(*g, sl23::mat_id()), *g, "sl23_identity_l");
        assert_eq!(sl23::mat_mul(sl23::mat_id(), *g), *g, "sl23_identity_r");
        assert_eq!(sl23::mat_mul(*g, sl23::mat_inv(*g)), sl23::mat_id(), "sl23_inverse");
        for h in grp {
            assert!(grp.contains(&sl23::mat_mul(*g, *h)), "sl23_closed");
        }
    }

    assert_eq!(sl23::order_of(sl23::sigma1()), 3, "sl23_sigma1_order");
    assert_eq!(sl23::order_of(sl23::sigma2()), 3, "sl23_sigma2_order");
    assert_eq!(sl23::w_element(), sl23::neg_i(), "sl23_W_is_negI");
    assert_eq!(sl23::order_of(sl23::neg_i()), 2, "sl23_negI_order2");
    for m in grp {
        assert_eq!(
            sl23::mat_mul(*m, sl23::neg_i()),
            sl23::mat_mul(sl23::neg_i(), *m),
            "sl23_negI_central"
        );
    }

    let o4 = sl23::elems_of_order4();
    assert_eq!(o4.len(), 6, "sl23_has_order4");
    assert!(o4.iter().all(|m| sl23::mat_mul(*m, *m) == sl23::neg_i()), "sl23_all_order4_square_negI");
    assert!(a4::ALL_A4.iter().all(|g| a4::order(*g) != 4), "sl23_a4_has_no_order4");

    let orb = sl23::orbit_of_vec((sov_core::types::Trit::T1, sov_core::types::Trit::T0));
    assert_eq!(orb.len(), 8, "sl23_vec_transitive_8");
    for m in grp {
        assert_eq!(
            sl23::act_vec(*m, (sov_core::types::Trit::T0, sov_core::types::Trit::T0)),
            (sov_core::types::Trit::T0, sov_core::types::Trit::T0),
            "sl23_zero_fixed"
        );
    }
    assert_eq!(1 + orb.len(), 9, "sl23_nine_is_1_plus_8");
}

// ── 覆盖映射 ─────────────────────────────────────────────────────────────

#[test]
fn cover_map() {
    let grp = sl23::all_sl23();
    for g in grp {
        for h in grp {
            assert_eq!(
                cover::perm_of_m(sl23::mat_mul(*g, *h)),
                cover::compose_p(&cover::perm_of_m(*g), &cover::perm_of_m(*h)),
                "cover_homomorphism"
            );
        }
    }
    let img = cover::image_perms();
    assert!(img.iter().all(cover::is_even_p), "cover_image_even");
    assert_eq!(img.len(), 12, "cover_quotient_12");
    let mut a = img.clone();
    a.sort();
    let mut b = cover::a4_perm_set();
    b.sort();
    assert_eq!(a, b, "cover_image_is_a4");

    let ks = cover::kernel_of_cover();
    assert_eq!(ks.len(), 2, "cover_kernel_size");
    assert!(ks.contains(&sl23::mat_id()), "cover_kernel_has_I");
    assert!(ks.contains(&sl23::neg_i()), "cover_kernel_has_negI");
    assert!(cover::cover_ok(), "cover_ok");
}

// ── A₄ 表示论 ────────────────────────────────────────────────────────────

#[test]
fn a4_representations() {
    for rho in rep_a4::ALL_IRREPS {
        for sigma in rep_a4::ALL_IRREPS {
            let want = if rho == sigma { eis(12, 0) } else { Eis::ZERO };
            assert_eq!(rep_a4::chi_orth(rho, sigma), want, "rep_chi_orthonormal");
        }
        assert_eq!(
            rep_a4::regular_multiplicity_num(rho),
            eis(12 * rep_a4::dim_i(rho), 0),
            "rep_regular_multiplicity"
        );
        assert_eq!(
            rep_a4::chi(rho, a4::A4::Id),
            eis(rep_a4::dim_i(rho), 0),
            "rep_chi_at_identity_is_dim"
        );
    }
    assert_eq!(rep_a4::col_orth_at_id(), eis(12, 0), "rep_col_orth_at_id_12");
    assert!(rep_a4::one_dim_reps_non_faithful(), "rep_one_dim_non_faithful");

    for r in [rep_a4::Irrep::R1, rep_a4::Irrep::R1p, rep_a4::Irrep::R1pp] {
        for g in a4::ALL_A4 {
            for h in a4::ALL_A4 {
                assert_eq!(
                    rep_a4::chi(r, a4::mul(g, h)),
                    rep_a4::chi(r, g) * rep_a4::chi(r, h),
                    "rep_one_dim_hom"
                );
            }
        }
    }
    assert_eq!(rep_a4::chi(rep_a4::Irrep::R3, a4::A4::Id), eis(3, 0));
    assert_eq!(rep_a4::chi(rep_a4::Irrep::R3, a4::A4::Flip(a4::F3::H0)), eis(-1, 0));
    assert_eq!(rep_a4::chi(rep_a4::Irrep::R3, a4::A4::Rot(a4::F4::F0, a4::F2::G0)), Eis::ZERO);

    let mut n = [0usize; 4];
    for g in a4::ALL_A4 {
        let i = match rep_a4::class_of(g) {
            rep_a4::ChiClass::CE => 0,
            rep_a4::ChiClass::CV4 => 1,
            rep_a4::ChiClass::CA => 2,
            rep_a4::ChiClass::CB => 3,
        };
        n[i] += 1;
    }
    assert_eq!(n, [1, 3, 4, 4], "rep_class_partition_4_3_4_4");

    for g in a4::ALL_A4 {
        for h in a4::ALL_A4 {
            assert_eq!(
                rep_a4::class_of(a4::mul(a4::mul(h, g), a4::inv(h))),
                rep_a4::class_of(g),
                "rep_class_conj_invariant"
            );
        }
    }
}

// ── 2A₄ 表示论 + Z[ω] 模型 + C[2A₄] ─────────────────────────────────────

#[test]
fn rep_2a4_tables() {
    assert_eq!(rep_2a4::class_sizes_sum(), 24, "rep2_class_sizes_sum_24");
    assert_eq!(rep_2a4::class_sizes_sum(), rep_2a4::GROUP_ORDER);
    assert!(rep_2a4::spinor_orthogonality(), "rep2_spinor_orthogonality");

    let s2 = rep_2a4::spinor_char(rep_2a4::Spinor::S2);
    let s2p = rep_2a4::spinor_char(rep_2a4::Spinor::S2p);
    let s2pp = rep_2a4::spinor_char(rep_2a4::Spinor::S2pp);
    assert!(s2 != s2p && s2 != s2pp && s2p != s2pp, "rep2_spinor_distinct");
    let idc = rep_2a4::identity_class_index();
    assert_eq!(rep_2a4::at_e(&s2, idc), eis(2, 0), "rep2_spinor_dim_2_S2");
    assert_eq!(rep_2a4::at_e(&s2p, idc), eis(2, 0), "rep2_spinor_dim_2_S2p");
    assert_eq!(rep_2a4::at_e(&s2pp, idc), eis(2, 0), "rep2_spinor_dim_2_S2pp");

    assert!(rep_2a4::dims_identity(), "rep2_dims_identity_24");
    assert_eq!(rep_2a4::predicted_dims(), &[1, 1, 1, 2, 2, 2, 3], "rep2_predicted_dims");
    assert!(rep_2a4::class_structure_ok(), "rep2_class_structure_computed");
    assert_eq!(rep_2a4::sl23_classes().len(), 7, "rep2_class_count_7");
    assert!(rep_2a4::all_chars_orthogonal(), "rep2_all_chars_orthogonal");
    assert!(rep_2a4::all_chars_distinct(), "rep2_all_chars_distinct");

    let dims: Vec<Eis> = rep_2a4::all_chars().iter().map(|r| rep_2a4::at_e(r, idc)).collect();
    let want = vec![eis(1, 0), eis(1, 0), eis(1, 0), eis(2, 0), eis(2, 0), eis(2, 0), eis(3, 0)];
    assert_eq!(dims, want, "rep2_full_table_dims");

    assert_eq!(rep_2a4::class_info_c()[idc], (1, 1), "rep2_identity_class_size_order");
    assert_eq!(rep_2a4::class_signature(), rep_2a4::EXPECTED_SIGNATURE, "rep2_class_order_signature");

    for i in 0..7 {
        assert_eq!(rep_2a4::neg_class_of(rep_2a4::neg_class_of(i)), i, "rep2_neg_class_involutive");
    }
    assert_eq!(rep_2a4::one_dim_homs().len(), 3, "rep2_one_dim_homs_3");
    assert!(rep_2a4::regular_multiplicity_ok(), "rep2_regular_multiplicity");

    // Z[ω] 模型
    assert!(c2a4::model_ok(), "c2a4_model_ok");
    assert_eq!(c2a4::class_info_m(), rep_2a4::class_signature(), "c2a4_class_signature_matches");
    assert_eq!(c2a4::group24().len(), 24, "c2a4_24_elements");
    assert_eq!(c2a4::m_index().len(), 24, "c2a4_one_dim_index_24");
    assert!(c2a4::tensor_reps_hom(), "c2a4_tensor_reps_hom");
    assert!(c2a4::tensor_reps_faithful(), "c2a4_tensor_reps_faithful");

    for &(sz, od) in rep_2a4::class_info_c() {
        assert_eq!(
            rep_2a4::spinor_value(od),
            c2a4::spinor_char_from_model(sz, od),
            "rep2_spinor_matches_model"
        );
    }
    for i in 0..7 {
        let od = rep_2a4::class_info_c()[i].1;
        let neg = rep_2a4::neg_class_of(i);
        let neg_od = rep_2a4::class_info_c()[neg].1;
        if od == 3 {
            assert_eq!(neg_od, 6, "rep2_neg_pairs_3_6");
        }
        if od == 6 {
            assert_eq!(neg_od, 3, "rep2_neg_pairs_6_3");
        }
        if od == 4 {
            assert_eq!(neg, i, "rep2_order4_neg_fixed");
        }
    }
}

// ── C[A₄] ────────────────────────────────────────────────────────────────

#[test]
fn carrier_a4_tests() {
    assert_eq!(carrier_a4::DIM_CARRIER, 12, "carrier_dim");
    for g in a4::ALL_A4 {
        assert_eq!(carrier_a4::norm_c2(&carrier_a4::basis_vec(g)), eis(1, 0), "carrier_parseval_delta");
        for h in a4::ALL_A4 {
            let want = if g == h { eis(1, 0) } else { Eis::ZERO };
            assert_eq!(
                carrier_a4::dot_c(&carrier_a4::basis_vec(g), &carrier_a4::basis_vec(h)),
                want,
                "carrier_basis_orthonormal"
            );
        }
    }
    assert_eq!(carrier_a4::plancherel_sum(), 12, "plancherel_identity");

    for g in a4::ALL_A4 {
        for h in a4::ALL_A4 {
            for i in a4::ALL_A4 {
                let lhs = carrier_a4::reg_act(g, &carrier_a4::reg_act(h, &carrier_a4::basis_vec(i)));
                let rhs = carrier_a4::reg_act(a4::mul(g, h), &carrier_a4::basis_vec(i));
                assert_eq!(lhs, rhs, "carrier_regular_action_hom");
            }
        }
    }
    for g in a4::ALL_A4 {
        for i in a4::ALL_A4 {
            for j in a4::ALL_A4 {
                assert_eq!(
                    carrier_a4::dot_c(
                        &carrier_a4::reg_act(g, &carrier_a4::basis_vec(i)),
                        &carrier_a4::reg_act(g, &carrier_a4::basis_vec(j))
                    ),
                    carrier_a4::dot_c(&carrier_a4::basis_vec(i), &carrier_a4::basis_vec(j)),
                    "carrier_unitary"
                );
            }
        }
    }
}

// ── Hopf 商 ──────────────────────────────────────────────────────────────

#[test]
fn hopf_quotient() {
    assert_eq!(hopf::all_t6_points().len(), 729, "hopf_points_729");
    for g in a4::ALL_A4 {
        for x in hopf::all_t6_points() {
            assert_eq!(hopf::proj_class(&hopf::act_on(g, x)), hopf::proj_class(x), "hopf_orbit_invariant");
        }
    }

    let mut classes: Vec<usize> = hopf::all_t6_points().iter().map(hopf::proj_class).collect();
    classes.sort();
    classes.dedup();
    assert_eq!(classes.len(), 135, "hopf_base_card_135");
    assert_eq!(hopf::base_card(), 135);

    assert_eq!(
        hopf::fiber_size_distribution(),
        &[(1, 27), (4, 54), (6, 27), (12, 27)],
        "hopf_fiber_distribution"
    );
    let sum: usize = hopf::fiber_size_distribution().iter().map(|(s, n)| s * n).sum();
    assert_eq!(sum, 729, "hopf_fiber_sum_729");
    assert!(hopf::fiber_size_distribution().iter().all(|(s, _)| *s <= 12), "hopf_no_54_orbit");

    for x in hopf::all_t6_points() {
        let same = hopf::all_t6_points().iter().filter(|y| hopf::proj_class(y) == hopf::proj_class(x)).count();
        assert_eq!(same, hopf::fiber_size_of(x), "hopf_key_complete");
    }
}

// ── 旋量 Hopf ────────────────────────────────────────────────────────────

#[test]
fn spin_hopf_tests() {
    assert_eq!(spin_hopf::SPIN_FIBER_CARD, 9, "spin_fiber_card_9");
    assert_eq!(spin_hopf::SPIN_TOTAL_CARD, 6561, "spin_total_card_6561");
    assert_eq!(sl23::order_of(spin_hopf::alpha_mat()), 4, "spin_alpha_order4");
    assert_eq!(spin_hopf::alpha_pow(2), sl23::neg_i(), "spin_alpha_sq_is_negI");
    assert_eq!(spin_hopf::alpha_pow(4), sl23::mat_id(), "spin_alpha_fourth_id");

    for x in 0u8..3 {
        for y in 0u8..3 {
            let v = (sov_core::types::Trit::from_u8(x), sov_core::types::Trit::from_u8(y));
            let n = (sov_a4::f3_neg(v.0), sov_a4::f3_neg(v.1));
            assert_eq!(spin_hopf::phase_act(2, v), n, "spin_negI_negates");
            assert_eq!(sl23::act_vec(sl23::w_element(), v), n, "spin_W_negates");
        }
    }
    assert_eq!(
        spin_hopf::phase_act(1, (sov_core::types::Trit::T0, sov_core::types::Trit::T0)),
        (sov_core::types::Trit::T0, sov_core::types::Trit::T0),
        "spin_alpha_fixes_zero"
    );

    for g in a4::ALL_A4 {
        for k in 0..4u32 {
            for x in hopf::all_t6_points() {
                let s = (*x, (sov_core::types::Trit::T1, sov_core::types::Trit::T0));
                assert_eq!(spin_hopf::spin_proj(&spin_hopf::spin_act(g, k, s)), hopf::proj_class(x), "spin_proj_invariant");
            }
        }
    }

    let orbits = spin_hopf::phase_orbits();
    assert_eq!(orbits.len(), 2, "spin_phase_orbits_count");
    assert_eq!(orbits[0].len(), 4, "spin_phase_orbit0_len");
    assert_eq!(orbits[1].len(), 4, "spin_phase_orbit1_len");
    assert_eq!(spin_hopf::spin_orbit_reps().len(), 135, "spin_orbit_reps_135");
}

// ── Spin2 / Spin3 ────────────────────────────────────────────────────────

#[test]
fn spin_criteria_tests() {
    assert_eq!(spin::det_commutator2(), eis(2, 0), "spin2_det_commutator_2");
    assert!(spin::no_common_eigenvector2(), "spin2_no_common_eigenvector");
    assert!(spin::spin2_ok(), "spin2_ab_ba_pinned");
    assert!(spin::spin3_ok(), "spin3_closed_products");
    assert_eq!(spin::det_rows(), eis(4, 0), "spin3_det_rows_4");
    assert!(spin::no_common_eigenvector3(), "spin3_no_common_eigenvector");
}

// ── C[2A₄] ───────────────────────────────────────────────────────────────

#[test]
fn carrier_2a4_tests() {
    assert!(carrier_2a4::carrier2_ok(), "c2a4_carrier_ok");
    assert!(carrier_2a4::delta_orthonormal(), "c2a4_delta_orthonormal");
    assert!(carrier_2a4::reg_act2_hom(), "c2a4_regact2_hom");
    assert!(carrier_2a4::reg_act2_unitary(), "c2a4_regact2_unitary");
    assert!(carrier_2a4::gram_hermitian(), "c2a4_coefficient_gram_hermitian");
    assert!(carrier_2a4::coefficients_independent(), "c2a4_coefficients_independent");
    assert!(carrier_2a4::bilinear_schur(), "c2a4_bilinear_schur_12");
    assert!(carrier_2a4::quaternionic(), "c2a4_fs_indicator_neg1");
}

// ── 截面 + 扭转缺陷 ──────────────────────────────────────────────────────

#[test]
fn section_tests() {
    assert_eq!(section::base_dim(), 135, "field_base_135");
    assert_eq!(section::fiber_dim(), 12, "field_fiber_12");
    assert_eq!(section::flat_dim(), 1620, "field_flat_1620");
    assert_eq!(section::flat_dim(), 135 * 12);
    assert_eq!(section::effective_dim(), 729, "field_effective_729");
    assert_eq!(section::overcount(), 891, "field_overcount_891");
    assert_eq!(section::flat_dim() - section::effective_dim(), section::overcount());
    assert_eq!(section::free_orbit_count(), 27, "field_free_orbits_27");
    assert_eq!(section::non_free_orbit_count(), 108, "field_nonfree_108");
    assert_eq!(section::free_orbit_count() + section::non_free_orbit_count(), 135);

    let xs = vec![carrier_a4::zero_c(); 135];
    assert!(section::mk_section(xs).is_some(), "field_section_length");
    let short = vec![carrier_a4::zero_c(); 134];
    assert!(section::mk_section(short).is_none(), "field_section_reject");
    assert_eq!(section::section_norm(&section::zero_section(), 0), Eis::ZERO, "field_zero_norm");

    assert!(section::mk_base_key(135).is_none(), "field_basekey_135");
    assert!(section::mk_base_key(-1).is_none(), "field_basekey_neg");
    assert_eq!(section::mk_base_key(0), Some(0));
    assert_eq!(section::all_base_keys().len(), 135);

    assert_eq!(section::min_fiber_size(), 1, "field_min_fiber_1");
    assert_eq!(section::max_fiber_size(), 12, "field_max_fiber_12");
    assert!(!section::uniform_coords_faithful(), "field_uniform_coords_not_faithful");

    let rows = section::overcount_by_orbit();
    let want = [
        section::OvercountRow { size: 1, count: 27, deficit: 297 },
        section::OvercountRow { size: 4, count: 54, deficit: 432 },
        section::OvercountRow { size: 6, count: 27, deficit: 162 },
        section::OvercountRow { size: 12, count: 27, deficit: 0 },
    ];
    assert_eq!(rows, want, "field_overcount_by_orbit");
    assert!(section::overcount_decomposition(), "field_overcount_decomposition");
    assert!(!section::equivariant_section_possible(), "field_no_equivariant_section");
}
