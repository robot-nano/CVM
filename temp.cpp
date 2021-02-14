#include <cvt/node/container.h>

#include <vector>

using namespace cvt;

int main(int argc, char** argv) {
  std::vector<std::pair<String, String>> vec;
  for (int i = 0; i < 255; ++i) {
    vec.emplace_back(std::to_string(i), std::to_string(i) + "v");
  }

  Map<String, String> map(vec);
}