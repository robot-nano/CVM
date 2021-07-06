from ctypes import *

libc = cdll.LoadLibrary("libc.so.6")

IntArray5 = c_int * 5
ia = IntArray5(5, 1, 7, 33, 99)
qsort = libc.qsort
qsort.restype = None

CMPFUNC = CFUNCTYPE(c_int, POINTER(c_int), POINTER(c_int))


def py_cmp_func(a, b):
    print("py_cmp_func", a[0], b[0])
    return 0


cmp_func = CMPFUNC(py_cmp_func)

qsort(ia, len(ia), sizeof(c_int), cmp_func)
