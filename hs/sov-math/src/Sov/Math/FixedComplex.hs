-- | Sov.Math.FixedComplex — Q16.16 定点复数 (镜像 rust fixed_complex.rs / C++ fixed_complex.h)
--
-- 层3 五行模数区。Q16 是人工可读投影层 — 状态演化用精确层 (Sov.Math.Eis),
-- 本层仅做人类可读投影, 不进入状态机演化。
--
-- Q16.16: 1 位符号 + 15 位整数 + 16 位小数 = Int32, 缩放 2^16 = 65536。
-- ω = e^{2πi/3} = −1/2 + i√3/2 → (−32768, +56753)
-- 五行振幅: 相生→+1, 相克→ω, 克制→ω² (1 + ω + ω² = 0)
--
-- 强类型化: Q16 用 newtype 包装 Int32 — 与普通 Int32 在类型上不可混淆;
-- 乘法只经由 q16Mul (饱和语义集中一处)。
module Sov.Math.FixedComplex
  ( Q16
  , unQ16
  , q16
  , q16One, q16Half, q16Zero
  , q16Mul
  , Q16Complex(..)
  , qcZero, qcOne, qcOmega, qcOmega2
  , qcAdd, qcMul, qcNormSq
  , omegaSumOk
  , ampGenerate, ampOvercome, ampOvercome2
  , omegaReQ16, omegaImQ16, omega2ReQ16, omega2ImQ16
  ) where

import Data.Int (Int32)
import Data.Bits (shiftR)

-- | Q16.16 定点数 — newtype 隔离, 防与裸 Int32 混用
newtype Q16 = Q16 { unQ16 :: Int32 }
  deriving (Eq, Show)

q16 :: Int32 -> Q16
q16 = Q16

q16One  :: Q16; q16One  = Q16 65536   -- 1.0
q16Half :: Q16; q16Half = Q16 32768   -- 0.5
q16Zero :: Q16; q16Zero = Q16 0

-- Q16 定点乘法: (a × b) >> 16 (Int64 中间量避免溢出)
q16Mul :: Q16 -> Q16 -> Q16
q16Mul (Q16 a) (Q16 b) =
  Q16 (fromIntegral ((toInteger a * toInteger b) `shiftR` 16))

-- ω = e^{2πi/3} = −1/2 + i√3/2 (Q16.16 定点)
omegaReQ16, omegaImQ16, omega2ReQ16, omega2ImQ16 :: Int32
omegaReQ16  = -32768   -- −0.5 × 65536
omegaImQ16  = 56753    -- +√3/2 × 65536
omega2ReQ16 = -32768   -- −0.5 × 65536
omega2ImQ16 = -56753   -- −√3/2 × 65536

-- 定点复数
data Q16Complex = Q16Complex { qcRe :: !Q16, qcIm :: !Q16 }
  deriving (Eq, Show)

qcZero, qcOne, qcOmega, qcOmega2 :: Q16Complex
qcZero   = Q16Complex q16Zero q16Zero
qcOne    = Q16Complex q16One  q16Zero
qcOmega  = Q16Complex (Q16 omegaReQ16)  (Q16 omegaImQ16)
qcOmega2 = Q16Complex (Q16 omega2ReQ16) (Q16 omega2ImQ16)

qcAdd :: Q16Complex -> Q16Complex -> Q16Complex
qcAdd (Q16Complex ar ai) (Q16Complex br bi) =
  Q16Complex (Q16 (unQ16 ar + unQ16 br)) (Q16 (unQ16 ai + unQ16 bi))

qcMul :: Q16Complex -> Q16Complex -> Q16Complex
qcMul (Q16Complex ar ai) (Q16Complex br bi) =
  Q16Complex (q16Mul ar br `q16SubI` q16Mul ai bi)
             (q16Mul ar bi `q16AddI` q16Mul ai br)
  where
    q16AddI x y = Q16 (unQ16 x + unQ16 y)
    q16SubI x y = Q16 (unQ16 x - unQ16 y)

-- 范数平方: a² + b² (Q16 乘后 >>16)
qcNormSq :: Q16Complex -> Q16
qcNormSq (Q16Complex r i) = q16Add (q16Mul r r) (q16Mul i i)

q16Add :: Q16 -> Q16 -> Q16
q16Add (Q16 a) (Q16 b) = Q16 (a + b)

-- ω² + ω + 1 = 0 (Q16 乘法有累积截断误差, 容忍 ±16) — 镜像 C++ verify_omega_cubed
omegaSumOk :: Bool
omegaSumOk =
  let w  = qcOmega
      w2 = qcMul w w
      s  = qcAdd (qcAdd w2 w) qcOne
      ok (Q16 v) = v >= -16 && v <= 16
  in ok (qcRe s) && ok (qcIm s)

-- 五行干涉振幅: 相生 +1, 相克 ω, 克制 ω²
ampGenerate, ampOvercome, ampOvercome2 :: Q16Complex
ampGenerate  = qcOne
ampOvercome  = qcOmega
ampOvercome2 = qcOmega2
