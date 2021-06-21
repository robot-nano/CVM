#ifndef CVM_C_RUNTIME_API_H
#define CVM_C_RUNTIME_API_H

#ifdef _MSC_VER
#define CVM_WEAK __declspec(selectany)
#else
#define CVM_WEAK __attribute__((weak))
#endif

#ifndef CVM_DLL
#ifdef _WIN32
#ifdef TVM_EXPORTS
#define CVM_DLL __declspec(dllexport)
#else
#define CVM_DLL __declspec(dllimport)
#endif
#else
#define CVM_DLL __attribute__((visibility("default")))
#endif
#endif

#define CVM_VERSION "0.1.dev0"

#include <dlpack/dlpack.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef int64_t cvm_index_t;

typedef enum {
  kDLAOCL = 5,
  kDLSDAccel = 6,
  kOpenGL = 11,
  kDLMicroDev = 13,
  kDLHexagon = 14,
  kDLWebGPU = 15
  //
} CVMDeviceExtraType;

typedef enum {
  kCVMArgInt = kDLInt,
  kCVMArgFloat = kDLFloat,
  kCVMOpaqueHandle = 3U,
  kCVMNullptr = 4U,
  kCVMDataType = 5U,
} CVMArgTypeCode;

typedef DLTensor* CVMArrayHandle;

typedef union {
  int64_t v_int64;
  double v_float64;
  void* v_handle;
  const char* v_str;
  DLDataType v_type;
  DLDevice v_device;
} CVMValue;

typedef struct {
  const char* data;
  size_t size;
} CVMByteArray;

typedef void* CVMRetValueHandle;
typedef void* CVMFunctionHandle;
typedef void* CVMRetValueHandle;

typedef void* CVMStreamHandle;
typedef void* CVMObjectHandle;

CVM_DLL const char *CVMGetLastError(void);

typedef int (*CVMPackedCFunc)(CVMValue* args, int* type_codes, int num_args,
                              CVMRetValueHandle ret, void* resource_handle);

CVM_DLL int CVMFuncRegisterGlobal(const char* name, CVMFunctionHandle f, int override);

CVM_DLL int CVMFuncGetGlobal(const char *name, CVMFunctionHandle* out);

CVM_DLL int CVMObjectTypeKey2Index(const char* type_key, unsigned* out_tindex);

CVM_DLL int CVMObjectFree(CVMObjectHandle obj);

#ifdef __cplusplus
}
#endif

#endif // CVM_C_RUNTIME_API_H
