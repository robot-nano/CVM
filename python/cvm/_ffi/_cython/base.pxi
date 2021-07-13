from ..base import get_last_ffi_error
from libcpp.vector cimport vector
from cpython.version cimport PY_MAJOR_VERSION
from libc.stdint cimport int32_t, int64_t, uint64_t, uint32_t, uint8_t, uint16_t
import ctypes

cdef enum CVMArgTypeCode:
    kInt = 0
    kUInt = 1
    kFloat = 2
    kCVMOpaqueHandle = 3
    kCVMNullptr = 4
    kCVMDataType = 5
    kDLDevice = 6
    kCVMDLTensorHandle = 7
    kCVMObjectHandle = 8
    kCVMModuleHandle = 9
    kCVMPackedFuncHandle = 10
    kCVMStr = 11
    kCVMBytes = 12
    kCVMNDArrayHandle = 13
    kCVMObjectRefArg = 14
    kCVMExtBegin = 15

cdef extern from "cvm/runtime/c_runtime_api.h":
    ctypedef struct DLDataType:
        uint8_t code
        uint8_t bits
        uint16_t lanes

    ctypedef struct DLDevice:
        int device_type
        int device_id

    ctypedef struct DLTensor:
        void *data
        DLDevice device
        int ndim
        DLDataType dtype
        int64_t *shape
        int64_t *strides
        uint64_t byte_offset

    ctypedef struct DLManagedTensor:
        DLTensor dl_tensor
        void *manager_ctx
        void (*deleter)(DLManagedTensor *self)

    ctypedef struct CVMValue:
        int64_t v_int64
        double v_float64
        void *v_handle
        const char *v_str
        DLDataType v_type
        DLDevice v_device


ctypedef int64_t cvm_index_t
ctypedef DLTensor *DLTensorHandle
ctypedef void *CVMStreamHandle
ctypedef void *CVMRetValueHandle
ctypedef void *CVMPackedFuncHandle
ctypedef void *ObjectHandle

ctypedef struct CVMObject:
    uint32_t type_index_
    int32_t ref_counter_
    void (*deleter_)(CVMObject *self)

ctypedef int (*CVMPackedCFunc)(
    CVMValue *args,
    int *type_codes,
    int num_args,
    CVMRetValueHandle ret,
    void *resource_handle)

ctypedef void (*CVMPackedFuncFinalizer)(void *resource_handle)


cdef extern from "cvm/runtime/c_runtime_api.h":
    void CVMAPISetLastError(const char *msg)
    const char *CVMGetLastError()
    int CVMFuncGetGlobal(const char *name,
                         CVMPackedFuncHandle *out)
    int CVMFuncCall(CVMPackedFuncHandle func,
                    CVMValue *arg_values,
                    int *type_codes,
                    int num_args,
                    CVMValue *ret_val,
                    int *ret_type_code)
    int CVMCFuncSetReturn(CVMRetValueHandle ret,
                          CVMValue *value,
                          int *type_code,
                          int num_ret)
    int CVMFuncListGlobalNames(int *out_size,
                               const char** out_array)
    int CVMObjectGetTypeIndex(ObjectHandle obj,
                              unsigned *out_index)
    int CVMFuncCreateFromCFunc(CVMPackedCFunc func,
                               void* resource_handle,
                               CVMPackedFuncFinalizer fin,
                               CVMPackedFuncHandle *out)
    int CVMCbArgToReturn(CVMValue* value, int* code)
    int CVMFuncFree(CVMPackedFuncHandle func)


cdef inline py_str(const char * x):
    if PY_MAJOR_VERSION < 3:
        return x
    else:
        return x.decode("utf-8")

cdef inline c_str(py_str):
    """Create ctypes char* from a python string

    Parameters
    ----------
    py_str : string
        python string

    Returns
    -------
    str : c_char_p
        A char pointer that can be passed to C API
    """
    return py_str.encode("utf-8")

cdef inline int CALL(int ret) except -2:
    # -2 brings exception
    if ret == -2:
        return -2
    if ret != 0:
        raise get_last_ffi_error()
    return 0

cdef inline object ctypes_handle(void *chandle):
    """Cast C handle to ctypes handle."""
    return ctypes.cast(<unsigned long long> chandle, ctypes.c_void_p)

cdef inline void *c_handle(object handle):
    """Cast C types handle to c handle."""
    cdef unsigned long long v_ptr
    v_ptr = handle.value
    return <void *> (v_ptr)
