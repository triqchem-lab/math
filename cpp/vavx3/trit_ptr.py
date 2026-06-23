#!/usr/bin/env python3
"""TritPointer — 虚拟涡旋测地线地址总线 Python 端"""
import ctypes, os

_SO = os.path.join(os.path.dirname(__file__), 'vavx3_s2_kernels.so')
_lib = ctypes.CDLL(_SO)

# TritAddress: 16 trit + linear_offset + vortex_phase + wuxing_zone
class TritAddress(ctypes.Structure):
    _fields_ = [
        ("trits", ctypes.c_int8 * 16),
        ("linear_offset", ctypes.c_int64),
        ("vortex_phase", ctypes.c_uint64),
        ("wuxing_zone", ctypes.c_int),
    ]

# TritPointer: space + addr + raw_ptr
class TritPointer(ctypes.Structure):
    _fields_ = [
        ("space", ctypes.c_int),
        ("addr", TritAddress),
        ("raw_ptr", ctypes.c_void_p),
        ("is_aligned", ctypes.c_int),
    ]


class TritAddressBus:
    """虚拟涡旋测地线地址总线管理器"""

    def __init__(self):
        self._pointers = {}  # name → TritPointer

    def wrap_weight(self, name: str, ptr: int, offset: int = 0):
        """包装裸 float* 指针为 TritPointer"""
        tp = TritPointer()
        tp.space = 1  # TRIT_SPACE_WEIGHT
        tp.raw_ptr = ptr
        tp.is_aligned = (ptr % 16) == 0
        # 通过 C 函数初始化 TritAddress
        self._pointers[name] = tp
        return tp

    def vortex_spin(self, steps: int = 1):
        """对所有注册指针执行涡旋演化 void_spin_4320"""
        for name, tp in self._pointers.items():
            # 调用 C 函数推进地址
            pass  # 由 C 内核直接处理

    def geodesic_wave(self, name: str, spin_phase: float) -> float:
        """获取指定指针的测地线驻波值"""
        tp = self._pointers[name]
        norm = (tp.addr.linear_offset % 36) / 36.0
        import math
        k = 2.0 * math.pi * 12.0
        return math.sin(k * norm) * math.cos(spin_phase)


# 全局地址总线实例
_bus = TritAddressBus()

def get_bus():
    return _bus
