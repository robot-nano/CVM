#include <type_traits>
#include <iostream>

template <typename Enum, typename = typename std::enable_if<std::is_integral<Enum>::value>::type>
void print(Enum ptr) {
  std::cout << ptr << std::endl;
}

enum Type : int {
  t1 = 0,
  t2 = 1,
  t3 = 2
};

int main(int argc, char** argv) {
  Type type = Type::t3;
  print(5);
}