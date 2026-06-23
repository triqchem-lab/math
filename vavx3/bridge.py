#!/usr/bin/env python3
"""VAVX3 SIMD kernel bridge — ctypes binding for S2Sovereign"""
import ctypes, os, numpy as np
from typing import Optional

_KERNEL_DIR = os.path.dirname(os.path.abspath(__file__))
_lib = None

def _load_lib():
    global _lib
    if _lib is None:
        so_path = os.path.join(_KERNEL_DIR, 'vavx3_s2_kernels.so')
        if os.path.exists(so_path):
            _lib = ctypes.CDLL(so_path)
        else:
            raise FileNotFoundError(f"Kernel not found: {so_path}")
    return _lib

# ============ GF(3) Addition ============

def gf3_add(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    """GF(3) modular addition: (a + b) % 3, AVX2 SIMD"""
    assert a.shape == b.shape and a.dtype == np.uint8
    n = a.size
    result = np.empty(n, dtype=np.uint8)
    lib = _load_lib()
    lib.gf3_add_batch(
        a.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        b.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        ctypes.c_int(n),
        result.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
    )
    return result

# ============ GF(3) Subtraction ============

def gf3_sub(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    """GF(3) modular subtraction: (a - b + 3) % 3"""
    assert a.shape == b.shape and a.dtype == np.uint8
    n = a.size
    result = np.empty(n, dtype=np.uint8)
    lib = _load_lib()
    lib.gf3_sub_batch(
        a.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        b.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        ctypes.c_int(n),
        result.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
    )
    return result

# ============ Christoffel Transport ============

def christoffel_transport(
    proto: np.ndarray,   # [H, B, 6] uint8
    query: np.ndarray,   # [B, N, H, 6] uint8
    shifts: np.ndarray,  # [B] uint8
) -> np.ndarray:         # [B, N, H, 6] uint8
    """批量 Christoffel 平行移动 (C内核)"""
    H, B, _ = proto.shape
    _, N, _, _ = query.shape
    result = np.empty((B, N, H, 6), dtype=np.uint8)
    lib = _load_lib()
    lib.christoffel_transport_batch(
        proto.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        query.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        shifts.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        ctypes.c_int(H), ctypes.c_int(B), ctypes.c_int(N),
        result.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
    )
    return result

# ============ Tryte Evaluation (A4) ============

def tryte_eval(trits: np.ndarray) -> tuple:
    """批量 A4 Tryte 评估: 返回 (labels, losses)"""
    assert trits.dtype == np.uint8 and trits.shape[1] == 6
    K = trits.shape[0]
    labels = np.empty(K, dtype=np.int32)
    losses = np.empty(K, dtype=np.float32)
    lib = _load_lib()
    lib.tryte_eval_batch(
        trits.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        ctypes.c_int(K),
        labels.ctypes.data_as(ctypes.POINTER(ctypes.c_int32)),
        losses.ctypes.data_as(ctypes.POINTER(ctypes.c_float)),
    )
    return labels, losses

# ============ Pack/Unpack ============

def pack_trits(trits: np.ndarray) -> np.ndarray:
    """5 trit/byte packing"""
    assert trits.dtype == np.uint8
    n = trits.size
    n_packed = (n + 4) // 5
    packed = np.empty(n_packed, dtype=np.uint8)
    lib = _load_lib()
    lib.pack_trits_5(
        trits.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
        ctypes.c_int(n),
        packed.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)),
    )
    return packed

# ============ Available check ============

def is_available() -> bool:
    try:
        _load_lib()
        return True
    except Exception:
        return False
