import ctypes
import traceback
from cpython cimport Py_INCREF, Py_DECREF
from numbers import Number, Integral
from ..base import string_types, py2cerror
from ..runtime_ctypes import DataType, Device, CVMByteArray, ObjectRValueRef

cdef void cvm_callback_finalizer(void * fhandle) with gil:
    local_pyfunc = <object> fhandle
    Py_INCREF(local_pyfunc)

cdef int cvm_callback(CVMValue *args,
                      int *type_codes,
                      int num_args,
                      CVMRetValueHandle ret,
                      void *fhandle) with gil:
    cdef list pyargs
    cdef CVMValue value
    cdef int tcode
    local_pyfunc = <object> (fhandle)
    pyargs = []
    for i in range(num_args):
        value = args[i]
        tcode = type_codes[i]
        print("tcode: ", i, tcode)
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

cdef object make_packed_func(CVMPackedFuncHandle chandle, int is_global):
    obj = _CLASS_PACKED_FUNC.__new__(_CLASS_PACKED_FUNC)
    (<PackedFuncBase> obj).chandle = chandle
    (<PackedFuncBase> obj).is_global = is_global
    return obj

def convert_to_cvm_func(object pyfunc):
    """Convert a python function to CVM function

    Parameters
    ----------
    pyfunc : python function
        The python function to be converted

    Returns
    -------
    cvmfunc : cvm.Function
        The converted cvm function
    """
    cdef CVMPackedFuncHandle chandle
    Py_INCREF(pyfunc)
    CALL(CVMFuncCreateFromCFunc(cvm_callback,
                                <void *> (pyfunc),
                                cvm_callback_finalizer,
                                &chandle))
    return make_packed_func(chandle, False)

cdef inline int make_arg(object arg,
                         CVMValue *value,
                         int *tcode,
                         list temp_args) except -1:
    """Pack arguments into c args cvm call accept"""
    cdef unsigned long long ptr
    if isinstance(arg, ObjectBase):
        value[0].v_handle = (<ObjectBase> arg).chandle
        tcode[0] = kCVMObjectHandle
    elif isinstance(arg, NDArrayBase):
        value[0].v_handle = (<NDArrayBase> arg).chandle
        tcode[0] = (kCVMNDArrayHandle if
                    not (<NDArrayBase> arg).c_is_view else kCVMDLTensorHandle)
    elif isinstance(arg, PyNativeObject):
        value[0].v_handle = (<ObjectBase> (arg.__cvm_object__)).chandle
        tcode[0] = kCVMObjectHandle
    elif isinstance(arg, _CVM_COMPATS):
        ptr = arg._cvm_handle
        value[0].v_handle = (<void*>ptr)
        tcode[0] = arg.__class__._cvm_tcode
    elif isinstance(arg, Integral):
        value[0].v_int64 = arg
        tcode[0] = kInt
    elif isinstance(arg, float):
        value[0].v_float64 = arg
        tcode[0] = kFloat
    elif isinstance(arg, str):
        tstr = c_str(arg)
        value[0].v_str = tstr
        tcode[0] = kCVMStr
        temp_args.append(str)
    elif arg is None:
        value[0].v_handle = NULL
        tcode[0] = kCVMNullptr
    elif isinstance(arg, Number):
        value[0].v_float64 = arg
        tcode[0] = kFloat
    elif isinstance(arg, DataType):
        tstr = c_str(str(arg))
        value[0].v_str = tstr
        tcode[0] = kCVMStr
        temp_args.append(str)
    elif isinstance(arg, Device):
        value[0].v_device = (<DLDevice*>(
            <unsigned long long>ctypes.addressof(arg)))[0]
        tcode[0] = kDLDevice
    elif isinstance(arg, (bytes, bytearray)):
        # from_buffer only takes in bytearray.
        if isinstance(arg, bytes):
            byte_arr = bytearray(arg)
            temp_args.append(byte_arr)
            arg = byte_arr

        arr = CVMByteArray()
        arr.data = ctypes.cast(
            (ctypes.c_byte * len(arg)).from_buffer(arg),
            ctypes.POINTER(ctypes.c_byte))
        arr.size = len(arg)
        value[0].v_handle = <void*>(
            <unsigned long long>ctypes.addressof(arr))
        tcode[0] = kCVMBytes
        temp_args.append(arr)
    elif isinstance(arg, string_types):
        tstr = c_str(arg)
        value[0].v_str = tstr
        tcode[0] = kCVMStr
        temp_args.append(tstr)
    elif isinstance(arg, (list, tuple, dict, _CLASS_OBJECT_GENERIC)):
        arg = _FUNC_CONVERT_TO_OBJECT(arg)
        value[0].v_handle = (<ObjectBase>arg).chandle
        tcode[0] = kCVMObjectHandle
        temp_args.append(arg)
    elif isinstance(arg, _CLASS_MODULE):
        value[0].v_handle = c_handle(arg.handle)
        tcode[0] = kCVMModuleHandle
    elif isinstance(arg, PackedFuncBase):
        value[0].v_handle = (<PackedFuncBase>arg).chandle
        tcode[0] = kCVMPackedFuncHandle
    elif isinstance(arg, ctypes.c_void_p):
        value[0].v_handle = &((<ObjectBase>(arg.obj)).chandle)
        tcode[0] = kCVMObjectRefArg
    elif callable(arg):
        arg = convert_to_cvm_func(arg)
        value[0].v_handle = (<PackedFuncBase>arg).chandle
        tcode[0] = kCVMPackedFuncHandle
        temp_args.append(arg)
    else:
        raise TypeError("Don't know how to handle type %s" % type(arg))
    return 0

