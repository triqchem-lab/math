# c_abi — sov-math 的 C ABI

本目录集中 sov-math 的 **C ABI** 全部内容：公共头、实现、一致性测试、示例与打包模板。
ABI 契约对三种后端实现（C++ / Rust / Haskell）逐符号一致——**同一份头文件可链接任意一种实现**。

## 目录结构

```
c_abi/
├── include/
│   ├── sov/                 公共头（30 个）
│   │   ├── sov.h            umbrella：全部 API
│   │   ├── sov_ffi.h        契约头：聚合所有设施
│   │   ├── version.h        ABI 版本 + 运行时生命周期
│   │   ├── types.h          基础类型（SOV_API / 状态码 / trit 等）
│   │   └── <facility>.h     单设施细分头（见下表）
│   └── sov_ffi.h            兼容转发头（旧写法 #include "sov_ffi.h" 仍可用）
├── src/
│   └── sov_ffi_cpp.cpp      C++ 实现的 C ABI 包装（导出 extern "C" 符号）
├── tests/
│   └── ffi_conformance.c    一致性测试：dlopen/LoadLibrary 加载实现，与 golden 快照比对
├── examples/
│   ├── hello_sov.c          C99 消费示例
│   └── hello_sov_c23.c      C23 消费示例（constexpr / nullptr / [[nodiscard]]）
├── packaging/
│   ├── sov-ffi.pc.in        pkg-config 模板
│   └── sov_ffi.map          GNU ld version script（仅非 Windows 生效）
└── CMakeLists.txt           由 ../cpp/CMakeLists.txt 通过 add_subdirectory 引入
```

## ABI 版本

| 项 | 值 |
|---|---|
| `SOV_ABI_MAJOR` | **1** |
| `SOV_ABI_MINOR` | **1** |
| `SOV_ABI_VERSION_STRING` | `"sov-math ABI 1.1"` |
| `sov_abi_version()` | `(major << 16) \| minor`，运行时查询 |
| `SOV_ABI_AT_LEAST(maj, min)` | 编译期版本断言宏 |
| 共享库 `VERSION` / `SOVERSION` | `1.0.0` / `1`（`libsov_ffi.so.1`） |

## 契约约定

1. **状态码返回**：所有函数返回 `int32_t`，`0` 表示成功，负值表示错误；错误文案用
   `sov_strerror(code)` 取（静态字符串，调用方**不要释放**）。
2. **零库内分配**：输出经调用方缓冲区回传，库内部不做堆分配。
3. **运行时生命周期**：`sov_runtime_init()` / `sov_runtime_fini()` 配对调用。
   C++ / Rust 实现是空操作；**Haskell 实现需要**（内部调 `hs_init` / `hs_exit`）。
   为了同一份示例代码可链接三种实现，`hello_sov.c` 用弱符号声明 `hs_init`。
4. **纯 C99 头**：`types.h` 强制要求 `__STDC_VERSION__ >= 199901L`；C23 特性（`constexpr`、
   `nullptr`、`[[nodiscard]]`）由 `SOV_HAS_C23` 探测，不用 `-std=c23` 也能编译 C99 消费代码。

## 设施一览

`include/sov/` 下的细分头（`sov_ffi.h` 全部聚合）：

| 类别 | 头文件 |
|---|---|
| GF(3) / 三值 | `trit.h`、`tryte.h`、`pack.h` |
| 域与环 | `gf9.h`、`ring.h`、`eis.h`（艾森斯坦整数） |
| 时钟 / 律学 | `dc.h`（DC 联合时钟）、`dayan.h`、`t6.h`、`digit.h`、`doz.h` |
| 定点 / 宽整数 | `wide.h`、`u384.h`、`width.h`、`adc.h` |
| 几何 / 结构 | `holo.h`、`soliton.h`、`nse.h`、`sunyi.h`、`chern.h` |
| A₄ 群与表示 | `a4.h`、`rep.h`、`carrier.h`、`hopf.h` |
| 桥接 | `bridge.h` |

## 构建

C ABI 由 `cpp/CMakeLists.txt` 通过 `add_subdirectory` 引入，**不单独 configure**：

```bash
cd ..            # 回到 math/cpp
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

产物（落在 `<build>/c_abi/`）：

| 平台 | 共享库 | 静态库 |
|---|---|---|
| Linux / macOS | `libsov_ffi.so` / `.dylib` | `libsov_ffi_static.a` |
| Windows (MinGW) | `libsov_ffi.dll` + `libsov_ffi.dll.a` | `libsov_ffi_static.a` |

## 使用

### 1. 直接编译

```bash
# C99
cc hello_sov.c -I include -L <build>/c_abi -lsov_ffi -o hello_sov

