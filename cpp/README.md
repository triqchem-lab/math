# cpp — sov-math 的 C++23 实现

纯整数域数学库（零浮点运行时），对外导出 **C ABI**（`libsov_ffi`）。
契约头在 [`../c_abi/include/sov/`](../c_abi/include/sov/)，与 rust / hs 实现逐符号一致。

---

## 一、构建

### 推荐：用 CMakePresets（与 CI 同一条命令）

```bash
# Windows（MinGW-w64 GCC / Clang）
cmake --preset windows-mingw
cmake --build --preset windows-mingw
ctest --preset windows-mingw

# Linux
cmake --preset linux-gcc
cmake --preset linux-clang

# 生产构建（安全硬化）
cmake --preset linux-gcc-hardened      # Linux: -fhardened
cmake --preset windows-mingw-hardened  # Windows: 逐项回退标志
```

Presets 定义在 [`../CMakePresets.json`](../CMakePresets.json)，按 `hostSystemName` 自动过滤——
在 Windows 上 `cmake --list-presets` 只显示 Windows 相关预设。

### 独立构建（只编 cpp 子项目）

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

产物：`libsov_ffi.{dll,so,dylib}` + `libsov_ffi_static.a`。

---

## 二、安全治理

### 2.1 编译硬化（已落地）

开关在 [`../cmake/Hardening.cmake`](../cmake/Hardening.cmake)，默认 **OFF**，生产构建加 `-DSOV_HARDENED=ON`。

| 编译器 | 实际注入的标志 |
|---|---|
| **GCC 15+ / Linux** | `-fhardened`（一键：FORTIFY3 + stack-protector-strong + stack-clash-protection + trivial-auto-var-init=zero + PIE + RELRO + cf-protection） |
| **Windows / MinGW** | 逐项回退：`-D_FORTIFY_SOURCE=3`、`-fstack-protector-strong`、`-fstack-clash-protection`、`-ftrivial-auto-var-init=zero`、`-fcf-protection=full`（**MinGW 不支持 `-fhardened`**，且它只给 warning 不给 error，探测不可靠，故按平台判定） |
| **MSVC** | `/GS /guard:cf /sdl` + 链接 `/DYNAMICBASE /NXCOMPAT` |

> ⚠ 实测记录：`-fhardened` 依赖 ELF 特性（PIE/RELRO/`-z now`），MinGW 上输出
> `cc1.exe: warning: '-fhardened' not supported for this target`——**警告不是错误**，
> 所以 `check_c_compiler_flag` 会误判为支持。本仓库按 `WIN32` 直接走回退分支。

### 2.2 C++23 安全特性（建议用法）

| 特性 | 用在哪 | 收益 |
|---|---|---|
| **`std::expected`** | 解析/校验类接口的返回值 | 错误进类型系统，调用方无法忽略；不穿越 ABI 边界 |
| **`std::print` / `std::format`** | 日志、诊断输出 | 格式串编译期检查，杜绝 `%s`/`%d` 不匹配（需 `-lstdc++exp`） |
| **`std::span`** | 代替 `(ptr, len)` 参数对 | 长度随指针，杜绝 C 风格的长度/指针脱节 |
| **`std::ranges`（`zip`/`chunk`/`views::filter`）** | 序列遍历 | 消除手写下标循环（越界与迭代器失效高发区） |
| **`std::stacktrace`** | 崩溃/异常路径 | 跨平台可移植栈回溯，不依赖 DBGHELP/backtrace |

### 2.3 面向未来：C++26 Profiles / Contracts

- **Profiles（P3081，目标 C++26）**：`-fprofile=bounds_safety` 等编译期强制规则。
  当前架构已为其预留空间：**业务逻辑不散落手动 `new`/`delete`/`malloc`，资源生命周期收敛到 RAII**，
  将来启用 `lifetime_safety` 时迁移成本最低。
- **Contracts（GCC 16 实验性）**：前置/后置条件进类型系统，替代文档约定。

---

## 三、跨平台

### 3.1 薄抽象层（PAL）

平台差异**只在 `c_abi/src/platform/` 落地**，业务代码里不出现 `#ifdef _WIN32`：

```
c_abi/src/platform/
├── dl.h            # 接口：sov_dlopen / sov_dlsym / sov_dlclose / sov_dlerror
├── dl_posix.c      # Linux / macOS / BSD：dlopen + -ldl
└── dl_win32.c      # Windows：LoadLibraryA / GetProcAddress / FreeLibrary
```

CMake 按 `if(WIN32)` 选择编译哪个 `.c`。新增平台相关能力时照此模式扩展（一个头 + N 个实现）。

### 3.2 工具链矩阵（实测）