cdef inline bytearray make_ret_bytes(void *chandle):
    handle = ctypes_handle(chandle)
    arr = ctypes.cast(handle, ctypes.POINTER(CVMByteArray))[0]
    size = arr.size
    res = bytearray(size)
    rptr = (ctypes.c_byte * size).from_buffer(res)
    if not ctypes.memmove(rptr, arr.data, size):
        raise RuntimeError("memmove failed")
    return res

cdef inline object make_ret(CVMValue value, int tcode):
    """Convert result to return value."""
    if tcode == kCVMObjectHandle:
        return make_ret_object(value.v_handle)
    elif tcode == kCVMNullptr:
        return None
    elif tcode == kInt:
        return value.v_int64
    elif tcode == kFloat:
        return value.v_float64
    elif tcode == kCVMNDArrayHandle:
        return c_make_array(value.v_handle, False, True)
    elif tcode == kCVMStr:
        return py_str(value.v_str)
    elif tcode == kCVMBytes:
        return make_ret_bytes(value.v_handle)
    elif tcode == kCVMOpaqueHandle:
        return ctypes_handle(value.v_handle)
    elif tcode == kDLDevice:
        return Device(value.v_device.device_type, value.v_device.device_id)
    elif tcode == kCVMModuleHandle:
        return _CLASS_MODULE(ctypes_handle(value.v_handle))
    elif tcode == kCVMPackedFuncHandle:
        return make_packed_func(value.v_handle, False)
    elif tcode in _CVM_EXT_RET:
        return _CVM_EXT_RET[tcode](ctypes_handle(value.v_handle))

cdef inline int FuncCall3(void *chandle,
                          tuple args,
                          int nargs,
                          CVMValue *ret_val,
                          int *ret_tcode) except -1:
    cdef CVMValue[3] values
    cdef int[3] tcodes
    nargs = len(args)
    temp_args = []
    for i in range(nargs):
        make_arg(args[i], &values[i], &tcodes[i], temp_args)
    CALL(CVMFuncCall(chandle, &values[0], &tcodes[0],
                     nargs, ret_val, ret_tcode))
    return 0

cdef inline int FuncCall(void *chandle,
                         tuple args,
                         CVMValue *ret_val,
                         int *ret_tcode) except -1:
    cdef int nargs
    nargs = len(args)
    if nargs <= 3:
        FuncCall3(chandle, args, nargs, ret_val, ret_tcode)
        return 0

    cdef vector[CVMValue] values
    cdef vector[int] tcodes
    values.resize(max(nargs, 1))
    tcodes.resize(max(nargs, 1))
    temp_args = []
    for i in range(nargs):
        make_arg(args[i], &values[i], &tcodes[i], temp_args)
    CVMFuncCall(chandle, &values[0], &tcodes[0],
                nargs, ret_val, ret_tcode)
    return 0

cdef inline int ConstructorCall(void *constructor_handle,
                                int type_code,
                                tuple args,
                                void** handle) except -1:
    """Call constructor of a handle function"""
    cdef CVMValue ret_val
    cdef int ret_tcode
    FuncCall(constructor_handle, args, &ret_val, &ret_tcode)
    assert ret_tcode == type_code
    handle[0] = ret_val.v_handle
    return 0

cdef class PackedFuncBase:
    cdef CVMPackedFuncHandle chandle
    cdef int is_global

    cdef inline _set_handle(self, handle):
        if handle is None:
            self.chandle = NULL
        else:
            self.chandle = c_handle(handle)

    property is_global:
        def __get__(self):
            return self.c_is_global != 0

        def __set__(self, value):
            self.c_is_global = value

    property handle:
        def __get__(self):
            if self.chandle == NULL:
                return None
            else:
                return ctypes.cast(<unsigned long long> self.chandle, ctypes.c_void_p)
        def __set__(self, value):
            self._set_handle(value)

    def __init__(self, handle, is_global):
        self._set_handle(handle)
        self.c_is_global = is_global

    def __dealloc__(self):
        if self.is_global == 0:
            CALL(CVMFuncFree(self.chandle))

    def __call__(self, *args):
        cdef CVMValue ret_val
        cdef int ret_tcode
        ret_tcode = kCVMNullptr
        FuncCall(self.chandle, args, &ret_val, &ret_tcode)
        return make_ret(ret_val, ret_tcode)

def _get_global_func(name, allow_missing):
    cdef CVMPackedFuncHandle chandle
    CALL(CVMFuncGetGlobal(c_str(name), &chandle))
    if chandle != NULL:
        return make_packed_func(chandle, True)

    if allow_missing:
        return None

    raise ValueError("Cannot find global function %s" % name)

_CLASS_PACKED_FUNC = None
_CLASS_MODULE = None
_CLASS_OBJECT = None
_CLASS_OBJECT_GENERIC = None
_FUNC_CONVERT_TO_OBJECT = None

def _set_class_packed_func(func_class):
    global _CLASS_PACKED_FUNC
    _CLASS_PACKED_FUNC = func_class
