//
// Created by WangJingYu on 2021/1/21.
//

#include <cvt/node/container.h>

#include <iostream>
#include <map>

using namespace cvt;

class ND {
 public:
  class List {
   public:
    void print() {
      std::cout << kNum[0] << std::endl;
    }
  };

  static constexpr uint64_t kNum[10] {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 10
  };
};

int main() {
  std::map<String, String> raw_map{{"a", "va"}, {"b", "vb"}, {"c", "vc"}, {"d", "vd"}};
  Map<String, String> map(raw_map.begin(), raw_map.end());
}