// test_a4_family.cpp — A₄/2A₄ 族移植测试（cpp 侧）
//
// 断言集与 hs/sov-math/test/Sov/Tests/Core.hs 的 carrierTests / sl23Tests /
// spinHopfTests / fieldTests / repA4Tests / rep2A4Tests 逐条对应；
// 由 rust/sov-math/sov-a4/tests/ 同名断言镜像三语言等价。
//
// 口径：全部为**有限集全枚举**（12 / 24 / 576 / 729 点级），不是归纳证明。
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "a4_group.h"
#include "carrier_2a4.h"
#include "carrier_a4.h"
#include "complex_2a4.h"
#include "constitutional.h"
#include "cover_2a4.h"
#include "hopf_quotient.h"
#include "rep_2a4.h"
#include "rep_a4.h"
#include "section_field.h"
#include "sl23.h"
#include "spin_criteria.h"
#include "spin_hopf.h"

namespace {

using sov::math::eis::Eis;

int g_pass = 0;
int g_fail = 0;

void check(const std::string& name, bool ok) {
    if (ok) { ++g_pass; }
    else { ++g_fail; std::printf("\xf0\x9f\x94\xb4 %s\n", name.c_str()); }
}

// ── A₄ ────────────────────────────────────────────────────────────────────

void test_a4() {
    using namespace sov::math::a4;

    check("a4_card", ALL_A4.size() == static_cast<std::size_t>(A4_CARD) && A4_CARD == 12);

    // 12 个置换互不相同
    {
        std::vector<std::array<std::uint8_t, 4>> ps;
        for (const auto& g : ALL_A4) ps.push_back(perm(g));
        std::sort(ps.begin(), ps.end());
        check("a4_perm_distinct",
              static_cast<int>(std::unique(ps.begin(), ps.end()) - ps.begin()) == 12);
        check("a4_perm_even", std::all_of(ps.begin(), ps.end(), is_even_perm));

        // 与"全部 4 点偶置换"逐项相等
        // （不用 next_permutation：GCC 15 在 4 元素 array 上会误报 stringop-overflow）
        std::vector<std::array<std::uint8_t, 4>> evens;
        for (std::uint8_t a = 0; a < 4; ++a)
            for (std::uint8_t b = 0; b < 4; ++b) {
                if (b == a) continue;
                for (std::uint8_t c = 0; c < 4; ++c) {
                    if (c == a || c == b) continue;
                    const std::uint8_t d = static_cast<std::uint8_t>(6 - a - b - c);
                    const std::array<std::uint8_t, 4> q{a, b, c, d};
                    if (is_even_perm(q)) evens.push_back(q);
                }
            }
        std::sort(evens.begin(), evens.end());
        check("a4_perm_set_equals_alt4", ps == evens);
    }

    bool ident_l = true, ident_r = true, inverses = true, assoc = true;
    for (const auto& g : ALL_A4) {
        if (mul(A4::id(), g) != g) ident_l = false;
        if (mul(g, A4::id()) != g) ident_r = false;
        if (mul(g, inv(g)) != A4::id() || mul(inv(g), g) != A4::id()) inverses = false;
    }
    for (const auto& g : ALL_A4)
        for (const auto& h : ALL_A4)
            for (const auto& k : ALL_A4)
                if (mul(mul(g, h), k) != mul(g, mul(h, k))) assoc = false;

    check("a4_identity_l", ident_l);
    check("a4_identity_r", ident_r);
    check("a4_inverse", inverses);
    check("a4_assoc", assoc);
    check("a4_generated_by_two_elements", generated_by_two_elements());

    // Rot i zero 固定顶点 i
    bool rot_fixes = true;
    for (const auto i : ALL_F4)
        for (const auto j : ALL_F2)
            if (perm(A4::rot(i, j))[static_cast<std::size_t>(i)] != static_cast<std::uint8_t>(i))
                rot_fixes = false;
    check("a4_rot_fixes_its_vertex", rot_fixes);

    // Flip 是无不动点的对合（三个双对换）
    bool flip_ok = true;
    for (const auto k : ALL_F3) {
        const auto p = perm(A4::flip(k));
        for (std::uint8_t i = 0; i < 4; ++i) {
            if (p[i] == i) flip_ok = false;                 // 无不动点
            if (p[p[i]] != i) flip_ok = false;              // 对合
        }
    }
    check("a4_flip_two_transpositions", flip_ok);

    // 阶：Id=1，Rot=3，Flip=2
    check("a4_orders", order(A4::id()) == 1 && order(A4::rot(F4::F0, F2::G0)) == 3
                       && order(A4::flip(F3::H0)) == 2);

    // index / from_index 互逆
    bool idx_roundtrip = true;
    for (const auto& g : ALL_A4)
        if (from_index(g.index()) != g) idx_roundtrip = false;
    check("a4_index_roundtrip", idx_roundtrip);
}

// ── SL(2,F₃) ──────────────────────────────────────────────────────────────

void test_sl23() {
    using namespace sov::math::sl23;

    const auto& grp = all_sl23();
    check("sl23_card_24", grp.size() == 24 && SL23_CARD == 24);

    bool det_one = true, closed = true, identity = true, inverses = true;
    for (const auto& m : grp) if (mat_det(m) != 1) det_one = false;
    for (const auto& g : grp) {
        if (mat_mul(g, mat_id()) != g || mat_mul(mat_id(), g) != g) identity = false;
        if (mat_mul(g, mat_inv(g)) != mat_id() || mat_mul(mat_inv(g), g) != mat_id()) inverses = false;
        for (const auto& h : grp) {
            const auto p = mat_mul(g, h);
            bool found = false;
            for (const auto& x : grp) if (x == p) { found = true; break; }
            if (!found) closed = false;
        }
    }
    check("sl23_det_one", det_one);
    check("sl23_closed", closed);
    check("sl23_identity", identity);
    check("sl23_inverse", inverses);

    check("sl23_sigma_orders", order_of(sigma1) == 3 && order_of(sigma2) == 3);
    check("sl23_W_is_negI", w_element() == neg_i);
    check("sl23_negI_order2", order_of(neg_i) == 2);

    bool central = true;
    for (const auto& m : grp)
        if (mat_mul(m, neg_i) != mat_mul(neg_i, m)) central = false;
    check("sl23_negI_central", central);

    const auto o4 = elems_of_order4();
    check("sl23_has_order4", o4.size() == 6);
    bool square_neg_i = true;
    for (const auto& m : o4) if (mat_mul(m, m) != neg_i) square_neg_i = false;
    check("sl23_all_order4_square_negI", square_neg_i);

    // A₄ 没有 4 阶元
    bool a4_no4 = true;
    for (const auto& g : sov::math::a4::ALL_A4)
        if (sov::math::a4::order(g) == 4) a4_no4 = false;
    check("sl23_a4_has_no_order4", a4_no4);

    // 8 个非零向量上传递；0 固定；9 = 1 + 8
    const auto orb = orbit_of_vec(Vec2{1, 0});
    check("sl23_vec_transitive_8", orb.size() == 8);
    bool zero_fixed = true;
    for (const auto& m : grp)
        if (act_vec(m, Vec2{0, 0}) != Vec2{0, 0}) zero_fixed = false;
    check("sl23_zero_fixed", zero_fixed);
    check("sl23_nine_is_1_plus_8", 1 + orb.size() == 9);

    // 非零向量全覆盖（8 个向量都在同一条轨道里）
    {
        std::vector<Vec2> all_orb;
        for (const auto& v : nonzero_vectors())
            for (const auto& w : orbit_of_vec(v)) {
                bool seen = false;
                for (const auto& x : all_orb) if (x == w) { seen = true; break; }
                if (!seen) all_orb.push_back(w);
            }
        check("sl23_all_nonzero_in_one_orbit", all_orb.size() == 8);
    }
}

// ── 覆盖映射 ──────────────────────────────────────────────────────────────

void test_cover() {
    using namespace sov::math::cover2a4;

    const auto& grp = sov::math::sl23::all_sl23();

    bool hom = true;
    for (const auto& g : grp)
        for (const auto& h : grp)
            if (perm_of_m(sov::math::sl23::mat_mul(g, h)) != compose_p(perm_of_m(g), perm_of_m(h)))
                hom = false;
    check("cover_homomorphism", hom);

    const auto img = image_perms();
    bool even = true;
    for (const auto& p : img) if (!is_even_p(p)) even = false;
    check("cover_image_even", even);
    check("cover_image_is_a4", cover_ok() || img.size() == 12);
    check("cover_quotient_12", img.size() == 12);

    const auto ks = kernel_of_cover();
    check("cover_kernel_is_pm_I",
          ks.size() == 2 && kernel_contains(ks, sov::math::sl23::mat_id())
          && kernel_contains(ks, sov::math::sl23::neg_i));
    check("cover_ok", cover_ok());
}

// ── A₄ 表示论 ─────────────────────────────────────────────────────────────

void test_rep_a4() {
    namespace RA4 = sov::math::rep_a4;
    using RA4::ChiClass;
    using RA4::Irrep;

    bool orth = true, regular = true, one_dim_hom = true, conj_inv = true;
    for (const auto rho : RA4::ALL_IRREPS)
        for (const auto sigma : RA4::ALL_IRREPS) {
            const Eis want = (rho == sigma) ? Eis{12, 0} : Eis{0, 0};
            if (RA4::chi_orth(rho, sigma) != want) orth = false;
        }
    for (const auto r : RA4::ALL_IRREPS)
        if (RA4::regular_multiplicity_num(r) != Eis{12 * RA4::dim_i(r), 0}) regular = false;

    check("rep_chi_orthonormal", orth);
    check("rep_regular_multiplicity", regular);
    check("rep_col_orth_at_id_12", RA4::col_orth_at_id() == Eis{12, 0});
    check("rep_one_dim_non_faithful", RA4::one_dim_reps_non_faithful());

    for (const auto r : {Irrep::R1, Irrep::R1p, Irrep::R1pp})
        for (const auto& g : sov::math::a4::ALL_A4)
            for (const auto& h : sov::math::a4::ALL_A4)
                if (RA4::chi(r, sov::math::a4::mul(g, h))
                    != RA4::chi(r, g) * RA4::chi(r, h))
                    one_dim_hom = false;
    check("rep_one_dim_hom", one_dim_hom);

    check("rep_three_dim_values",
          RA4::chi(Irrep::R3, sov::math::a4::A4::id()) == Eis{3, 0}
              && RA4::chi(Irrep::R3, sov::math::a4::A4::flip(sov::math::a4::F3::H0))
                     == Eis{-1, 0}
              && RA4::chi(Irrep::R3, sov::math::a4::A4::rot(sov::math::a4::F4::F0,
                                                            sov::math::a4::F2::G0))
                     == Eis{0, 0});

    bool at_id_dim = true;
    for (const auto r : RA4::ALL_IRREPS)
        if (RA4::chi(r, sov::math::a4::A4::id()) != Eis{RA4::dim_i(r), 0}) at_id_dim = false;
    check("rep_chi_at_identity_is_dim", at_id_dim);

    int n_ce = 0, n_cv4 = 0, n_ca = 0, n_cb = 0;
    for (const auto& g : sov::math::a4::ALL_A4) {
        switch (RA4::class_of(g)) {
            case ChiClass::CE: ++n_ce; break;
            case ChiClass::CV4: ++n_cv4; break;
            case ChiClass::CA: ++n_ca; break;
            case ChiClass::CB: ++n_cb; break;
        }
    }
    check("rep_class_partition_4_3_4_4", n_ce == 1 && n_cv4 == 3 && n_ca == 4 && n_cb == 4);

    for (const auto& g : sov::math::a4::ALL_A4)
        for (const auto& h : sov::math::a4::ALL_A4)
            if (RA4::class_of(sov::math::a4::mul(sov::math::a4::mul(h, g),
                                                 sov::math::a4::inv(h)))
                != RA4::class_of(g))
                conj_inv = false;
    check("rep_class_conj_invariant", conj_inv);
}

// ── 2A₄ 表示论 + Z[ω] 模型 + C[2A₄] 判据 ──────────────────────────────────

void test_rep_2a4() {
    namespace R2 = sov::math::rep_2a4;
    namespace C2 = sov::math::c2a4;

    check("rep2_class_sizes_sum_24",
          R2::class_sizes_sum() == 24 && R2::class_sizes_sum() == R2::GROUP_ORDER);
    check("rep2_spinor_orthogonality", R2::spinor_orthogonality());
    check("rep2_spinor_distinct",
          R2::spinor_char(R2::Spinor::S2) != R2::spinor_char(R2::Spinor::S2p)
              && R2::spinor_char(R2::Spinor::S2) != R2::spinor_char(R2::Spinor::S2pp)
              && R2::spinor_char(R2::Spinor::S2p) != R2::spinor_char(R2::Spinor::S2pp));
    check("rep2_spinor_dim_2",
          R2::at_e(R2::spinor_char(R2::Spinor::S2), R2::identity_class_index()) == Eis{2, 0}
              && R2::at_e(R2::spinor_char(R2::Spinor::S2p), R2::identity_class_index())
                     == Eis{2, 0}
              && R2::at_e(R2::spinor_char(R2::Spinor::S2pp), R2::identity_class_index())
                     == Eis{2, 0});
    check("rep2_dims_identity_24", R2::dims_identity());
    check("rep2_predicted_dims",
          R2::predicted_dims() == std::vector<int>({1, 1, 1, 2, 2, 2, 3}));
    check("rep2_class_structure_computed", R2::class_structure_ok());
    check("rep2_class_count_7", R2::sl23_classes().size() == 7);
    check("rep2_all_chars_orthogonal", R2::all_chars_orthogonal());
    check("rep2_all_chars_distinct", R2::all_chars_distinct());

    {
        std::vector<Eis> dims;
        for (const auto& row : R2::all_chars())
            dims.push_back(R2::at_e(row, R2::identity_class_index()));
        std::vector<Eis> want;
        for (int i = 0; i < 3; ++i) want.push_back(Eis{1, 0});
        for (int i = 0; i < 3; ++i) want.push_back(Eis{2, 0});
        want.push_back(Eis{3, 0});
        check("rep2_full_table_dims", dims == want);
    }

    check("rep2_identity_class_size_order",
          R2::class_info_c()[static_cast<std::size_t>(R2::identity_class_index())]
              == std::pair<int, int>{1, 1});
    check("rep2_class_order_signature", R2::class_signature() == R2::expected_signature());

    bool neg_invol = true;
    for (int i = 0; i < 7; ++i)
        if (R2::neg_class_of(R2::neg_class_of(i)) != i) neg_invol = false;
    check("rep2_neg_class_involutive", neg_invol);

    check("rep2_one_dim_homs_3", R2::one_dim_homs().size() == 3);
    check("rep2_regular_multiplicity", R2::regular_multiplicity_ok());

    // ── Z[ω] 模型（χ = tr）
    check("c2a4_model_ok", C2::model_ok());
    check("c2a4_class_signature_matches", C2::class_info_m() == R2::class_signature());
    check("c2a4_24_elements_distinct", C2::group24().size() == 24);
    check("c2a4_one_dim_index_24", C2::m_index().size() == 24);
    check("c2a4_tensor_reps_hom", C2::tensor_reps_hom());
    check("c2a4_tensor_reps_faithful", C2::tensor_reps_faithful());

    check("rep2_spinor_matches_model", [&] {
        for (const auto& [sz, od] : R2::class_info_c())
            if (R2::spinor_value(od) != C2::spinor_char_from_model(sz, od)) return false;
        return true;
    }());

    check("rep2_neg_pairs_3_6", [&] {
        for (int i = 0; i < 7; ++i) {
            const int od = R2::class_info_c()[static_cast<std::size_t>(i)].second;
            const int neg = R2::neg_class_of(i);
            const int neg_od = R2::class_info_c()[static_cast<std::size_t>(neg)].second;
            if (od == 3 && neg_od != 6) return false;
            if (od == 6 && neg_od != 3) return false;
            if (od == 4 && neg != i) return false;
        }
        return true;
    }());
}

// ── C[A₄] 载体 + Plancherel ───────────────────────────────────────────────

void test_carrier_a4() {
    namespace CA = sov::math::carrier_a4;
    using sov::math::a4::ALL_A4;
    using sov::math::a4::A4;

    check("carrier_dim", CA::DIM_CARRIER == 12);

    bool orth = true, parseval = true, reg_hom = true, unitary = true;
    for (const auto& g : ALL_A4)
        for (const auto& h : ALL_A4) {
            const Eis want = (g == h) ? Eis{1, 0} : Eis{0, 0};
            if (CA::dot_c(CA::basis_vec(g), CA::basis_vec(h)) != want) orth = false;
        }
    for (const auto& g : ALL_A4)
        if (CA::norm_c2(CA::basis_vec(g)) != Eis{1, 0}) parseval = false;

    for (const auto& g : ALL_A4)
        for (const auto& h : ALL_A4)
            for (const auto& i : ALL_A4) {
                const auto lhs = CA::reg_act(g, CA::reg_act(h, CA::basis_vec(i)));
                const auto rhs = CA::reg_act(sov::math::a4::mul(g, h), CA::basis_vec(i));
                if (lhs != rhs) reg_hom = false;
            }

    for (const auto& g : ALL_A4)
        for (const auto& i : ALL_A4)
            for (const auto& j : ALL_A4)
                if (CA::dot_c(CA::reg_act(g, CA::basis_vec(i)), CA::reg_act(g, CA::basis_vec(j)))
                    != CA::dot_c(CA::basis_vec(i), CA::basis_vec(j)))
                    unitary = false;

    check("carrier_basis_orthonormal", orth);
    check("carrier_parseval_delta", parseval);
    check("plancherel_identity", CA::plancherel_sum() == 12);
    check("carrier_regular_action_hom", reg_hom);
    check("carrier_unitary", unitary);
}

// ── 离散 Hopf 纤维化 T⁶ → T⁶/A₄ ─────────────────────────────────────────

void test_hopf() {
    namespace H = sov::math::hopf;
    using sov::math::a4::ALL_A4;

    check("hopf_points_729", H::all_t6_points().size() == 729);

    bool invariant = true;
    for (const auto& g : ALL_A4)
        for (const auto& x : H::all_t6_points())
            if (H::proj_class(H::act_on(g, x)) != H::proj_class(x)) invariant = false;
    check("hopf_orbit_invariant", invariant);

    {
        std::vector<int> seen;
        for (const auto& x : H::all_t6_points()) {
            const int c = H::proj_class(x);
            bool dup = false;
            for (const int s : seen) if (s == c) { dup = true; break; }
            if (!dup) seen.push_back(c);
        }
        check("hopf_base_card_135", seen.size() == 135 && H::base_card() == 135);
    }

    const auto& dist = H::fiber_size_distribution();
    const std::vector<std::pair<int, int>> want{{1, 27}, {4, 54}, {6, 27}, {12, 27}};
    check("hopf_fiber_distribution", dist == want);

    int sum = 0;
    for (const auto& [s, n] : dist) sum += s * n;
    check("hopf_fiber_sum_729", sum == 729);

    bool no54 = true;
    for (const auto& [s, n] : dist) if (s > 12) no54 = false;
    check("hopf_no_54_orbit", no54);

    bool key_complete = true;
    for (const auto& x : H::all_t6_points()) {
        int same = 0;
        for (const auto& y : H::all_t6_points())
            if (H::proj_class(y) == H::proj_class(x)) ++same;
        if (same != H::fiber_size_of(x)) key_complete = false;
    }
    check("hopf_key_complete", key_complete);
}

// ── 旋量 Hopf 丛 T⁶ × GF(3)² ─────────────────────────────────────────────

void test_spin_hopf() {
    namespace SH = sov::math::spin_hopf;
    namespace SL = sov::math::sl23;
    namespace H = sov::math::hopf;
    using sov::math::a4::ALL_A4;

    check("spin_fiber_card_9", SH::SPIN_FIBER_CARD == 9);
    check("spin_total_card_6561", SH::SPIN_TOTAL_CARD == 6561);
    check("spin_alpha_order4", SL::order_of(SH::alpha_mat()) == 4);
    check("spin_alpha_sq_is_negI", SH::alpha_pow(2) == SL::neg_i);
    check("spin_alpha_fourth_id", SH::alpha_pow(4) == SL::mat_id());

    bool negates = true, w_negates = true, fixes_zero = true;
    for (sov::math::Trit x = 0; x < 3; ++x)
        for (sov::math::Trit y = 0; y < 3; ++y) {
            const SH::SpinFiber v{x, y};
            const SH::SpinFiber n{SL::f3_neg(x), SL::f3_neg(y)};
            if (SH::phase_act(2, v) != n) negates = false;
            if (SL::act_vec(SL::w_element(), v) != n) w_negates = false;
        }
    if (SH::phase_act(1, SH::SpinFiber{0, 0}) != SH::SpinFiber{0, 0}) fixes_zero = false;

    check("spin_negI_negates", negates);
    check("spin_W_negates", w_negates);
    check("spin_alpha_fixes_zero", fixes_zero);

    bool proj_inv = true;
    for (const auto& g : ALL_A4)
        for (unsigned k = 0; k < 4; ++k)
            for (const auto& x : H::all_t6_points())
                if (SH::spin_proj(SH::spin_act({g, k}, {x, SH::SpinFiber{1, 0}}))
                    != H::proj_class(x))
                    proj_inv = false;
    check("spin_proj_invariant", proj_inv);

    bool orbits_2x4 = SH::phase_orbits().size() == 2
                   && SH::phase_orbits()[0].size() == 4
                   && SH::phase_orbits()[1].size() == 4;
    check("spin_phase_orbits_2x4", orbits_2x4);
    check("spin_orbit_reps_135", SH::spin_orbit_reps().size() == 135);
}

// ── Spin2 / Spin3 不可约判据 ─────────────────────────────────────────────

void test_spin_criteria() {
    namespace SC = sov::math::spin_criteria;

    check("spin2_det_commutator_2", SC::det_commutator2() == Eis{2, 0});
    check("spin2_no_common_eigenvector", SC::no_common_eigenvector2());
    check("spin2_ab_ba_pinned", SC::spin2_ok());
    check("spin3_closed_products", SC::spin3_ok());
    check("spin3_det_rows_4", SC::det_rows() == Eis{4, 0});
    check("spin3_no_common_eigenvector", SC::no_common_eigenvector3());
}

// ── C[2A₄] + 双线性 Schur ────────────────────────────────────────────────

void test_carrier_2a4() {
    namespace K2 = sov::math::carrier_2a4;

    check("c2a4_carrier_ok", K2::carrier2_ok());
    check("c2a4_delta_orthonormal", K2::delta_orthonormal());
    check("c2a4_regact2_hom", K2::reg_act2_hom());
    check("c2a4_regact2_unitary", K2::reg_act2_unitary());
    check("c2a4_coefficient_gram_hermitian", K2::gram_hermitian());
    check("c2a4_coefficients_independent", K2::coefficients_independent());
    check("c2a4_bilinear_schur_12", K2::bilinear_schur());
    check("c2a4_fs_indicator_neg1", K2::quaternionic());
}

// ── 截面空间 + 扭转缺陷 ──────────────────────────────────────────────────

void test_section() {
    namespace FD = sov::math::section;
    namespace CA = sov::math::carrier_a4;

    check("field_base_135", FD::base_dim() == 135);
    check("field_fiber_12", FD::fiber_dim() == 12);
    check("field_flat_1620", FD::flat_dim() == 1620 && FD::flat_dim() == 135 * 12);
    check("field_effective_729", FD::effective_dim() == 729);
    check("field_overcount_891",
          FD::overcount() == 891 && FD::flat_dim() - FD::effective_dim() == FD::overcount());
    check("field_free_orbits_27", FD::free_orbit_count() == 27);
    check("field_nonfree_108",
          FD::non_free_orbit_count() == 108
              && FD::free_orbit_count() + FD::non_free_orbit_count() == 135);

    {
        std::vector<CA::Carrier> xs(135, CA::zero_c());
        check("field_section_length", FD::mk_section(xs).has_value());
        xs.pop_back();
        check("field_section_reject", !FD::mk_section(xs).has_value());
    }

    check("field_zero_norm", FD::section_norm(FD::zero_section(), 0) == Eis{0, 0});
    check("field_basekey_rejects_out_of_range",
          !FD::mk_base_key(135).has_value() && !FD::mk_base_key(-1).has_value()
              && FD::mk_base_key(0).has_value() && FD::all_base_keys().size() == 135);
    check("field_min_fiber_1", FD::min_fiber_size() == 1 && FD::max_fiber_size() == 12);
    check("field_uniform_coords_not_faithful", !FD::uniform_coords_faithful());

    {
        const auto& rows = FD::overcount_by_orbit();
        const std::vector<FD::OvercountRow> want{{1, 27, 297}, {4, 54, 432},
                                                 {6, 27, 162}, {12, 27, 0}};
        bool same = rows.size() == want.size();
        if (same)
            for (std::size_t i = 0; i < rows.size(); ++i)
                if (rows[i].size != want[i].size || rows[i].count != want[i].count
                    || rows[i].deficit != want[i].deficit)
                    same = false;
        check("field_overcount_by_orbit", same);
    }
    check("field_overcount_decomposition", FD::overcount_decomposition());
    check("field_no_equivariant_section", !FD::equivariant_section_possible());
}

} // namespace

int main() {
    test_a4();
    test_sl23();
    test_cover();
    test_rep_a4();
    test_rep_2a4();
    test_carrier_a4();
    test_hopf();
    test_spin_hopf();
    test_spin_criteria();
    test_carrier_2a4();
    test_section();

    std::printf("%s\n", std::string(40, '-').c_str());
    if (g_fail == 0) {
        std::printf("\xe2\x9c\x85 全部通过: %d/%d\n", g_pass, g_pass + g_fail);
        return 0;
    }
    std::printf("\xf0\x9f\x94\xb4 失败 %d 项 (通过 %d)\n", g_fail, g_pass);
    return 1;
}
