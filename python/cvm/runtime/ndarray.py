import cvm._ffi

from cvm._ffi.base import _LIB, check_call, _FFI_MODE
from . import _ffi_api

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from cvm._ffi._cy3.core import NDArrayBase
except (RuntimeError, ImportError) as error:
    pass


@cvm._ffi.register_object("runtime.NDArray")
class NDArray(NDArrayBase):
    pass


def empty(shape, dtype="float32"):
    pass
