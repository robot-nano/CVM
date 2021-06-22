//
// Created by WangJingYu on 2021/6/21.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_OBJECT_INTERNAL_H_
#define CVM_INCLUDE_CVM_RUNTIME_OBJECT_INTERNAL_H_

namespace cvm {
namespace runtime {

class ObjectInternal {
 public:
  static uint32_t ObjectTypeKey2Index(const std::string& type_key) {
    return 0;
  }
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_OBJECT_INTERNAL_H_
