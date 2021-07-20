import ctypes

from cvm._ffi.base import _LIB, _RUNTIME_ONLY, check_call, _FFI_MODE, c_str

try:
    if _FFI_MODE == "ctypes":
        raise ImportError()
    from cvm._ffi._cy3.core import _set_class_object, _set_class_object_generic
    from cvm._ffi._cy3.core import ObjectBase
except (RuntimeError, ImportError) as error:
    if _FFI_MODE == "cython":
        raise error


class Object(ObjectBase):
    """Base class for all cvm's runtime objects."""

    __slots__ = []

    def __repr__(self):
        pass #TODO

    def __dir__(self):
        class_names = dir(self.__class__)
        pass

    def __getattr__(self, name):
        if name in self.__slots__:
            raise AttributeError(f"{name} is not set")


_set_class_object(Object)
