-- | Sov.Math.Complex2A4 — 2A₄ 的 2 维**复**表示：显式 Z[ω] 矩阵，χ = tr
--
-- 动机（评审 T2）：`Rep2A4` 原先按元素阶"读数"取旋量特征标，属**断言**（值对、理由不对）。
-- 本模块给出显式矩阵模型，让 χ 真正由**迹**算出。
--
-- 生成元（由计算搜索得到，最小系数；闭包实测 24 元）：
--
--   i = [[ −ω, −1 ], [ −ω,  ω ]]      阶 4，i² = −I
--   j = [[  0,  ω ], [ 1+ω, 0 ]]      阶 4，j² = −I，ij = −ji
--   U = [[  0, −1 ], [  1, −1 ]]      阶 3，U·i·U⁻¹ = j
--
-- ⟨i, j, U⟩ ⊂ GL(2, Z[ω])，阶 24，阶分布 {1:1, 2:1, 4:6, 3:8, 6:8} = SL(2,3)。
-- 迹分布 {2:1, −2:1, 0:6, 1:8, −1:8} —— 正是旋量特征标的取值。
--
-- ⚠ 关键事实（本模块验证，`modelTracesBySizeOrder`）：
--   迹**只依赖 (类大小, 元素阶)**。这正是 `Rep2A4` 可以用 (size, order) 索引
--   取值的依据——此前那只是假设。
--
-- 与 `Cover2A4` 的关系：那里证的是 GF(3) 模型 → A₄ 的覆盖；这里证的是同一群在
--   Z[ω] 上的忠实 2 维复实现。两者合起来支撑"2A₄"这个名字。
--
-- 全枚举口径：闭包/类/阶/迹都是 24 元有限集上的全枚举，非归纳证明。
module Sov.Math.Complex2A4
  ( M2E (..)
  , mulM, idM, invM, detM, traceM, powM, orderOfM
  , iGen, jGen, uGen, group24
  , conjClassM, classesM, classInfoM
  , charBySizeOrder, spinorCharFromModel
  , modelOrders, modelTraces, traceDependsOnlyOnSizeOrder
  , mIndex, oneDimChar, tensorRep, tensorFaithful, tensorHom
  , tensorRepsFaithful, tensorRepsHom
  , modelOk
  ) where

import Data.List (nub, sort)

import Sov.Math.Eis
import Sov.Math.Error (constitutionalPanic)

-- | Z[ω] 上的 2×2 矩阵
data M2E = M2E Eis Eis Eis Eis deriving (Eq, Show)

mulM :: M2E -> M2E -> M2E
mulM (M2E a b c d) (M2E e f g h) =
  M2E (addE (mulE a e) (mulE b g))
      (addE (mulE a f) (mulE b h))
      (addE (mulE c e) (mulE d g))
      (addE (mulE c f) (mulE d h))

idM :: M2E
idM = M2E eisOne eisZero eisZero eisOne

detM :: M2E -> Eis
detM (M2E a b c d) = addE (mulE a d) (negE (mulE b c))

-- | det = 1 时的逆
invM :: M2E -> M2E
invM (M2E a b c d) = M2E d (negE b) (negE c) a

traceM :: M2E -> Eis
traceM (M2E a _ _ d) = addE a d

powM :: Int -> M2E -> M2E
powM n m
  | n <= 0    = idM
  | otherwise = mulM m (powM (n - 1) m)

--------------------------------------------------------------------------------
-- 生成元（计算搜索选出，系数 ∈ {−1,0,1}）
--------------------------------------------------------------------------------

iGen, jGen, uGen :: M2E
iGen = M2E (Eis 0 (-1)) (Eis (-1) 0) (Eis 0 (-1)) (Eis 0 1)   -- [[−ω,−1],[−ω,ω]]
jGen = M2E eisZero (Eis 0 1) (Eis 1 1) eisZero                  -- [[0,ω],[1+ω,0]]
uGen = M2E eisZero (Eis (-1) 0) (Eis 1 0) (Eis (-1) 0)          -- [[0,−1],[1,−1]]

generators :: [M2E]
generators = [iGen, jGen, uGen]

-- | 生成闭包（有限 ⇒ 单侧 + 逆闭包即群）
group24 :: [M2E]
group24 = go [idM] [idM]
  where
    gens = generators ++ map invM generators
    go acc []       = acc
    go acc (x : xs) =
      let new = [ y | g <- gens, let y = mulM g x, y `notElem` acc ]
      in go (acc ++ new) (xs ++ new)

--------------------------------------------------------------------------------
-- 类结构与特征标（迹）
--------------------------------------------------------------------------------

orderOfM :: M2E -> Int
orderOfM m = case [ k | k <- [1 .. 24], powM k m == idM ] of
    (k : _) -> k
    []      -> constitutionalPanic "Complex2A4: 元素阶超过 24（不可达）"

conjClassM :: M2E -> [M2E]
conjClassM m = nub [ mulM (mulM h m) (invM h) | h <- group24 ]

classesM :: [[M2E]]
classesM = go group24
  where
    go []       = []
    go (m : ms) = conjClassM m : go (filter (`notElem` conjClassM m) ms)

classInfoM :: [(Int, Int)]
classInfoM = sort [ (length c, classOrderOf c) | c <- classesM ]

classOrderOf :: [M2E] -> Int
classOrderOf (m : _) = orderOfM m
classOrderOf []      = 0

firstOfClass :: [M2E] -> M2E
firstOfClass (m : _) = m
firstOfClass []      = idM

modelOrders :: [Int]
modelOrders = sort (map orderOfM group24)

