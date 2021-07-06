import sys
import ctypes

from .base import _LIB, check_call, py_str, c_str, string_types, _FFI_MODE, _RUNTIME_ONLY

try:
    # pylint: disable=wrong-import-position,unused-import
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from ._cy3.core import _register_object
    from ._cy3.core import _get_global_func
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
        The function to be returned, None if function is missing
    """
    return _get_global_func(name, allow_missing)


def list_global_func_names():
    plist = ctypes.POINTER(ctypes.c_char_p)()
    size = ctypes.c_uint()

    check_call(_LIB.CVMFuncListGlobalNames(ctypes.byref(size), ctypes.byref(plist)))
    fnames = []
    for i in range(size.value):
        fnames.append(py_str(plist[i]))
    return fnames


def _get_api(f):
    flocal = f
    flocal.is_global = True
    return flocal


def _init_api(namespace, target_module_name=None):
    """Initialize api for a given module name.

    Parameters
    ----------
    namespace : str
        The namespace for the source registry

    target_module_name : str
        The target module name if different from namespace
    """
    target_module_name = target_module_name if target_module_name else namespace
    if namespace.startswith("cvm."):
        _init_api_prefix(target_module_name, namespace[4:])
    else:
        _init_api_prefix(target_module_name, namespace)


def _init_api_prefix(module_name, prefix):
    module = sys.modules[module_name]

    for name in list_global_func_names():
        if not name.startswith(prefix):
            continue

        fname = name[len(prefix) + 1:]
        target_module = module

        if fname.find(".") != -1:
            continue
        f = get_global_func(name)
        ff = _get_api(f)
        ff.__name__ = fname
        ff.__doc__ = "CVM PackedFunc %s. " % fname
        setattr(target_module, ff.__name__, ff)
