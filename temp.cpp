#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/packed_func.h>

#include <functional>
#include <iostream>
#include <type_traits>

using namespace cvm::runtime;

int add_one(int) {
  return 0;
}

enum TempType {
  a = 1,
  b = 2,
  c = 3
};

int main(int argc, char** argv) {
//  TypedPackedFunc<int(int)> packed_func(add_one);


}