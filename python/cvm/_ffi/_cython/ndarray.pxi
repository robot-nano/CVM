import ctypes

from ..runtime_ctypes import CVMArrayHandle

cdef class NDArrayBase:
    cdef DLTensor* chandle
    cdef int c_is_view

    cdef inline _set_handle(self, handle):
        cdef unsigned long long ptr
        if handle is None:
            self.chandle = NULL
        else:
            ptr = ctypes.cast(handle, ctypes.c_void_p).value
            self.chandle = <DLTensor*>(ptr)

    property _cvm_handle:
        def __get__(self):
            return <unsigned long long>self.chandle

    property handle:
        def __get__(self):
            if self.chandle == NULL:
                return None
            else:
                return ctypes.cast(
                    <unsigned long long>self.chandle, CVMArrayHandle)

        def __set__(self, value):
            self._set_handle(value)

    property is_view:
        def __get__(self):
            return self.c_is_view != 0

    @property
    def shape(self):
        """Shape of this array"""
        return tuple(self.chandle.shape[i] for i in range(self.chandle.ndim))

    def __init__(self, handle, is_view):
        self._set_handle(handle)
        self.c_is_view = is_view

cdef extern from "cvm/runtime/ndarray.h" namespace "cvm::runtime":
    cdef void* CVMArrayHandleToObjectHandle(DLTensorHandle handle)


cdef c_make_array(void *chandle, is_view, is_container):
    global _CVM_ND_CLS

    if is_container:
        tindex = (
            <CVMObject*>CVMArrayHandleToObjectHandle(<DLTensorHandle>chandle)).type_index_
        if tindex < len(_CVM_ND_CLS):
            cls = _CVM_ND_CLS[tindex]
            if cls is not None:
                ret = cls.__new__(cls)
            else:
                ret = _CLASS_NDARRAY.__new__(_CLASS_NDARRAY)
        else:
            ret = _CLASS_NDARRAY.__new__(_CLASS_NDARRAY)
        (<NDArrayBase>ret).chandle = <DLTensor*>chandle
        (<NDArrayBase>ret).c_is_view = <int>is_view
        return ret
    else:
        ret = _CLASS_NDARRAY.__new__(_CLASS_NDARRAY)
        (<NDArrayBase>ret).chandle = <DLTensor*>chandle
        (<NDArrayBase>ret).c_is_view = <int>is_view
        return ret

cdef list _CMV_ND_CLS = []

cdef _CVM_COMPATS = ()

cdef _CVM_EXT_RET = {}

cdef list _CVM_ND_CLS = []

cdef _register_ndarray(int index, object cls):
    """register ndarray class"""
    global _CVM_ND_CLS
    while len(_CVM_ND_CLS) <= index:
        _CVM_ND_CLS.append(None)

    _CVM_ND_CLS[index] = cls

def _make_array(handle, is_view, is_container):
    cdef unsigned long long ptr
    ptr = ctypes.cast(handle, ctypes.c_void_p).value
    return c_make_array(<void*>ptr, is_view, is_container)

cdef object _CLASS_NDARRAY = None