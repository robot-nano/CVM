#include <iostream>

typedef enum {
  kDLInt = 0U,
  kDLFloat = 1U,
} DLDataType;

typedef enum {
  kInt = kDLInt,
  kNull = 1U,
} CVMType;

int main(int argc, char **argv) {
  int mem[2];
  mem[0] = kDLFloat;
  if (mem[0] == kNull) {
    std::cout << "equal" << std::endl;
  }
}