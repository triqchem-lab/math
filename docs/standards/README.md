# C 标准文本档案 (ISO/IEC 9899) — 来源与用途

> 用途：本仓库 C ABI 的头文件组织、标识符命名、头自足性等要求的**权威依据**。
> ISO 正式文本收费；此处均为 **WG14 官方公开草案**（与正式版逐字一致或为其最终草案）。

| 文件 | 标准 | WG14 编号 | 页数 | 来源 URL |
|---|---|---|---|---|
| `C99-N1256.pdf` / `.txt` | ISO/IEC 9899:1999 (C99, 含 TC1–TC3) | N1256 | 552 | https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf |
| `C23-N3096.pdf` | ISO/IEC 9899:2023 (C23 工作草案) | N3096 | 761 | https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf |
| `C23-N3220.pdf` / `.txt` | ISO/IEC 9899:2024 (C23 定稿) | N3220 | 759 | https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf |

`.txt` 为 `pdftotext` 提取的纯文本（便于按条款 grep 引用）。

## 本仓库直接依赖的条款

| 条款 | 内容 | 本仓库对应 |
|---|---|---|
| **C99 §7.1.2** | 每个库函数都声明在头中；头声明一组相关函数与必要类型/宏 | `dist/include/sov/*.h` 一设施一头 |
| **C99 §7.1.3** | 保留标识符：下划线+大写/下划线开头者任何用途保留；下划线开头者文件作用域保留；实现保留 `str`/`mem`/`wcs`+小写、`E`+数字/大写、`LC_`+大写、`SIG`+大写、`PRI`/`SCN`、`atomic_`/`cnd_`/`mtx_`/`thrd_`/`tss_` 等前缀（§7.26 未来库方向） | 本库公共标识符**全部**用 `sov_` / `SOV_` 前缀 ⇒ 不落入任何保留空间 |
| **C99 §7.26** | 未来库方向（上述保留前缀清单） | 见 `engineering/c99_compliance_check.py` 机械校验 |
| **C23 §7.1.2** | 标准头清单（C23 新增 `<stdbit.h>` `<stdckdint.h>`，移除/并入若干） | 命名与自足性对齐 |
| **C99 §6.10.3.5 / C23** | `__STDC_VERSION__` 取值（C99=199901L, C23=202311L） | 头内版本闸门 |

## 复现下载

```bash
cd docs/standards
curl -C - -O https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf
curl -C - -O https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf
curl -C - -O https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf
pdftotext -q n1256.pdf C99-N1256.txt
```

## 本机标准库实现 (对照用)

glibc 2.43（`/usr/include/*.h` 226 个头）；gcc 支持 `-std=c99`（`__STDC_VERSION__=199901L`）与 `-std=c23`（`202311L`）。
