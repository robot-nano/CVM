# 代码要点

## _ffi.base

```python
def _load_lib():
    lib_path = libinfo.find_lib_path()  # 查找 [libtvm.so, libtvm_runtime.so]
    if sys.platform.startswith("win32") and sys.version_info >= (3, 8):
        for path in libinfo.get_dll_directories():
            os.add_dll_directory(path)
    lib = ctypes.CDLL(lib_path[0], ctypes.RTLD_GLOBAL)
    # 指定 cpp 中返回值为(const char*)的函数，默认为 int
    lib.CVMGetLastError.restype = ctypes.c_char_p   
    return lib, os.path.basename(lib_path[0])

_LIB, _LIB_NAME = _load_lib()
```
