//
// Created by WangJingYu on 2021/6/21.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_RUNTIME_BASE_H_
#define CVM_INCLUDE_CVM_RUNTIME_RUNTIME_BASE_H_

#include <cvm/runtime/c_runtime_api.h>

#include <stdexcept>

#define API_BEGIN() try {
#define API_END()                                       \
  }                                                     \
  catch (cvm::runtime::EnvErrorAlreadySet & _except_) { \
    return -2;                                          \
  }                                                     \
  catch (std::exception & _except_) {                   \
    return CVMAPIHandleException(_except_);             \
  }                                                     \
  return 0;  // NOLINT(*)

int CVMAPIHandleException(const std::exception& e);

#endif  // CVM_INCLUDE_CVM_RUNTIME_RUNTIME_BASE_H_
