import ctypes
from cvm._ffi.base import _LIB, c_str, _FFI_MODE

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from cvm._ffi._cy3.core import _set_class_packed_func, PackedFuncBase
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error

PackedFuncHandle = ctypes.c_void_p


class PackedFunc(PackedFuncBase):
    """The PackedFunc object used int CVM

    Function plays an key role to bridge front and backend in CVM.
    Function provide a type-erased interface, you can call function with positional arguments.

    The compiled module returns Function.
    CVM backend also registers and exposes its API as Functions.

    The following are list of common usage scenario of cvm.runtime.PackedFunc

    - Automatic exposure of C++ API into python
    - To call PackedFunc from python side
    - To call python callbacks to inspect result in generated code
    - Bring python hook into C++ backend

    See Also
    --------
    cvm.register_func: How to register global function
    cvm.get_global_func: How to get global function
    """


_set_class_packed_func(PackedFunc)
