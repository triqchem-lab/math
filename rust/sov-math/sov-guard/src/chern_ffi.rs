//! C FFI — 精确陈数检测器的跨语言接口 (Python ctypes 加载)
//!
//! 数据布局: [head:12][cell:12][trit:6] u8 扁平缓冲 = 864 字节,
//! 与 pyBitNet `bitnet/gf3/chern2_constructor.py` 的 proto[head][cell][0][i] 同构。
//! 全部整数, 无浮点: C = num/den, 交叉相乘比较 (ChernExact::equals 同构)。

use crate::chern::{compute_chern_exact, HEADS, Proto, S2_CELLS};

/// 扁平缓冲字节数: 12 × 12 × 6 = 864
pub const PROTO_BYTES: usize = HEADS * S2_CELLS * 6;

/// 注入 C=2 单极基准构型到扁平缓冲, 返回注入胞腔数 (72), 失败返回 -1
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sov_chern_inject_monopole(buf: *mut u8, len: usize) -> i32 {
    if buf.is_null() || len < PROTO_BYTES {
        return -1;
    }
    let data = unsafe { std::slice::from_raw_parts_mut(buf, PROTO_BYTES) };
    data.fill(0);
    let mut proto = Proto::new();
    let injected = proto.inject_c2_monopole();
    for h in 0..HEADS {
        for c in 0..S2_CELLS {
            let base = (h * S2_CELLS + c) * 6;
            data[base..base + 6].copy_from_slice(proto.cell_proto(h, c));
        }
    }
    injected as i32
}

/// 从扁平缓冲构造 Proto 并计算精确陈数 C = num/den (分母恒 216)
/// 成功返回 0 并写 out_num/out_den, 参数非法返回 -1
#[unsafe(no_mangle)]
pub unsafe extern "C" fn sov_chern_compute(
    data: *const u8,
    len: usize,
    out_num: *mut i64,
    out_den: *mut i64,
) -> i32 {
    if data.is_null() || out_num.is_null() || out_den.is_null() || len < PROTO_BYTES {
        return -1;
    }
    let src = unsafe { std::slice::from_raw_parts(data, PROTO_BYTES) };
    let mut proto = Proto::new();
    for h in 0..HEADS {
        for c in 0..S2_CELLS {
            let base = (h * S2_CELLS + c) * 6;
            let mut t = [0u8; 6];
            t.copy_from_slice(&src[base..base + 6]);
            proto.set_cell(h, c, t);
        }
    }
    let (num, den) = compute_chern_exact(&proto);
    unsafe {
        *out_num = num;
        *out_den = den;
    }
    0
}
