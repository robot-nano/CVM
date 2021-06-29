import ctypes
from ..base import _LIB, check_call, c_str


class NDArrayBase(object):
    """A simple Device/CPU Array object in runtime."""
    __slots__ = ["handle", "is_view"]

    def __init__(self, handle, is_view=False):
        """Initialize the function with handle

        Parameters
        ----------
        handle : CVMArrayHandle
            the handle to the underlying C++ CVMArray
        """
        self.handle = handle
        self.is_view = is_view

    def __del__(self):
        if not self.is_view and _LIB:
            check_call(_LIB.CVMArrayFree(self.handle))

    @property
    def _cvm_handle(self):
        return ctypes.cast(self.handle, ctypes.c_void_p).value

    def _copyto(self, target_nd):
        check_call(_LIB.CVMArrayCopyFromTo(self.handle, target_nd.handle, None))
        return target_nd

    @property
    def shape(self):
        """Shape of this array"""
        return tuple(self.handle.contents.shape[i] for i in range(self.handle.contents.ndim))
