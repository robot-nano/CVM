//
// Created by WangJingYu on 2021/7/5.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
#define CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_

#include <cvm/runtime/packed_func.h>

namespace cvm {
namespace runtime {

class Registry {
 public:
  CVM_DLL Registry& set_body(PackedFunc f);
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
