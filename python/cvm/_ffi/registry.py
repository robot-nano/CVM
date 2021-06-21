import sys
import ctypes

from .base import _LIB, check_call, py_str, c_str, string_types, _FFI_MODE, _RUNTIME_ONLY

try:
    # pylint: disable=wrong-import-position,unused-import
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from ._cy3.core import _register_object
except (RuntimeError, ImportError) as error:
    pass


def register_object(type_key=None):
    object_name = type_key if isinstance(type_key, str) else type_key.__name__

    def register(cls):
        if hasattr(cls, "_type_index"):
            tindex = cls._type_index
        else:
            tidx = ctypes.c_uint()
            if not _RUNTIME_ONLY:
                check_call(_LIB.CVMObjectTypeKey2Index(c_str(object_name), ctypes.byref(tidx)))
            else:
                ret = _LIB.TVMObjectTypeKey2Index(c_str(object_name), ctypes.byref(tidx))
                if ret != 0:
                    return cls
            tindex = tidx.value
        _register_object(tindex, cls)
        return cls

    if isinstance(type_key, str):
        return register

    return register(type_key)
