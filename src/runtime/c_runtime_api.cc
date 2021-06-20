//
// Created by WangJingYu on 2021/6/18.
//

#include <stdio.h>
#include <cvm/runtime/c_runtime_api.h>

int CVMGetPrint(void) {
  printf("Calling Native\n");
  return 1;
}

const char *CVMGetLastError(void) {
  return "AttributeError: IntImm object has no attributed __array_interface__\n"
         "Stack trace:\n"
         "  File \"/hdd/ws/cpp/CVM/src/runtime/c_runtime_api.cc\", line 16\n"
         "  0: tvm::ReflectionVTable::GetAttr(tvm::runtime::Object*, tvm::runtime::String const&) const\n"
         "        at /hdd/ws/cpp/CVM/src/runtime/c_runtime_api.cc:16\n";
}
