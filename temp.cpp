#include <cvm/runtime/c_runtime_api.h>

#include <iostream>

int main(int argc, char** argv) {
  std::string res = CVMGetLastError();
  std::cout << res << std::endl;
}