-- | sov-validation — 验证组汇总可执行 (镜像 rust sov-validation/main.rs 输出面)
module Main (main) where

import qualified Sov.Validation.Zomega as ZW
import qualified Sov.Validation.ChiInner as CI
import qualified Sov.Validation.GF9Diag as G9
import qualified Sov.Validation.GF4Magic as G4
import qualified Sov.Validation.Magic as MG
import qualified Sov.Validation.EM as EM

main :: IO ()
main = do
  putStrLn "=== sov-validation (Haskell) ==="
  report "Z[ω,i] 恒等式+结合律 81³ 穷举" ZW.zwiAssocCheck81
  report "杨辉三角 Lucas 定理" (ZW.binomialModP 5 2 3 == 1 && ZW.binomialModP 4 2 3 == 0 && ZW.binomialModP 6 3 3 == 2)
  report "2·A₄=SL(2,3) 特征标不可约" CI.verifyChiIrreducible
  report "GF(9) 幻方链判据分裂" G9.verifyAll9
  report "GF(4) 九节核验" G4.verifyAll4
  report "幻方组 (3×3/9 阶/12 阶/144 阶/M₄)" MG.verifyAllMagic
  report "电磁学频率标定+范数坍缩" EM.verifyAllEMFreq
  report "电磁学离散作用量+Noether" EM.verifyAllEMAction
  where
    report name ok = putStrLn ((if ok then "✅ " else "🔴 ") ++ name)
