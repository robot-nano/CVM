import sys
import ctypes

from .base import _LIB, check_call, py_str, c_str, _FFI_MODE, _RUNTIME_ONLY

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from ._cy3.core import _register_object
    from ._cy3.core import convert_to_cvm_func, _get_global_func, PackedFuncBase
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error


def register_object(type_key=None):
    """register object type.

    Parameters
    ----------
    type_key : str or cls
        The type key of the node

    Examples
    --------

    """
    object_name = type_key if isinstance(type_key, str) else type_key.__name__

    def register(cls):
        """internal register function"""
        if hasattr(cls, "_type_index"):
            tindex = cls._type_index
        else:
            tidx = ctypes.c_uint()
            if not _RUNTIME_ONLY:
                check_call(_LIB.CVMObjectTypeKey2Index(c_str(object_name), ctypes.byref(tidx)))
            else:
                ret = _LIB.CVMObjectTypeKey2Index(c_str(object_name), ctypes.byref(tidx))
                if ret != 0:
                    return cls
            tindex = tidx.value
        _register_object(tindex, cls)
        return cls

    if isinstance(type_key, str):
        return register

    return register(type_key)


def register_func(func_name, f=None, override=False):
    if callable(func_name):
        f = func_name
        func_name = f.__name__

    if not isinstance(func_name, str):
        raise ValueError("expect string function name")

    ioverride = ctypes.c_int(override)

    def register(myf):
        if not isinstance(myf, PackedFuncBase):
            myf = convert_to_cvm_func(myf)
        check_call(_LIB.CVMFuncRegisterGlobal(c_str(func_name), myf.handle, ioverride))
        return myf

    if f:
        return register(f)
    return register


def get_global_func(name, allow_missing=False):
    """Get a global function by name

    Parameters
    ----------
    name : str
        The name of the global function

    allow_missing : bool
        Whether allow missing function or raise an error.

    Returns
    -------
    func : PackedFunc
        The function to returned, None if function is missing
    """
    return _get_global_func(name, allow_missing)
