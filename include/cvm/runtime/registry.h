#ifndef CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
#define CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_

#include <cvm/runtime/packed_func.h>

#include <string>
#include <utility>
#include <vector>

namespace cvm {
namespace runtime {

class Registry {
 public:
  CVM_DLL static Registry& Register(const std::string& name, bool override = false);

  // Internal class.
  struct Manager;

 protected:
  /*! \brief name of the function */
  std::string name_;

  friend struct Manager;
};

#define CVM_FUNC_REG_VAR_DEF static CVM_ATTRIBUTE_UNUSED ::cvm::runtime::Registry& __mk_##CVM

#define CVM_REGISTER_GLOBAL(OpName) \
  CVM_STR_CONCAT(CVM_FUNC_REG_VAR_DEF, __COUNTER__) = ::cvm::runtime::Registry::Register(OpName)

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
