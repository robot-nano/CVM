#include <functional>
#include <iostream>
#include <type_traits>
#include <atomic>
#include <memory>

class Temp {
 public:
  ~Temp() {
    std::cout << "~Temp()" << std::endl;
  }

  int test = 2;
};

class Temp2 : public Temp {
 public:
  ~Temp2() {
    std::cout << "~Temp2()" << std::endl;
  }
};

int main(int argc, char** argv) {
  Temp2* temp2 = new Temp2();
  temp2->Temp2::~Temp2();
  temp2->~Temp2();
}