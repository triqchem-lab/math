{-# OPTIONS --cubical --rewriting --guardedness #-}
-- | SpecAnchor — 跨语言实现的 Agda 规范锚 (等级 3 等价)
--
-- 目的: 把 cpp/rust/hs 三语言对账表 (dump_tables 的 DCCRT/DCMIX/GF3* 等节)
-- 的**每一个值**在 discrete-mathematics 规范层用 refl 判定一遍。
-- 链路: 规范 (本模块, 内核检查) ≡ 三语言实现 (dump 逐行一致, oracle 回执在册)
--       ⇒ 端到端锚定到 Agda 内核。
--
-- 本体论: 只锚定**投影层数值** (Z/12 是 DC 的投影); DC 本源事实
-- (mixedOp-12-cycle / dcZeroOblivion) 由库内既有证明承载, 本模块引用而非重证。
module SpecAnchor where

open import Sovereign.Base.Trit
open import Sovereign.Algebra.GroupTheory.DuodecClock
open import Sovereign.Algebra.GroupTheory.Duodecimal
open import Sovereign.Foundation.Eisenstein
open import Data.Nat using (ℕ; zero; suc)
open import Data.Product using (_×_; _,_)
open import Data.Integer using (ℤ; +_)
open import Data.Fin using (Fin; zero; suc)
open import Relation.Binary.PropositionalEquality using (_≡_; refl)

--------------------------------------------------------------------------------
-- §1. GF(3) 运算表 — 9 加法 + 9 乘法 (镜像 dump GF3ADD/GF3MUL 18 项)
--------------------------------------------------------------------------------

-- 加法表 9 项 (镜像 dump GF3ADD)
add-00 : T₀ ⊕ T₀ ≡ T₀
add-00 = refl
add-01 : T₀ ⊕ T₁ ≡ T₁
add-01 = refl
add-02 : T₀ ⊕ T₂ ≡ T₂
add-02 = refl
add-10 : T₁ ⊕ T₀ ≡ T₁
add-10 = refl
add-11 : T₁ ⊕ T₁ ≡ T₂
add-11 = refl
add-12 : T₁ ⊕ T₂ ≡ T₀
add-12 = refl
add-20 : T₂ ⊕ T₀ ≡ T₂
add-20 = refl
add-21 : T₂ ⊕ T₁ ≡ T₀
add-21 = refl
add-22 : T₂ ⊕ T₂ ≡ T₁
add-22 = refl

-- 乘法表 9 项 (镜像 dump GF3MUL)
mul-00 : T₀ ⊗ T₀ ≡ T₀
mul-00 = refl
mul-01 : T₀ ⊗ T₁ ≡ T₀
mul-01 = refl
mul-02 : T₀ ⊗ T₂ ≡ T₀
mul-02 = refl
mul-10 : T₁ ⊗ T₀ ≡ T₀
mul-10 = refl
mul-11 : T₁ ⊗ T₁ ≡ T₁
mul-11 = refl
mul-12 : T₁ ⊗ T₂ ≡ T₂
mul-12 = refl
mul-20 : T₂ ⊗ T₀ ≡ T₀
mul-20 = refl
mul-21 : T₂ ⊗ T₁ ≡ T₂
mul-21 = refl
mul-22 : T₂ ⊗ T₂ ≡ T₁
mul-22 = refl        -- 宪法: T₂⊗T₂ = T₁

-- char-3 归零: (t ⊕ t) ⊕ t = T₀ (零冥族 addZero 分量, 镜像 hs dcChar3)
anchor-char3 : ∀ (t : Trit) → (t ⊕ t) ⊕ t ≡ T₀
anchor-char3 T₀ = refl
anchor-char3 T₁ = refl
anchor-char3 T₂ = refl

--------------------------------------------------------------------------------
-- §2. C4 相位: α⁴ = a0 (相位归零, 镜像 mulAlphaCycle)
--------------------------------------------------------------------------------

anchor-alpha4 : mulAlpha (mulAlpha (mulAlpha a1 a1) a1) a1 ≡ a0
anchor-alpha4 = refl

anchor-alpha2 : mulAlpha a1 a1 ≡ a2          -- α² = 180°
anchor-alpha2 = refl

anchor-alpha3 : mulAlpha a2 a1 ≡ a3          -- α³ = 270°
anchor-alpha3 = refl

--------------------------------------------------------------------------------
-- §3. EC 单位元与 mixedOp 锚 (镜像 hs/rust/cpp dc_zero_oblivion)
--------------------------------------------------------------------------------

anchor-e : duodec-e ≡ (T₀ , a0)
anchor-e = refl

anchor-mixedop-zeroR : ∀ (x : Trit) (a : AlphaPower)
                     → mixedOp (x , a) duodec-e ≡ (x , a)
anchor-mixedop-zeroR T₀ a0 = refl
anchor-mixedop-zeroR T₀ a1 = refl
anchor-mixedop-zeroR T₀ a2 = refl
anchor-mixedop-zeroR T₀ a3 = refl
anchor-mixedop-zeroR T₁ a0 = refl
anchor-mixedop-zeroR T₁ a1 = refl
anchor-mixedop-zeroR T₁ a2 = refl
anchor-mixedop-zeroR T₁ a3 = refl
anchor-mixedop-zeroR T₂ a0 = refl
anchor-mixedop-zeroR T₂ a1 = refl
anchor-mixedop-zeroR T₂ a2 = refl
anchor-mixedop-zeroR T₂ a3 = refl

-- 联合生成元 g = (T₁, a1) 的平移锚: mixedOp g (T₀,a₀) = (T₁,a₁)
anchor-gen : mixedOp (T₁ , a1) duodec-e ≡ (T₁ , a1)
anchor-gen = refl

-- 损益抵消: (T₁,a0) mixedOp (T₂,a0) = (T₀,a0) — 04-zero-oblivion §1.1 的加法归零
anchor-cancel : mixedOp (T₁ , a0) (T₂ , a0) ≡ duodec-e
anchor-cancel = refl

--------------------------------------------------------------------------------
-- §4. CRT 投影 12 点 (镜像 dump DCCRT 12 项; crt12 是规范的唯一事实源)
--------------------------------------------------------------------------------

-- 12 点全表: toℕ₁₂ (crt12 t k) 的规范值 (镜像 dump DCCRT 12 项)
crt-00 : toℕ₁₂ (crt12 T₀ zero) ≡ 0
crt-00 = refl
crt-01 : toℕ₁₂ (crt12 T₀ (suc zero)) ≡ 9
crt-01 = refl
crt-02 : toℕ₁₂ (crt12 T₀ (suc (suc zero))) ≡ 6
crt-02 = refl
crt-03 : toℕ₁₂ (crt12 T₀ (suc (suc (suc zero)))) ≡ 3
crt-03 = refl
crt-10 : toℕ₁₂ (crt12 T₁ zero) ≡ 4
crt-10 = refl
crt-11 : toℕ₁₂ (crt12 T₁ (suc zero)) ≡ 1
crt-11 = refl
crt-12 : toℕ₁₂ (crt12 T₁ (suc (suc zero))) ≡ 10
crt-12 = refl
crt-13 : toℕ₁₂ (crt12 T₁ (suc (suc (suc zero)))) ≡ 7
crt-13 = refl
crt-20 : toℕ₁₂ (crt12 T₂ zero) ≡ 8
crt-20 = refl
crt-21 : toℕ₁₂ (crt12 T₂ (suc zero)) ≡ 5
crt-21 = refl
crt-22 : toℕ₁₂ (crt12 T₂ (suc (suc zero))) ≡ 2
crt-22 = refl
crt-23 : toℕ₁₂ (crt12 T₂ (suc (suc (suc zero)))) ≡ 11
crt-23 = refl

-- 与三语言测试的 4 个锚完全对应 (e→0, g→1, (T₀,α)→9, (T₁,a₀)→4)
anchor-crt-e : toℕ₁₂ (crt12 T₀ zero) ≡ 0
anchor-crt-e = crt-00
anchor-crt-g : toℕ₁₂ (crt12 T₁ (suc zero)) ≡ 1
anchor-crt-g = crt-11
anchor-crt-a : toℕ₁₂ (crt12 T₀ (suc zero)) ≡ 9
anchor-crt-a = crt-01
anchor-crt-t : toℕ₁₂ (crt12 T₁ zero) ≡ 4
anchor-crt-t = crt-10

--------------------------------------------------------------------------------
-- §5. Eisenstein 单位群 (镜像 hs/rust 测试: ω³=1, ω·ω²=1, |ω|=1)
--------------------------------------------------------------------------------

anchor-omega3 : ωᵉ *ᵉ ωᵉ *ᵉ ωᵉ ≡ 1ᵉ
anchor-omega3 = ω³≡1

anchor-omega2 : ωᵉ *ᵉ ωᵉ ≡ ω²ᵉ
anchor-omega2 = ω²≡ω*ω

anchor-omega-inv : ωᵉ *ᵉ ω²ᵉ ≡ 1ᵉ
anchor-omega-inv = ω*ω²≡1

-- |ω| = 1 (范数, 镜像 hs Eis normE / rust Eis::norm) — normᵉ : Eisenstein → ℤ
anchor-omega-norm : normᵉ ωᵉ ≡ (+ 1)
anchor-omega-norm = refl