# C23
cc -std=c23 hello_sov_c23.c -I include -L <build>/c_abi -lsov_ffi -o hello_sov_c23
```

### 2. pkg-config（安装后）

```bash
cmake --install <build> --prefix /usr/local
# 然后
cc hello_sov.c $(pkg-config --cflags --libs sov-ffi) -o hello_sov
```

## 测试

```bash
cd <build> && ctest -R ffi_conformance --output-on-failure
```

`ffi_conformance` 通过 `dlopen` / `LoadLibraryA` 动态加载实现库，逐个符号与 golden 快照比对——
因此同一个测试二进制可以对 **cpp / rust / hs 三种实现的库**分别跑一遍：

```bash
./c_abi/ffi_conformance <path-to-any-implementation>/libsov_ffi.so
```

## 三种实现

| 实现 | 库文件 | 位置 |
|---|---|---|
| C++ | `libsov_ffi` | 本目录 `src/sov_ffi_cpp.cpp` |
| Rust | `libsov_ffi_rust` | `../../rust/` |
| Haskell | `libsov_ffi_hs` | `../../hs/` |

三者导出**同一组 `extern "C"` 符号**，头文件完全通用；Haskell 实现额外需要
`sov_runtime_init()`（弱符号 `hs_init` 已在示例中处理）。

## 平台注意

| 项 | 说明 |
|---|---|
| Windows `.dll` | CMake 的 `RUNTIME DESTINATION bin` 处理 `.dll`，导入库 `.dll.a` 落 `lib/` |
| `sov_ffi.map` | GNU ld version script，**仅非 Windows 生效**；MinGW ld 支持有限 |
| `dlopen` vs `LoadLibrary` | `tests/ffi_conformance.c` 内置两套封装，按 `_WIN32` 切换 |
| 编译器 | GCC ≥ 14（C++23 + `-mavx2`）或 Clang ≥ 20；MSYS2 / winlibs 均可，但**注意 winlibs 构建在 AVX2 下有栈对齐缺陷**，推荐 MSYS2 官方构建 |

---

## 工程基础设施（契约层专属）

### 一、平台抽象层（PAL）

契约层自身也要跨平台运行（`ffi_conformance` 需要动态加载实现库）：

```
src/platform/
├── dl.h            # 接口：sov_dlopen / sov_dlsym / sov_dlclose / sov_dlerror
├── dl_posix.c      # Linux / macOS：dlopen + -ldl
└── dl_win32.c      # Windows：LoadLibraryA / GetProcAddress / FreeLibrary
```

`tests/ffi_conformance.c` **不出现任何 `#ifdef _WIN32`**——平台差异全部由 CMake 选源文件：

```cmake
if(WIN32)
    target_sources(ffi_conformance PRIVATE src/platform/dl_win32.c)
else()
    target_sources(ffi_conformance PRIVATE src/platform/dl_posix.c)
    target_link_libraries(ffi_conformance PRIVATE dl)
endif()
```

### 二、构建：独立 or 聚合

```bash
# 方式 1：只建契约层 + 测试程序（不依赖任何实现）
cmake -S . -B build -G Ninja && cmake --build build

# 方式 2：指定要验证的实现库
cmake -S . -B build -G Ninja -DSOV_FFI_LIBRARY=/path/to/libsov_ffi.dll
cmake --build build && ctest --test-dir build --output-on-failure

# 方式 3：从仓库根聚合构建（cpp + c_abi 同一棵树，自动验证 in-tree 实现）
cd .. && cmake --preset windows-mingw && cmake --build --preset windows-mingw
ctest --preset windows-mingw
```

### 三、安全硬化

契约层的测试程序同样支持 `-DSOV_HARDENED=ON`（模块在 [`../cmake/Hardening.cmake`](../cmake/Hardening.cmake)）：

| 平台 | 注入的编译/链接选项 |
|---|---|
| GCC 15+ / Linux | `-fhardened` |
| Windows / MinGW | `-D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection -ftrivial-auto-var-init=zero -fcf-protection=full` |
| MSVC | `/GS /guard:cf /sdl` + `/DYNAMICBASE /NXCOMPAT` |

> MinGW 上 `-fhardened` 只给 warning（不是 error），`check_c_compiler_flag` 会误判为支持，
> 因此按 `WIN32` 直接判定走回退分支。

### 四、CI

[`../.github/workflows/ci.yml`](../.github/workflows/ci.yml) 三个 job：

| job | 内容 |
|---|---|
| `build-test` | Linux(GCC/Clang)、Windows(MinGW)、macOS(Apple Clang) 三端 configure/build/ctest |
| `cross-windows` | Linux 主机交叉编译出 Windows 产物，并用 `file` 验证 PE 格式 |
| `hardened` | Linux 上 `-fhardened` 构建 + 验证标志确实进入 `compile_commands.json` |

### 五、验证三语言实现的统一流程

```bash
# 对任意实现库跑同一套一致性测试（输出必须逐行相同）
./ffi_conformance <libsov_ffi.so|dll|dylib>      # cpp 实现
./ffi_conformance <sov_guard.dll>                # rust 实现
./ffi_conformance <libsovffi.dll>                # hs 实现
```

这就是"C 作为跨语言中转站"的判据：**契约一份、实现任选、验证统一**。
