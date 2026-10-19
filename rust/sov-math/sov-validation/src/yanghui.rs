// 杨辉三角 (Pascal) 模 p, Lucas 定理

fn modpow(mut b: u64, mut e: u64, m: u64) -> u64 {
    let mut r = 1u64;
    b %= m;
    while e > 0 {
        if e & 1 == 1 {
            r = r * b % m;
        }
        b = b * b % m;
        e >>= 1;
    }
    r
}

// C(n,k) mod p, n < p (p 素数), 乘法公式 + Fermat 逆
fn comb_small(n: u64, k: u64, p: u64) -> u64 {
    if k > n {
        return 0;
    }
    if k == 0 || k == n {
        return 1;
    }
    let k = k.min(n - k);
    let mut num = 1u64;
    let mut den = 1u64;
    for i in 1..=k {
        num = num * (n - k + i) % p;
        den = den * i % p;
    }
    num * modpow(den, p - 2, p) % p
}

// Lucas 定理: C(n,k) ≡ Π C(n_i, k_i) (mod p), n_i/k_i 为 p 进制位
fn binomial_mod_p(n: u64, k: u64, p: u64) -> u64 {
    if k == 0 {
        return 1;
    }
    let (nm, km) = (n % p, k % p);
    if km > nm {
        return 0;
    }
    comb_small(nm, km, p) * binomial_mod_p(n / p, k / p, p) % p
}

pub fn verify_all() {
    // 杨辉三角模 3 前 8 行
    println!("杨辉三角 C(n,*) mod 3:");
    for n in 0..8u64 {
        let row: Vec<u64> = (0..=n).map(|k| binomial_mod_p(n, k, 3)).collect();
        println!("  n={}: {:?}", n, row);
    }

    // Lucas 定理实例 (修正: 原稿断言 C(5,2)=0、C(4,2)=1 均算错)
    // C(5,2)=10≡1; Lucas: 5=(12)₃, 2=(02)₃ → C(1,0)·C(2,2)=1·1=1
    assert_eq!(binomial_mod_p(5, 2, 3), 1, "C(5,2) ≡ 1 mod 3");
    // C(4,2)=6≡0; Lucas: 4=(11)₃, 2=(02)₃ → C(1,0)·C(1,2)=1·0=0
    assert_eq!(binomial_mod_p(4, 2, 3), 0, "C(4,2) ≡ 0 mod 3");
    // C(6,3)=20≡2; Lucas: 6=(20)₃, 3=(10)₃ → C(2,0)·C(0,1)·C(2,1)... 应为 2
    assert_eq!(binomial_mod_p(6, 3, 3), 2, "C(6,3) ≡ 2 mod 3");
    // 边界: C(n,0)=1, C(n,n)=1
    assert_eq!(binomial_mod_p(10, 0, 3), 1);
    assert_eq!(binomial_mod_p(10, 10, 3), 1);
    println!("杨辉三角模 3 Lucas 定理 ✅");
}
