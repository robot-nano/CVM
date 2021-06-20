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

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

CVM_DLL int CVMGetPrint(void);

#ifdef __cplusplus
}
#endif

#endif  // CVM_C_RUNTIME_API_H
