cdef class NDArrayBase:
    cdef DLTensor* chandle
    cdef int c_is_view

    property _cvm_handle:
        def __get__(self):
            return <unsigned long long>self.chandle


cdef list _CVM_ND_CLS = []


cdef _register_ndarray(int index, object cls):
    """register object class"""
    global _CVM_ND_CLS
    while len(_CVM_ND_CLS) <= index:
        _CVM_ND_CLS.append(None)

    _CVM_ND_CLS[index] = cls