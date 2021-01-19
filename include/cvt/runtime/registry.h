//
// Created by WangJingYu on 2021/1/19.
//

#ifndef CVT_INCLUDE_CVT_RUNTIME_REGISTRY_H_
#define CVT_INCLUDE_CVT_RUNTIME_REGISTRY_H_

#include <cvt/runtime/packed_func.h>

#include <string>
#include <utility>
#include <vector>

namespace cvt {
namespace runtime {

class Registry {
 public:
  CVT_DLL static Registry& Register(const std::string& name, bool override = false);

  // Internal class.
  struct Manager;

 protected:
  /*! \brief name of the function */
  std::string name_;

  friend struct Manager;
};

#define CVT_FUNC_REG_VAR_DEF static CVT_ATTRIBUTE_UNUSED ::cvt::runtime::Registry& __mk_##CVT

#define CVT_REGISTER_GLOBAL(OpName) \
  CVT_STR_CONCAT(CVT_FUNC_REG_VAR_DEF, __COUNTER__) = ::cvt::runtime::Registry::Register(OpName)

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_REGISTRY_H_
