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



cdef list _CVM_ND_CLS = []


cdef _register_ndarray(int index, object cls):
    """register object class"""
    global _CVM_ND_CLS
    while len(_CVM_ND_CLS) <= index:
        _CVM_ND_CLS.append(None)

    _CVM_ND_CLS[index] = cls