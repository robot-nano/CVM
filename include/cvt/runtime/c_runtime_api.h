//
// Created by WangJingYu on 2021/1/12.
//

#ifndef CVT_C_RUNTIME_API_H
#define CVT_C_RUNTIME_API_H

#ifdef _MSC_VER
#define CVT_WEAK __declspec(selectany)
#else
#define CVT_WEAK __attribute__((weak))
#endif

#ifndef CVT_DLL
#ifdef _WIN32
#ifdef TVM_EXPORTS
#define CVT_DLL __declspec(dllexport)
#else
#define CVT_DLL __declspec(dllimport)
#endif
#else
#define CVT_DLL __attribute__((visibility("default")))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>



#ifdef __cplusplus
}
#endif

#endif  // CVT_C_RUNTIME_API_H
