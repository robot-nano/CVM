from ..base import get_last_ffi_error

cdef extern from "cvm/runtime/c_runtime_api.h":
    const char *CVMGetLastError()
    int CVMGetPrint()

cdef inline int CALL(int ret) except -2:
    # -2 brings exception
    if ret == -2:
        return -2
    if ret != 0:
        raise get_last_ffi_error()
    return 0

cdef cpp_print():
    return CALL(CVMGetPrint())

def test_print():
    return cpp_print()