-- | Sov.Math.Block — SOV v2.6 存储块与范畴标签 (镜像 rust types.rs 剩余部分)
--
-- 强类型化: LayerTag 是 4 构造子枚举 (rust 是裸 u32 常量);
-- SovBlock128 的 qs 槽位是 6 个 Trit (语义上就是 trit, 不再是裸 u8)。
-- 定点陈数/相位字段全部整型, 零浮点。
module Sov.Math.Block
  ( LayerTag(..)
  , layerTagValue
  , SovBlock128(..)
  , sovBlockZero
  , SovHolographicHeader(..)
  , headerZero
  ) where

import Data.Int    (Int32)
import Data.Word   (Word8, Word16, Word32)
import Sov.Math.Adc
import Sov.Math.Trit
import Sov.Math.Nat
import Sov.Math.Vec

-- | SOV v2.6 范畴标签 — 4 构造子枚举, 编译期封闭
data LayerTag
  = TagL1Gf3Tensor    -- 0x10000001
  | TagL2Z3RRing      -- 0x20000002
  | TagL3ChiralState  -- 0x30000003
  | TagTopologyGrid   -- 0x40000004
  deriving (Eq, Ord, Show, Enum, Bounded)

layerTagValue :: LayerTag -> Word32
layerTagValue TagL1Gf3Tensor   = 0x10000001
layerTagValue TagL2Z3RRing     = 0x20000002
layerTagValue TagL3ChiralState = 0x30000003
layerTagValue TagTopologyGrid  = 0x40000004

-- | 128 位主权块 (16 字节对齐): qs = 6 trit 槽位 (类型层保证 {0,1,2})
data SovBlock128 = SovBlock128
  { sbQs         :: Vec N6 Trit   -- 6 个量化 trit 槽位
  , sbScaleUE8M0 :: !Word8        -- UE8M0 指数标度
  , sbPhaseBias  :: !Word8        -- 相位偏置
  , sbChernGuard :: !Word8        -- 陈数守卫字节
  , sbWuxingMask :: !Word8        -- 五行掩码
  , sbReserved   :: Vec N6 Word8  -- 预留
  } deriving (Eq, Show)

sovBlockZero :: SovBlock128
sovBlockZero = SovBlock128 (replicateV n6 T0) 0 0 0 0 (replicateV n6 0)

-- | SOV v2.6 全息头 (72 字节语义布局)
data SovHolographicHeader = SovHolographicHeader
  { hhMagic           :: !Word32
  , hhVersion         :: !Word32
  , hhGrandPumpStep   :: !Word32
  , hhChernNumberQ16  :: !Int32    -- 定点陈数 (整型, 禁浮点)
  , hhC3SolitonPhase  :: !Word16
  , hhZhonglvClosure  :: !Word32
  , hhPaddingAlign    :: !Word16
  , hhAccumulatorLimbs :: Uint384  -- 6 × 64 位 LCM 累加器快照
  } deriving (Eq, Show)

headerZero :: SovHolographicHeader
headerZero = SovHolographicHeader 0 0 0 0 0 0 0 (replicateV n6 0)
