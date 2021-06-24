#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/packed_func.h>

#include <functional>
#include <iostream>
#include <type_traits>

using namespace cvm::runtime;

void temp(CVMArgs args, CVMRetValue* rv) {

}

int main(int argc, char** argv) {
  PackedFunc packed_func(temp);
  packed_func(1, 2);
}