//
// Created by WangJingYu on 2021/1/21.
//

#include <iostream>
#include <string>
#include <type_traits>

template <typename T, std::size_t N>
class static_vector {
  using StorageType = typename std::aligned_storage<1024, 1024>::type;
 public:
  template <typename... Args>
  void emplace_back(Args&&... args) {
    StorageType *data = new StorageType();
    std::cout << alignof(*data) << std::endl;
  }
};

struct alignas(128) ST {
  int a;
};

int main() {
  static_vector<ST, 10> vec;
  vec.emplace_back(ST());
}