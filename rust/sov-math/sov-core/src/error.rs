// 两层错误体系
//   可恢复 → Result<_, MathError>
//   不可恢复 → constitutional_panic!()

use std::fmt;

/// 主权数学错误 — 可恢复
#[derive(Debug)]
pub enum MathError {
    /// Trit 值不在 {0,1,2} 内
    IllegalTrit { value: u8, context: &'static str },

    /// Tryte 值 ≥ 729
    TryteOverflow { value: u16 },

    /// GF(3) 矩阵乘法维度不匹配
    DimensionMismatch { expected: usize, actual: usize },

    /// LCM 桥陈数未锁定, 逆向桥被拒绝
    ChernNotLocked { current_q16: i32 },

    /// SOV 文件格式错误
    InvalidFormat { detail: String },

    /// I/O 错误
    IoError(String),
}

impl fmt::Display for MathError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            MathError::IllegalTrit { value, context } => {
                write!(f, "🔴 非法 Trit: value={} in {}", value, context)
            }
            MathError::TryteOverflow { value } => {
                write!(f, "🔴 Tryte 溢出: value={} (max=728)", value)
            }
            MathError::DimensionMismatch { expected, actual } => {
                write!(f, "🔴 维度不匹配: expected={}, actual={}", expected, actual)
            }
            MathError::ChernNotLocked { current_q16 } => {
                write!(f, "🔴 陈数未锁定: C_q16={}, 拒绝逆向桥", current_q16)
            }
            MathError::InvalidFormat { detail } => {
                write!(f, "🔴 SOV 格式错误: {}", detail)
            }
            MathError::IoError(e) => {
                write!(f, "🔴 I/O 错误: {}", e)
            }
        }
    }
}

impl std::error::Error for MathError {}

impl From<std::io::Error> for MathError {
    fn from(e: std::io::Error) -> Self {
        MathError::IoError(e.to_string())
    }
}

/// 宪法级断言 — 违反时 panic! (不可恢复)
#[macro_export]
macro_rules! constitutional_panic {
    ($msg:expr) => {
        panic!("🔴 宪法违宪: {}", $msg);
    };
    ($fmt:expr, $($arg:tt)*) => {
        panic!("🔴 宪法违宪: {}", format!($fmt, $($arg)*));
    };
}

/// 宪法级编译期断言
#[macro_export]
macro_rules! constitutional_static_assert {
    ($cond:expr, $msg:expr) => {
        const _: () = if !$cond { panic!($msg) };
    };
}

/// 可恢复检查: trit 值合法性
pub fn check_trit(value: u8, context: &'static str) -> Result<(), MathError> {
    if value <= 2 {
        Ok(())
    } else {
        Err(MathError::IllegalTrit { value, context })
    }
}
