//
// Created by WangJingYu on 2021/5/2.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_C_BACKEND_API_H_
#define CVM_INCLUDE_CVM_RUNTIME_C_BACKEND_API_H_

#include <cvm/runtime/c_runtime_api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  void* sync_handle;
  int32_t num_task;
} CVMParallelGroupEnv;

#ifdef __cplusplus
}
#endif

#endif  // CVM_INCLUDE_CVM_RUNTIME_C_BACKEND_API_H_