| 平台 | 推荐编译器 | 实测状态 | 备注 |
|---|---|---|---|
| **Windows** | **MSYS2 UCRT64 的 GCC 16.2.0** | ✅ 17/17 | `-march=native` 解析为 skylake |
| Windows（备选） | Clang 23.1.4（MSYS2） | ✅ 17/17 | 与 GCC 共用 libstdc++ |
| **Windows（不推荐）** | winlibs 的 GCC 15/16 | ❌ SIGSEGV | **AVX2 栈对齐缺陷**：寄存器 spill 生成 `vmovdqa` 但不做栈重对齐；`--disable-bootstrap` 构建 |
| **Linux** | GCC 15+ / Clang | 待 CI 验证 | `-fhardened` 可用 |
| **macOS** | Apple Clang（Xcode CLT） | 待 CI 验证 | **唯一合规路径**，见 [`../cmake/toolchain-macos.cmake`](../cmake/toolchain-macos.cmake) |

> winlibs 与 MSYS2 同版本 16.2.0 表现相反，原因是构建配置：MSYS2 用 `--enable-bootstrap` +
> `--with-arch=nocona`，winlibs 用 `--disable-bootstrap` 且不指定基线。
> 这不是 CPU 差异——`-march=haswell`（E5 v3 同代）在 winlibs 上同样崩。

### 3.3 交叉编译

Linux 主机 → Windows 产物：

```bash
sudo apt install g++-mingw-w64-x86-64
cmake --preset cross-windows-from-linux
cmake --build --preset cross-windows-from-linux
```

Toolchain 文件：[`../cmake/toolchain-windows-mingw.cmake`](../cmake/toolchain-windows-mingw.cmake)。
产物是原生 PE，不依赖模拟层；分发建议静态链接或随包附带 `libgcc_s_seh-1.dll` / `libstdc++-6.dll` / `libwinpthread-1.dll`。

---

## 四、性能优化

### 4.1 已启用

| 项 | 配置 | 位置 |
|---|---|---|
| 优化等级 | `-O3` | CMakeLists 顶部 |
| 指令集 | `-mavx2 -march=native` | 同上（MSVC 为 `/arch:AVX2 /O2`） |
| 手写向量化 | VAVX3 的 198 处 `_mm256_*` intrinsics | `vavx3/` |

### 4.2 建议启用（未默认开，按需）

```bash
# LTO：跨 TU 内联与去虚拟化
cmake -B build -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# PGO：用真实负载的 profile 指导分支布局
cmake -B build-pgo -DCMAKE_CXX_FLAGS="-fprofile-generate"
# ... 跑基准 ...
cmake -B build-pgo2 -DCMAKE_CXX_FLAGS="-fprofile-use"
```

### 4.3 已知陷阱：AVX2 栈对齐

Windows x64 ABI 只保证 **16 字节**栈对齐，而 AVX2 的 `vmovdqa` 要求 **32 字节**。
编译器必须自己插入 `and $-32, %rsp`；GCC 在某些构建上没做对（见 §3.2）。

- 症状：`-O2`/`-O3` 正常，加 `-mavx2` 必崩；崩在 `vmovdqa ...(%rsp)` 且地址非 32 的倍数
- **不要**用的错误方案：`-mstackrealign`（只对齐到 16）、`-fno-tree-vectorize`（问题在寄存器分配，不在向量化）、
  给函数加 `alignas(32)` 局部变量（GCC 不理会）
- **正确方案**：换 MSYS2 官方构建的 GCC，或用 Clang

---

## 五、数值严谨性

所有编译期常量表（如 `rsqrt` Q16 LUT）以 **mpmath 130 位十进制**为金标准，
用 `hs/sov-math/engineering/gen_wide_consts.py` 生成，舍入口径 `int(x + 0.5)`。

误差界验证（`gf3_types.h` 注释有完整论证）：

| 项 | 数值 |
|---|---|
| 覆盖维度 | DIM ∈ {24, 144, 1200, 1440}，共 2812 个 LUT 元素 |
| 与 130 位金标准差异 | **0** |
| 最小舍入间隙 | 1.058e-4（DIM=1440, m=805） |
| double 误差上界 | 2.33e-9 |
| 余量 | **45382 倍** ⇒ 舍入方向不可能被 double 误差翻转 |

详见 [`../docs/abi.md`](../docs/abi.md) §〇之二。

---

## 六、目录

```
cpp/
├── CMakeLists.txt          构建定义（sov_math INTERFACE + sov_ffi SHARED/STATIC）
├── include/                内部数学头（不对外）
│   ├── gf3_types.h         编译期常量表（含误差界论证）
│   ├── complex_2a4.h       A₄ 表示论
│   └── ...
├── src/sov_ffi_cpp.cpp     C ABI 导出实现（extern "C"）
├── tests/                  17 个测试（RED/AMBER/GREEN 分级）
└── vavx3/                  VAVX3 虚拟 ISA（手写 AVX2，256 位拼 512 位）
```

契约头与一致性测试在 [`../c_abi/`](../c_abi/)；Rust 实现在 [`../rust/sov-math/`](../rust/sov-math/)；
Haskell 实现在 [`../hs/sov-math/`](../hs/sov-math/)。
