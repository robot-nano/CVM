import ctypes
import os
import sys
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
    lib.CVMGetLastError.restype = ctypes.c_char_p
    return lib, os.path.basename(lib_path[0])


try:
    import readline
except ImportError:
    pass

__version__ = libinfo.__version__
_LIB, _LIB_NAME = _load_lib()

_RUNTIME_ONLY = "runtime" in _LIB_NAME

_FFI_MODE = os.environ.get("CVM_FFI", "auto")


def c_str(string):
    """Create ctypes char* from a python string

    Parameters
    ----------
    string : string
        Python string

    Returns
    -------
    str : c_char_p
        A char pointer that can be passed to C API
    """
    return ctypes.c_char_p(string.encode("utf-8"))


def c_array(ctype, values):
    """Create ctypes array from a python array

    Parameters
    ----------
    ctype : ctypes data type
        data type of the array we want to convert to

    values : tuple or list
        data content

    Returns
    -------
    out : ctypes array
        Created ctypes array
    """
    return (ctype * len(values))(*values)


ERROR_TYPE = {}


class CVMError(RuntimeError):
    """Default error thrown by CVM functions.

    CVMError will be raised if you do not give any error type specification.
    """


def register_error(func_name=None, cls=None):
    """Register and error class so it can be recognized by the ffi error handler.

    Parameters
    ----------
    func_name : str or function or class
        The name of the error function

    cls : function
        The function to create the class

    Returns
    -------
    fregister : function
        Register function if f is not specified

    Examples
    --------
    .. code-block:: python

        @cvm.error.register_error
        class MyError(RuntimeError):
            pass

        err_inst = cvm.error.create_ffi_error("MyError: xyz")
        assert isinstance(err_inst, MyError)
    """
    if callable(func_name):
        cls = func_name
        func_name = cls.__name__

    def register(mycls):
        """internal register function"""
        err_name = func_name if isinstance(func_name, str) else mycls.__name__
        ERROR_TYPE[err_name] = mycls
        return mycls

    if cls is None:
        return register
    return register(cls)


def _valid_error_name(name):
    """Check whether name is valid error name."""
    return all(x.isalnum() or x in "_." for x in name)


def _find_error_type(line):
    """Find the error name given the first line of the error message.

    Parameters
    ----------
    line : str
        The first line of error message.

    Returns
    -------
    name : str
        The error name
    """
    if sys.platform == "win32":
        end_pos = line.rfind(":")
        if end_pos == -1:
            return None
        start_pos = line.find(":", 0, end_pos)
        if start_pos == -1:
            err_name = line[:end_pos].strip()
        else:
            err_name = line[start_pos + 1: end_pos].strip()
        if _valid_error_name(err_name):
            return err_name
        return None

    end_pos = line.find(":")
    if end_pos == -1:
        return None
    err_name = line[:end_pos]
    if _valid_error_name(err_name):
        return err_name
    return None


def c2pyerror(err_msg):
    """Translate C API error message to python style.

    Parameters
    ----------
    err_msg : str
        The error message.

    Returns
    -------
    new_msg : str
        Translated message.

    err_type : str
        Detected error type.
    """
    arr = err_msg.split("\n")
    if arr[-1] == "":
        arr.pop()
    err_type = _find_error_type(arr[0])
    trace_mode = False
    stack_trace = []
    message = []
    for line in arr:
        if trace_mode:
            if line.startswith("        "):
                stack_trace[-1] += "\n" + line
            elif line.startswith("  "):
                stack_trace.append(line)
            else:
                trace_mode = False
        if not trace_mode:
            if line.startswith("Stack trace"):
                trace_mode = True
            else:
                message.append(line)
    out_msg = ""
    if stack_trace:
        out_msg += "Traceback (most recent call last):\n"
        out_msg += "\n".join(reversed(stack_trace)) + "\n"
    out_msg += "\n".join(message)
    return out_msg, err_type


def py2cerror(err_msg):
    """Translate python style error message to C style.

    Parameters
    ----------
    err_msg : str
        The error message.

    Returns
    -------
    new_msg : str
        Translated message.
    """
    arr = err_msg.split("\n")
    if arr[-1] == "":
        arr.pop()
    trace_mode = False
    stack_trace = []
    message = []
    for line in arr:
        if trace_mode:
            if line.startswith("  "):
                stack_trace.append(line)
            else:
                trace_mode = False
        if not trace_mode:
            if line.find("Traceback") != -1:
                trace_mode = True
            else:
                message.append(line)
    # Remove the first error name if there are two of them.
    # RuntimeError: MyErrorName: message => MyErrorName: message
    head_arr = message[0].split(":", 3)
    if len(head_arr) >= 3 and _valid_error_name(head_arr[1].strip()):
        head_arr[1] = head_arr[1].strip()
        message[0] = ":".join(head_arr[1])
    # reverse the stack trace.
    out_msg = "\n".join(message)
    if stack_trace:
        out_msg += "\nStack trace:\n"
        out_msg += "\n".join(reversed(stack_trace)) + "\n"
    return out_msg


def get_last_ffi_error():
    """Create error object given result of CVMGetLastError.

    Returns
    -------
    err : object
        The error object based on the err_msg
    """
    c_err_msg = py_str(_LIB.CVMGetLastError())
    py_err_msg, err_type = c2pyerror(c_err_msg)
    if err_type is not None and err_type.startswith("cvm.error."):
        err_type = err_type[10:]
    return ERROR_TYPE.get(err_type, CVMError)(py_err_msg)


def check_call(ret):
    """Check the return value of C API call

    This function will raise exception when error occurs.
    Wrap every API call with this function.

    Parameters
    ----------
    ret : int
        return value from API calls
    """
    if ret != 0:
        return get_last_ffi_error()
