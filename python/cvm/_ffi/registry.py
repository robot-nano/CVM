import sys
import ctypes

from .base import _LIB, check_call, py_str, c_str, _FFI_MODE, _RUNTIME_ONLY

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error
