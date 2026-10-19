-- | Sov.Math.Error — 两层错误体系 (镜像 rust error.rs)
--
--   可恢复   → Either MathError _
--   不可恢复 → constitutionalPanic (对齐 constitutional_panic!)
--
-- 用 GADT 携带上下文证据: ChernNotLocked 记录当前 Q16 值,
-- DimensionMismatch 记录期望/实际维度。
module Sov.Math.Error
  ( MathError(..)
  , renderError
  , constitutionalPanic
  ) where

-- | 主权数学错误 — 可恢复
data MathError where
  -- | GF(3) 矩阵乘法维度不匹配
  DimensionMismatch  :: { expectedDim :: Int, actualDim :: Int } -> MathError
  -- | LCM 桥陈数未锁定, 逆向桥被拒绝
  ChernNotLocked     :: { currentQ16 :: Int } -> MathError
  -- | SOV 文件格式错误
  InvalidFormat      :: { detail :: String } -> MathError
  -- | I/O 错误 (包装 IO 异常信息)
  IoError            :: String -> MathError

instance Show MathError where
  show = renderError

renderError :: MathError -> String
renderError (DimensionMismatch e a) =
  "🔴 维度不匹配: expected=" ++ show e ++ ", actual=" ++ show a
renderError (ChernNotLocked q)      =
  "🔴 陈数未锁定: C_q16=" ++ show q ++ ", 拒绝逆向桥"
renderError (InvalidFormat d)       = "🔴 SOV 格式错误: " ++ d
renderError (IoError e)             = "🔴 I/O 错误: " ++ e

-- | 宪法级断言 — 违反时直接 error (不可恢复, 对齐 constitutional_panic!)
constitutionalPanic :: String -> a
constitutionalPanic msg = error ("🔴 宪法违宪: " ++ msg)
