cdef extern from "cvm/runtime/c_runtime_api.h":
    const char *CVMGetPrint()


cdef cpp_print():
    CVMGetPrint()

def test_print():
    cpp_print()