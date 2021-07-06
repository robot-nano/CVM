import ctypes
import traceback
from cython cimport Py_INCREF, Py_DECREF

cdef void cvm_callback_finalize(void* fhandle) with gil:
    local_pyfunc = <object>(fhandle)
    Py_DECREF(local_pyfunc)

cdef int cvm_callback(CVMValue* args,
                      int* type_codes,
                      int num_args,
                      CVMRetValueHandle ret,
                      void* fhandle) with gil:
    cdef list pyargs
    cdef CVMValue value
    cdef int tcode
    local_pyfunc = <object>(fhandle)
    pyargs = []
    for i in range(num_args):
        value = args[i]
        tcode = type_codes[i]
        if (tcode == kCVMObjectHandle or
            tcode == kCVMPackedFuncHandle or
            tcode == kCVMModuleHandle or
            tcode == kCVMNDArrayHandle or
            tcode == kCVMObjectRefArg or
            tcode > kCVMExtBegin):
            CALL(CVMCbArgToReturn(&value, &tcode))

        if tcode != kCVMDLTensorHandle:
            pyargs.append(make_ret(value, tcode))
        else:
            pyargs.append(c_make_array(value.v_handle, True, False))
    try:
        rv = local_pyfunc(*pyargs)
    except Exception:
        msg = traceback.format_exc()
        msg = py2cerror(msg)
        CVMAPISetLastError(c_str(msg))
        return -1
    if rv is not None:
        if isinstance(rv, tuple):
            raise ValueError("PackedFunction can only support one return value")
        temp_args = []
        make_arg(rv, &value, &tcode, temp_args)
        CALL(CVMCFuncSetReturn(ret, &value, &tcode, 1))
    return 0


cdef object make_packed_func(CVMPackedFuncHandle chandle, int is_global):
    obj = _CLASS_PACKED_FUNC.__new__(_CLASS_PACKED_FUNC)
    (<PackedFuncBase>obj).chandle = chandle
    (<PackedFuncBase>obj).is_global = is_global
    return obj


def convert_to_cvm_func(object pyfunc):
    """Convert a python function to CVM function

    Parameters
    ----------
    pyfunc : python function
        The python function to be converted.

    Returns
    -------
    cvmfunc : cvm.Function
        The converted cvm function
    """
    cdef CVMPackedFuncHandle chandle
    Py_INCREF(pyfunc)
    CALL(CVMFuncCreateFromCFunc(cvm_callback,
                                <void*>(pyfunc),
                                cvm_callback_finalize,
                                &chandle))
    return make_packed_func(chandle, False)


def _get_global_func(name, allow_missing):
    cdef CVMPackedFuncHandle chandle
    CALL(CVMFuncGetGlobal(c_str(name), &chandle))
    if handle != NULL:
        return make_packed_func(chandle, True)

    if allow_missing:
        return None

    raise ValueError("Cannot find global function %s" % name)
