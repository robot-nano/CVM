import ctypes
from cpython cimport Py_INCREF, Py_DECREF
import traceback

cdef object make_packed_func(CVMPackedFuncHandle chandle, int is_global):
    print("make_packed_func")
    pass

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
                return ctypes.cast(<unsigned long long>self.chandle, ctypes.c_void_p)
        def __set__(self, value):
            self._set_handle(value)

    def __init__(self, handle, is_global):
        self._set_handle(handle)
        self.c_is_global = is_global

    def __call__(self, *args):
        cdef CVMValue ret_val
        cdef int ret_tcode
        ret_tcode = kCVMNullptr


def _get_global_func(name, allow_missing):
    cdef CVMPackedFuncHandle chandle
    CALL(CVMFuncGetGlobal(c_str(name), &chandle))
    if chandle != NULL:
        return make_packed_func(chandle, True)

    if allow_missing:
        return None

    raise ValueError("Cannot find global function %s" % name)
