#include <atomic>
#include <functional>
#include <iostream>
#include <iostream>
#include <memory>
#include <type_traits>
#include <type_traits>
using namespace std;

class pod1 {};  // empty class
class pod2 {
 public:
  int a;
};  // class with public var
class pod3 {
 public:
  int b;
  pod2 p2;
};          // class with a obj
class pod4  // class with a static var
{
 public:
  static int a;
  int b;
};
class pod5 {
  int a;
};                                    // class with private var
class pod6 : public pod1 {};          // class derived from a pod class
class pod7 : virtual public pod1 {};  // class virtual derived from a class
class pod8  // class with a user defined ctor, even if it's an empty function
{
  pod8() {}
};
class pod9  // class with a virtual fun
{
 public:
  virtual void test() {}
};

class pod10  // class with non-static pod class
{
 public:
  pod2& p;
};
class pod11  // class with vars have different access type
{
 public:
  int a;

 private:
  int b;
};
class pod12 : public pod1 {
  int a;
  pod1 b;
};
class pod13 : public pod1  // first member cannot be of the same type as base
{
  pod1 b;
  int a;
};
class pod14 : public pod2 {
  int b;
};  // more than one class has non-static data members

template <class T>
class test {
 public:
  test() {
    cout << typeid(T).name() << " " << std::is_pod<T>::value << " " << std::is_trivial<T>::value
         << " " << std::is_standard_layout<T>::value << std::endl;
  }
};

int main() {
  test<pod1>();
  test<pod2>();
  test<pod3>();
  test<pod4>();
  test<pod5>();
  test<pod6>();
  test<pod7>();
  test<pod8>();
  test<pod9>();
  test<pod10>();
  test<pod11>();
  test<pod12>();
  test<pod13>();
  test<pod14>();
}
