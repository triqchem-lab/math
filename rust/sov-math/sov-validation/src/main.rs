mod zomega;
mod magic_square;
mod yanghui;
mod ternary_magic;
mod gf4_magic;
mod chi_inner;
mod gf9_diag;
mod em_frequency_validation;
mod em_discrete_action_validation;

fn main() {
    println!("=== Z[ω,i] 环乘法验证 ===");
    zomega::verify_all();

    println!("\n=== 3×3 幻方与正交拉丁方 ===");
    magic_square::verify_all();

    println!("\n=== 杨辉三角模 3 ===");
    yanghui::verify_all();

    println!("\n=== 三进制 GF(9) 幻方与正交拉丁矩阵 ===");
    ternary_magic::verify_all();

    println!("\n=== GF(4) 仿射公式 → 正交拉丁方 → 幻方 ===");
    gf4_magic::verify_all();

    println!("\n=== 2·A₄ 推导特征标不可约性 ⟨χ,χ⟩=1 ===");
    chi_inner::verify_all();

    println!("\n=== GF(9) 幻方链推广: a≠b 判据特征 3 形态 ===");
    gf9_diag::verify_all();

    println!("\n=== 电磁学频率标定 + 范数坍缩验证 ===");
    em_frequency_validation::verify_all();

    println!("\n=== 电磁学离散作用量原理验证 ===");
    em_discrete_action_validation::verify_all();
}
