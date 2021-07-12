cdef list OBJECT_TYPE = []

def _register_object(int index, object cls):
    """register object class"""
    if issubclass(cls, NDArrayBase):
        _register_ndarray(index, cls)
        return

    global OBJECT_TYPE
    while len(OBJECT_TYPE) <= index:
        OBJECT_TYPE.append(None)

    OBJECT_TYPE[index] = cls

cdef inline object make_ret_object(void *chandle):
    global OBJECT_TYPE
    global _CLASS_OBJECT
    cdef unsigned tindex
    cdef object handle
    object_type = OBJECT_TYPE
    handle = ctypes_handle(chandle)
    CALL(CVMObjectGetTypeIndex(chandle, &tindex))

    if tindex < len(OBJECT_TYPE):
        cls = OBJECT_TYPE[tindex]
        if cls is not None:
            if issubclass(cls, PyNativeObject):
                obj = _CLASS_OBJECT.__new__(_CLASS_OBJECT)
                (<ObjectBase>obj).chandle = chandle
                return cls.__from_cvm_object_(cls, obj)
            obj = cls.__new__(cls)
        else:
            obj = _CLASS_OBJECT.__new__(_CLASS_OBJECT)
    else:
        obj = _CLASS_OBJECT.__new__(_CLASS_OBJECT)

    (<ObjectBase>obj).chandle = chandle
    return obj


class PyNativeObject:
    """Base class of all CVM objects that also subclass python's builtin types."""
    __slots__ = []

    def __init_cvm_object_by_constructor__(self, fconstructor, *args):
        """Initialize the internal cvm_object by calling constructor function.

        Parameters
        ----------
        fconstructor : Function
            Constructor function.

        args : list of objects
            The arguments to the constructor

        Returns
        -------

        """
        obj = _CLASS_OBJECT.__new__(_CLASS_OBJECT)
        obj.__init_handle_by_constructor__(fconstructor, *args)
        self.__cvm_object__ = obj


cdef class ObjectBase:
    cdef void* chandle

    cdef inline _set_handle(self, handle):
        cdef unsigned long long ptr
        if handle is None:
            self.chandle = NULL
        else:
            ptr = handle.value
            self.chandle = <void*>(ptr)

    property handle:
        def __get__(self):
            return ctypes_handle(self.chandle)

        def __set__(self, value):
            self._set_handle(value)

    def __init_handle_by_constructor__(self, fconstructor, *args):
        """Initialize the handle by calling constructor function.

        Parameters
        ----------
        fconstructor : Function
            Constructor function.

        args : list of objects
            The arguments to the constructor

        Notes
        -----
        We have a special calling convention to call constructor functions.
        So the return handle is directly set into the Node object
        instead of creating a new Node.
        """
        self.chandle = NULL
        cdef void* chandle
        ConstructorCall(
            (<PackedFuncBase>fconstructor).chandle,
            kCVMObjectHandle, args, &chandle)
        self.chandle = chandle
