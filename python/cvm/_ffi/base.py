import sys
import os
import ctypes
import numpy as np
from . import libinfo

string_types = (str,)
integer_types = (int, np.int32)
numeric_types = integer_types + (float, np.float32)

if sys.platform == "win32":

    def _py_str(x):
        try:
            return x.decode("utf-8")
        except UnicodeDecodeError:
            encoding = "cp" + str(ctypes.cdll.kernel32.GetACP())
        return x.decode(encoding)


    py_str = _py_str
else:
    py_str = lambda x: x.decode("utf-8")


def _load_lib():
    lib_path = libinfo.find_lib_path()
    if sys.platform.startswith("win32") and sys.version_info >= (3, 8):
        for path in libinfo.get_dll_directories():
            os.add_dll_directory(path)
    lib = ctypes.CDLL(lib_path[0], ctypes.RTLD_GLOBAL)
    return lib, os.path.basename(lib_path[0])


_LIB, _LIB_NAME = _load_lib()


def get_last_ffi_error():
    """Create error object given result of CVMGetLastError.

    Returns
    -------

    """
    c_err_msg = py_str(_LIB)
