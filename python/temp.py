import numpy as np
from cvm._ffi.runtime_ctypes import DataType

data = np.array([1, 2], dtype=np.float32)

data_type = DataType("int")

print(data_type)
