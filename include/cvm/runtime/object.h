#ifndef CVM_INCLUDE_RUNTIME_OBJECT_H_
#define CVM_INCLUDE_RUNTIME_OBJECT_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/logging.h>

#include <string>
#include <type_traits>
#include <utility>

#ifndef CVM_OBJECT_ATOMIC_REF_COUNTER
#define CVM_OBJECT_ATOMIC_REF_COUNTER 1
#endif

#if CVM_OBJECT_ATOMIC_REF_COUNTER
#include <atomic>
#endif

namespace cvm {
namespace runtime {

struct TypeIndex {
  enum {
    /*! \brief Root object type. */
    kRoot = 0,
    /*! \brief runtime::Module. */
    kRuntimeModule = 1,
    /*! \brief runtime::NDArray */
    kRuntimeNDArray = 2,
    /*! \brief runtime::String. */
    kRuntimeString = 3,
    /*! \brief runtime::Array. */
    kRuntimeArray = 4,
    /*! \brief runtime::Map. */
    kRuntimeMap = 5,
    // static assignments that may subject to change.
    kRuntimeClosure,
    kRuntimeADT,
    kStaticIndexEnd,
    /*! \brief Type index is allocated during runtime. */
    kDynamic = kStaticIndexEnd
  };
};  // namespace TypeIndex

class CVM_DLL Object {
 public:
  typedef void (*FDeleter)(Object* self);


};

/*!
 * \brief A custom smart pointer for Object.
 * \tparam T the content data type.
 * \sa make_object
 */
template <typename T>
class ObjectPtr {
 public:
};

/*! \brief Base class of all object reference */
class ObjectRef {
 public:

};



}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_RUNTIME_OBJECT_H_
