#include <cvt/node/container.h>

using namespace cvt;

int main(int argc, char** argv) {

  Map<ObjectRef, ObjectRef> map({{String("1"), String("v2")}});

  String result = Downcast<String, ObjectRef>((*map.begin()).first);
  std::cout << result << std::endl;
}