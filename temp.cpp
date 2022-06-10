#include <iostream>
#include <functional>
#include <memory>

class Object {
 public:
  Object() { std::cout << "Object" << std::endl; }

  int a = 10;
};

// test PR

void packed_c_func_finalizer(void* resource_handle) {
  delete static_cast<Object*>(resource_handle);
}

int main(int argc, char** argv) {
  Object *obj = new Object;
}
