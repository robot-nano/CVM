//
// Created by WangJingYu on 2021/1/21.
//

#include <iostream>
#include <string>
#include <type_traits>

template <typename T>
class Base {
 public:
  void bar() {
    std::cout << "bar" << std::endl;
  }
};

template <typename T>
class Derived : Base<T> {
 public:
  void foo() {
    Base<T>::bar();
  }
};

int main() {
}