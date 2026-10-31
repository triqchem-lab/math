# sov-math (Rust) — Sovereign 律算核心的 Rust 实现

workspace 含 6 个 crate，其中 **`sov-guard`** 以 `crate-type = ["rlib", "cdylib"]` 导出
**C ABI**，与 cpp / hs 实现逐符号等价。

---

## 一、构建

### 工具链

`rust-toolchain.toml` 钉 **nightly**（`rustup` 会自动拉取）：

```toml
[toolchain]
channel = "nightly"
components = ["rustfmt", "clippy"]
```

### 构建 + 测试

```bash
cd rust/sov-math

# 全部 crate
cargo build --release

# 只编导出 C ABI 的那个（产出 sov_guard.dll / libsov_guard.so）
cargo build --release -p sov-guard

# 测试
cargo test --release
```

产物位置：`target/release/sov_guard.dll`（Windows）/ `libsov_guard.so`（Linux）。

### 用 C ABI 一致性测试验证

`c_abi` 的 `ffi_conformance` 可加载**任意**实现库，包括 rust：

```bash
cmake -S ../../c_abi -B /tmp/cabi -G Ninja \
      -DSOV_FFI_LIBRARY=target/release/sov_guard.dll
cmake --build /tmp/cabi && ctest --test-dir /tmp/cabi --output-on-failure
```

---

## 二、安全治理

### 2.1 内存安全边界

Rust 默认内存安全，但 **FFI 导出是唯一的 `unsafe` 区**：

| 位置 | 风险 | 约束 |
|---|---|---|
| `#[no_mangle] pub extern "C" fn` | 参数指针可能为 NULL / 越界 | 每个导出函数入口先校验指针，返回 `SOV_ERR_NULL` |
| `unsafe { *ptr }` 解引用 | 调用方生命周期不受控 | 只写入调用方提供的缓冲区，**库内零分配** |
| `std::slice::from_raw_parts` | 长度由调用方给出 | 与契约头 `types.h` 的缓冲区长度约定一致，不信任额外长度 |

**约定：`unsafe` 只出现在 FFI 边界，且每处都要有 `// SAFETY:` 注释说明前提。**

### 2.2 溢出与算术

Rust release 默认 **关闭**整数溢出检查（wrap 而非 panic）。本库是纯整数域，溢出即错：

```toml
# 建议在需要严格算术的 crate 里显式开启
[profile.release]
overflow-checks = true
```

> 当前 workspace 未设置 profile 覆盖（用默认）。若要上生产，建议评估开启
> `overflow-checks` 的性能代价——本库 Q16 定点运算多，热点路径可用 `wrapping_*` 显式标注。

### 2.3 依赖审计

```bash
cargo install cargo-audit
cargo audit                 # 已知漏洞
cargo deny check bans licenses   # 许可证与禁用依赖（需 deny.toml）
```

---

## 三、跨平台

### 3.1 目标三元组

```bash
rustup target list --installed
rustup target add x86_64-pc-windows-gnu      # 从 Linux 交叉编到 Windows
rustup target add aarch64-apple-darwin       # Apple Silicon
```

本机当前：`x86_64-pc-windows-msvc`（默认）。

### 3.2 平台差异

| 项 | 处理 |
|---|---|
| 动态库后缀 | Cargo 按 target 自动产出 `.dll` / `.so` / `.dylib` |
| C ABI 符号名 | `#[no_mangle]` + `extern "C"`，三平台一致 |
| 运行时链接 | Windows 上 cdylib 依赖 MSVC 运行时或 MinGW 运行时，分发时需确认 |

### 3.3 与 C ABI 契约的对接

Rust 侧**不复制**契约头，直接引用 `../../c_abi/include/sov/`：
符号名、参数顺序、返回码语义都以那里为准。任何改动必须先改契约头再改实现。

---

## 四、性能优化

### 4.1 已启用

| 项 | 说明 |
|---|---|
| `--release` | `opt-level = 3` |
| 手写向量化 | `vavx3.rs` 里的数组运算（与 cpp 侧同构） |

### 4.2 建议启用

```toml
# rust/sov-math/Cargo.toml
[profile.release]
lto = "fat"              # 跨 crate 内联（对 cdylib 尤其有效）
codegen-units = 1        # 更好优化，代价是编译变慢
panic = "abort"          # 减小二进制、去掉 unwind 表（FFI 边界本就不该 unwind）
strip = "debuginfo"      # 分发包更小
```

> 注意：`panic = "abort"` 会让 `catch_unwind` 失效——如果 FFI 边界依赖它兜底，就别开。

### 4.3 PGO

```bash
# 1) 采集
RUSTFLAGS="-Cprofile-generate=/tmp/pgo" cargo build --release
# 2) 跑基准
./target/release/<bench>
# 3) 用 profile 重编
RUSTFLAGS="-Cprofile-use=/tmp/pgo/merged.profdata" cargo build --release
```

### 4.4 与 C++ 边界的零拷贝（可选，未来）

若将来要 cpp ↔ rust 直接互调（不经过 C ABI 的纯函数调用），推荐：

- **`cxx` crate**：在 `#[cxx::bridge]` 里声明两侧签名，代码生成器产出 `extern "C"` 桥接 +
  编译期静态断言（`no copying, no serialization, no allocation, no runtime checks`）。
- **`zerocopy`**：`#[derive(FromBytes, IntoBytes)]` + `#[repr(C)]` 让零拷贝二进制解析
  从"手写 unsafe"变成"编译期可验证"。

当前架构走 C ABI 共享库，暂不需要；仅在未来追求极致性能时评估。

---

## 五、目录

```
rust/sov-math/
├── Cargo.toml          workspace 定义（edition 2024）
├── rust-toolchain.toml 钉 nightly
├── sov-core/           核心类型与 GF(3)/GF(9)/环运算
├── sov-chiral/         手性几何
├── sov-topology/       拓扑（ρ / soliton）
├── sov-a4/             A₄ 群与表示
├── sov-validation/     验证族
└── sov-guard/          L7/L8 守卫 + C ABI 导出（cdylib）
```

契约头：[`../../c_abi/include/sov/`](../../c_abi/include/sov/) ·
一致性测试：[`../../c_abi/tests/ffi_conformance.c`](../../c_abi/tests/ffi_conformance.c)
