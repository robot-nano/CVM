//
// Created by WangJingYu on 2021/1/21.
//

#include <type_traits>
#include <iostream>
#include <vector>

class Func {
 public:
  Func& operator=(const Func& other) { this->data_ = other.data_ + 1;  return *this; }
  int mem() const { return data_; }

 public:
  int data_;
};

Func func;
Func fu;

std::vector<int> vec;
int a = 3;
#define Expr &a

int main(int argc, char** argv) {
  if (std::is_lvalue_reference<decltype((Expr))>::value) {
    std::cout << "is lvalue reference " << std::endl;
  } else if (std::is_rvalue_reference<decltype((Expr))>::value) {
    std::cout << "is rvalue reference " << std::endl;
  } else {
    std::cout << "is prvalue" << std::endl;
  }
}