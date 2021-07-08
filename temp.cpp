#include <cvm/runtime/c_runtime_api.h>

#include <iostream>
#include <functional>

int main(int argc, char** argv) {
  CVMFunctionHandle handle;
  CVMFuncGetGlobal("test", &handle);
}