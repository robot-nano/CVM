//
// Created by WangJingYu on 2021/5/27.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_MEMORY_MANAGER_H_
#define CVM_INCLUDE_CVM_RUNTIME_MEMORY_MANAGER_H_

#include <cvm/runtime/object.h>

namespace cvm {
namespace runtime {

class MemNode : public Object {
 public:
  void origin_func() {}

  friend class Mem;
};

class Mem : public ObjectRef {
  void temp() { /*(*this)->origin_func();*/ }
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_MEMORY_MANAGER_H_
