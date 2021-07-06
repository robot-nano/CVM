#include <iostream>

class Base {
 public:
  Base(const Base& other) { this->test(other); }

 private:
  template<typename T>
  void test(const T& other) {
  }

  template<typename T>
  void SwitchToClass(T v) {

  }
};

int main(int argc, char **argv) {

}