modelTraces :: [Eis]
modelTraces = map traceM group24   -- 未排序（Eis 无 Ord）

-- | 迹只依赖 (类大小, 元素阶) —— `Rep2A4` 按 (size, order) 索引取值的依据
traceDependsOnlyOnSizeOrder :: Bool
traceDependsOnlyOnSizeOrder =
  all (\(sz, od) ->
        let cs = [ traceM (firstOfClass c) | c <- classesM
                   , length c == sz, classOrderOf c == od ]
        in length (nub cs) <= 1)
      (nub classInfoM)

-- | 定义表示的特征标，按 (类大小, 元素阶) 索引
charBySizeOrder :: [(Int, Int, Eis)]
charBySizeOrder =
  [ (length c, classOrderOf c, traceM (firstOfClass c)) | c <- classesM ]

-- | 按 (类大小, 元素阶) 取特征标值（`Rep2A4` 用此构造旋量特征标）
spinorCharFromModel :: Int -> Int -> Eis
spinorCharFromModel sz od =
  case [ chi | (s, o, chi) <- charBySizeOrder, s == sz, o == od ] of
    (chi : _) -> chi
    []        -> constitutionalPanic "Complex2A4: (size,order) 无对应类（不可达）"

--------------------------------------------------------------------------------
-- 三个 2 维表示 = 定义表示 ⊗ 三个 1 维表示（矩阵级，不只是字符级）
--
-- 1 维表示经 2A₄ → A₄ → C₃ 分解，由 U 的像决定：m(i)=m(j)=0, m(U)=1，
-- 再沿乘法一致化（BFS；不一致即说明假设有误，`mIndex` 会返回空）。
-- λ_k(g) = ω^{k·m(g)}，张量表示 = λ_k(g)·M(g)（标量乘矩阵）。
--------------------------------------------------------------------------------

-- | 1 维表示的指数 m : G → {0,1,2}（BFS 一致化；不一致则缺失）
mIndex :: [(M2E, Int)]
mIndex = go [(idM, 0)] [(idM, 0)]
  where
    genExp = [ (iGen, 0), (jGen, 0), (uGen, 1) ]
    go acc [] = acc
    go acc ((x, mx) : rest) =
      let news =
            [ (mulM g x, (d + mx) `mod` 3)
            | (g, d) <- genExp
            , all (\(y, _) -> y /= mulM g x) acc ]        -- 只看新元素
          newNeg =
            [ (mulM (invM g) x, ((3 - d) + mx) `mod` 3)
            | (g, d) <- genExp
            , all (\(y, _) -> y /= mulM (invM g) x) acc ]
          fresh = news ++ newNeg
      in go (acc ++ fresh) (rest ++ fresh)

-- | λ_k(g) = ω^{k·m(g)}；不在表中（不可达）返回 1
oneDimChar :: Int -> M2E -> Eis
oneDimChar k g =
  case [ m | (y, m) <- mIndex, y == g ] of
    (m : _) -> powEis ((k * m) `mod` 3)
    []      -> constitutionalPanic "Complex2A4: 元素不在 mIndex 中（不可达）"

powEis :: Int -> Eis
powEis 0 = eisOne
powEis 1 = eisOmega
powEis _ = eisOmega2

scaleM :: Eis -> M2E -> M2E
scaleM s (M2E a b c d) = M2E (mulE s a) (mulE s b) (mulE s c) (mulE s d)

-- | 第 k 个 2 维表示（k = 0 即定义表示本身）：g ↦ λ_k(g)·M(g)
tensorRep :: Int -> M2E -> M2E
tensorRep k g = scaleM (oneDimChar k g) g

tensorFaithful :: Int -> Bool
tensorFaithful k = length (nub [ tensorRep k g | g <- group24 ]) == 24

tensorHom :: Int -> Bool
tensorHom k = all (\(g, h) -> tensorRep k (mulM g h)
                              == mulM (tensorRep k g) (tensorRep k h)) ghPairs
  where ghPairs = [ (g, h) | g <- group24, h <- group24 ]

tensorRepsFaithful :: Bool
tensorRepsFaithful = all tensorFaithful [0, 1, 2]

tensorRepsHom :: Bool
tensorRepsHom = all tensorHom [0, 1, 2]

--------------------------------------------------------------------------------
-- 自检
--------------------------------------------------------------------------------

modelOk :: Bool
modelOk =
  length group24 == 24
  && length (nub group24) == 24
  && all (\m -> detM m == eisOne) group24
  && powM 2 iGen == M2E (Eis (-1) 0) eisZero eisZero (Eis (-1) 0)   -- i² = −I
  && powM 2 jGen == M2E (Eis (-1) 0) eisZero eisZero (Eis (-1) 0)   -- j² = −I
  && powM 3 uGen == idM                                             -- U 阶 3
  && mulM iGen jGen == mulM (M2E (Eis (-1) 0) eisZero eisZero (Eis (-1) 0)) (mulM jGen iGen)
                                                                    -- ij = −ji
  && mulM (mulM uGen iGen) (invM uGen) == jGen                      -- U i U⁻¹ = j
  && modelOrders == [1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4
                    , 6, 6, 6, 6, 6, 6, 6, 6]
  && traceDependsOnlyOnSizeOrder
  && length (nub (map fst mIndex)) == 24   -- mIndex 覆盖全部 24 元（不只是条数）
  && length mIndex == 24          -- 1 维特征指数在 24 元上一致
  && tensorRepsFaithful           -- 三个 2 维表示（矩阵级）都忠实
  && tensorRepsHom                -- 三个都是同态
