from cvm._ffi.base import _FFI_MODE, _RUNTIME_ONLY, check_call, _LIB, c_str

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from cvm._ffi._cy3.core import ObjectBase
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error
    pass


class Object(ObjectBase):
    """Base class for all cvm's runtime objects."""

    __slot__ = []

    def __repr__(self):
        return ""
