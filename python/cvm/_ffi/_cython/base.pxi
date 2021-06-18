cdef enum CVMArgTypeCode:
    kInt = 0
    kUInt = 1
    kFloat = 2

cdef inline hello():
    print("hello")
    return "hello"