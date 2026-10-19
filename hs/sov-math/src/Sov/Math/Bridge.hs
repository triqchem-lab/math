-- | Sov.Math.Bridge — LCM 桥状态机 (镜像 rust bridge.rs / C++ lcm_bridge.h)
--
-- 层1↔层2 唯一合法通道。状态机:
--   L1Ready → forwardBridge → L2Computing
--   L2Computing → reverseBridge (陈数锁定时) → L1Ready
-- 陈数守卫: C_q16 ∈ [CHERN_TARGET_Q16 ± 655] 才允许逆向桥。
--
-- 纯函数式: 返回 (新状态, 结果 Either MathError)。
module Sov.Math.Bridge
  ( BridgeState(..)
  , LcmBridge(..)
  , newBridge
  , forwardBridge
  , reverseBridge
  , microPump
  , midPump
  , grandPump
  , chernGuardOk
  , updateChern
  , bridgeStep
  , c3SolitonPhase
  , zhonglvClosureCount
  ) where

import Data.Word    (Word64, Word16)
import Sov.Math.Adc
import qualified Sov.Math.Constants as C
import Sov.Math.Error
import Sov.Math.Trit

data BridgeState = L1Ready | L2Computing | L2Done | ChernLocked
  deriving (Eq, Show)

data LcmBridge = LcmBridge
  { bridgeAcc     :: LcmAccumulator
  , bridgeState   :: BridgeState
  , bridgeChernQ16 :: Int
  } deriving (Eq, Show)

newBridge :: LcmBridge
newBridge = LcmBridge newAcc L1Ready 0

-- 正向桥: L1 → L2 (仲吕闭合)
forwardBridge :: LcmBridge -> LcmBridge
forwardBridge b = b { bridgeAcc = zhonglvClosure (bridgeAcc b)
                    , bridgeState = L2Computing
                    }

-- 逆向桥: L2 → L1 (陈数未锁定 → mid_pump + 拒绝)
reverseBridge :: LcmBridge -> [Trit] -> (LcmBridge, Either MathError [Trit])
reverseBridge b z3r
  | chernGuardOk b = (b { bridgeState = L1Ready }, Right z3r)
  | otherwise      =
      let b' = midPump b
      in (b', Left (ChernNotLocked (bridgeChernQ16 b)))

microPump :: LcmBridge -> LcmBridge
microPump b = b { bridgeAcc = zhonglvClosure (bridgeAcc b)
                , bridgeState = L1Ready
                }

midPump :: LcmBridge -> LcmBridge
midPump b = b { bridgeAcc = newAcc, bridgeState = L1Ready }

grandPump :: LcmBridge -> LcmBridge
grandPump b = (midPump b) { bridgeChernQ16 = 0 }

-- 陈数守卫: |C_q16 − 2·65536| ≤ 655
chernGuardOk :: LcmBridge -> Bool
chernGuardOk b =
  let c = fromIntegral (bridgeChernQ16 b) :: Int
      target = fromIntegral C.chernTargetQ16 :: Int
      tol    = fromIntegral C.chernGuardTolerance :: Int
  in c >= target - tol && c <= target + tol

updateChern :: Int -> LcmBridge -> LcmBridge
updateChern q16 b =
  let b' = b { bridgeChernQ16 = q16 }
  in if chernGuardOk b' then b' { bridgeState = ChernLocked } else b'

-- 步进 + 微泵周期 (step_count 达 12 触发 micro_pump)
bridgeStep :: Word64 -> LcmBridge -> LcmBridge
bridgeStep delta b =
  let acc' = accStep delta (bridgeAcc b)
      b'   = b { bridgeAcc = acc' }
  in if accStepCount acc' >= C.microPump then microPump b' else b'

-- C3 孤子相位
c3SolitonPhase :: LcmBridge -> Word16
c3SolitonPhase b = fromIntegral (accStepCount (bridgeAcc b) `mod` C.c3CycleSteps)

-- 仲吕闭合计数
zhonglvClosureCount :: LcmBridge -> Word64
zhonglvClosureCount b =
  fromIntegral (accStepCount (bridgeAcc b) `div` C.zhonglvPeriod)
