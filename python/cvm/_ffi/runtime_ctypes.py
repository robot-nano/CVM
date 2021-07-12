import ctypes

cvm_shape_index_t = ctypes.c_int64


class CVMByteArray(ctypes.Structure):
    """Temp data structure for byte array"""
    _fields_ = [("data", ctypes.POINTER(ctypes.c_byte)), ("size", ctypes.c_size_t)]


class DataTypeCode(object):
    INT = 0
    UINT = 1
    FLOAT = 2
    HANDLE = 3
    BFLOAT = 4


class DataType(ctypes.Structure):
    _fields_ = [("type_code", ctypes.c_uint8), ("bits", ctypes.c_uint8), ("lanes", ctypes.c_uint16)]


class Device(ctypes.Structure):
    _fields_ = [("device_type", ctypes.c_int), ("device_id", ctypes.c_int)]


class CVMArray(ctypes.Structure):
    """CVMValue in C API"""

    _fields_ = [
        ("data", ctypes.c_void_p),
        ("device", Device),
        ("ndim", ctypes.c_int),
        ("dtype", DataType),
        ("shape", ctypes.POINTER(cvm_shape_index_t)),
        ("strides", ctypes.POINTER(cvm_shape_index_t)),
        ("byte_offset", ctypes.c_uint64)
    ]


class ObjectRValueRef:
    """Represent an RValue ref to an object that can be moved.

    Parameters
    ----------
    obj : cvm.runtime.Object
        The object that this value refers to
    """
    __slots__ = ["obj"]

    def __init__(self, obj):
        self.obj = obj


CVMArrayHandle = ctypes.POINTER(CVMArray)
