//
// Created by WangJingYu on 2021/6/22.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_MODULE_H_
#define CVM_INCLUDE_CVM_RUNTIME_MODULE_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/object.h>

namespace cvm {
namespace runtime {

class ModuleNode;

class Module : public ObjectRef {
 public:
  Module() {}
  explicit Module(ObjectPtr<Object> n) : ObjectRef(n) {}

  using ContainerType = ModuleNode;
};

class ModuleNode : public Object {

};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_MODULE_H_
