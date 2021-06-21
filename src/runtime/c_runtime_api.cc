//
// Created by WangJingYu on 2021/6/18.
//

#include <cvm/runtime/c_runtime_api.h>

#include "runtime_base.h"

const char *CVMGetLastError(void) {
  return "";
}

int CVMAPIHandleException(const std::exception& e) {
  return -1;
}
