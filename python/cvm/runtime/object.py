import ctypes

from cvm._ffi.base import _LIB, _RUNTIME_ONLY, check_call, _FFI_MODE, c_str

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from cvm._ffi._cy3.core import ObjectBase
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error


class Object(ObjectBase):
    """

    """
