import ctypes
import numpy as np

cvm_shape_index_t = ctypes.c_int64


class DataTypeCode(object):
    INT = 0
    UINT = 1
    FLOAT = 2
    HANDLE = 3
    BFLOAT = 4


class DataType(ctypes.Structure):
    """CVM datatype structure"""

    _fields = [("type_code", ctypes.c_uint8), ("bits", ctypes.c_uint8), ("lanes", ctypes.c_uint16)]
    CODE2STR = {
        DataTypeCode.INT: "int",
        DataTypeCode.UINT: "uint",
        DataTypeCode.FLOAT: "float",
        DataTypeCode.HANDLE: "handle",
        DataTypeCode.BFLOAT: "bfloat"
    }

    def __init__(self, type_str):
        super(DataType, self).__init__()
        if isinstance(type_str, np.dtype):
            type_str = str(type_str)

        if type_str == "bool":
            self.bits = 1
            self.type_code = DataTypeCode.UINT
            self.lanes = 1
            return

        arr = type_str.split("x")
        head = arr[0]
        self.lanes = int(arr[1]) if len(arr) > 1 else 1
        bits = 32

        if head.startswith("int"):
            self.type_code = DataTypeCode.INT
            head = head[3:]
        elif head.startswith("uint"):
            self.type_code = DataTypeCode.UINT
            head = head[3:]
        elif head.startswith("float"):
            self.type_code = DataTypeCode.FLOAT
            head = head[5:]
        elif head.startswith("handle"):
            self.type_code = DataTypeCode.HANDLE
            bits = 64
            head = ""
        elif head.startswith("bfloat"):
            self.type_code = DataTypeCode.BFLOAT
            head = head[6:]
        elif head.startswith("custom"):
            print("TODO: ", __file__)
            # TODO:
        else:
            raise ValueError("Do not know how to handle type %s" % type_str)
        bits = int(head) if head else bits
        self.bits = bits

    def __repr__(self):
        if self.bits == 1 and self.lanes == 1:
            return "bool"
        if self.type_code in DataType.CODE2STR:
            type_name = DataType.CODE2STR[self.type_code]
        else:
            # TODO
            pass

        x = "%s%d" % (type_name, self.bits)
        if self.lanes != 1:
            x += "x%d" % self.lanes
        return x

    def __eq__(self, other):
        return (
            self.bits == other.bits
            and self.type_code == other.type_code
            and self.lanes == other.type_code
        )

    def __ne__(self, other):
        return not self.__eq__(other)


class Device(ctypes.Structure):
    """CVM device structure

    """
    _fields = [("device_type", ctypes.c_int), ("device_id", ctypes.c_int)]
    MASK2STR = {
        1: "cpu",
        2: "cuda",
        4: "opencl",
        5: "aocl",
        6: "sdaccel",
        7: "vulkan",
        8: "metal",
        9: "vpi",
        10: "rocm",
        12: "ext_dev",
        13: "micro_dev",
        14: "hexagon",
        15: "webgpu",
    }
    STR2MASK = {
        "llvm": 1,
        "stackvm": 1,
        "cpu": 1,
        "c": 1,
        "cuda": 2,
        "nvptx": 2,
        "cl": 4,
        "opencl": 4,
        "aocl": 5,
        "aocl_sw_emu": 5,
        "sdaccel": 6,
        "vulkan": 7,
        "metal": 8,
        "vpi": 9,
        "rocm": 10,
        "ext_dev": 12,
        "micro_dev": 13,
        "hexagon": 14,
        "webgpu": 15,
    }

    def __init__(self, device_type, device_id):
        super(Device, self).__init__()
        self.device_type = int(device_type)
        self.device_id = device_id

    def _GetDeviceAttr(self, device_type, device_id, attr_id):
        """Internal helper function to invoke runtime.GetDeviceAttr"""
        pass


class CVMArray(ctypes.Structure):
    """CVMValue in C API"""

    _fields_ = [
        ("data", ctypes.c_void_p),
        ("device", Device),
        ("ndim", ctypes.c_int),
        ("dtype", DataType),
        ("shape", ctypes.POINTER(cvm_shape_index_t)),
        ("strides", ctypes.POINTER(cvm_shape_index_t)),
        ("byte_offset", ctypes.c_uint64),
    ]


CVMArrayHandle = ctypes.POINTER(CVMArray)
