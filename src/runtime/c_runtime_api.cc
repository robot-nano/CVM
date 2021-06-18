//
// Created by WangJingYu on 2021/6/18.
//

#include <stdio.h>
#include <cvm/runtime/c_runtime_api.h>

const char *CVMGetPrint(void) {
  printf("Calling Native\n");
  return "return native";
}