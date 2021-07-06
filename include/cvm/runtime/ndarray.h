//
// Created by WangJingYu on 2021/7/6.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_
#define CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_

#include <cvm/runtime/container.h>

namespace cvm {
namespace runtime {

typedef DLDevice Device;

/*!
 * \brief Managed NDArray.
 *  The array is backed by reference counted blocks.
 */
class NDArray : public ObjectRef {
 public:
  /*! \brief ContainerBase used to back the CVMArrayHandle */
  class ContainerBase;
  /*! \brief NDArray internal container type */
  class Container;
  /*! \brief Container type for Object system. */
  using ContainerType = Container;
  /*! \brief default constructor */
  NDArray() = default;
  /*!
   * \brief constructor
   * \param data ObjectPtr to the data container
   */
  explicit NDArray(ObjectPtr<Object> data) : ObjectRef(data) {}

  CVM_DLL static NDArray Empty(std::vector<int64_t> shape, DLDataType dtype, Device device);

  inline static ObjectPtr<Object> FFIDataFromHandle(CVMArrayHandle handle);

  inline static void FFIDecRef(CVMArrayHandle handle);

  inline static CVMArrayHandle FFIGetHandle(const ObjectRef &nd);
};

class NDArray::ContainerBase {};

class NDArray::Container : public Object, public NDArray::ContainerBase {
  friend class NDArray;
};

inline ObjectPtr<Object> NDArray::FFIDataFromHandle(CVMArrayHandle handle) {
  return GetObjectPtr<Object>(
      static_cast<NDArray::Container *>(reinterpret_cast<NDArray::ContainerBase *>(handle)));
}

inline void NDArray::FFIDecRef(CVMArrayHandle handle) {
  static_cast<NDArray::Container *>(reinterpret_cast<NDArray::ContainerBase *>(handle))->DecRef();
}

inline CVMArrayHandle NDArray::FFIGetHandle(const ObjectRef &nd) {
  auto ptr = reinterpret_cast<CVMArrayHandle>(static_cast<NDArray::ContainerBase *>(
      static_cast<NDArray::Container *>(const_cast<Object *>(nd.get()))));
  return ptr;
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_
