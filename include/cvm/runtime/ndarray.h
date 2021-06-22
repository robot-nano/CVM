//
// Created by WangJingYu on 2021/6/22.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_
#define CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/object.h>

#include <vector>

namespace cvm {
namespace runtime {

template <typename T>
inline T* BeginPtr(std::vector<T>& vec) {
  if (vec.size() == 0) {
    return NULL;
  } else {
    return &vec[0];
  }
}

typedef DLDevice Device;

class NDArray : public ObjectRef {
 public:
  class ContainerBase;
  class Container;
  using ContainerType = Container;

  NDArray() {}

  explicit NDArray(ObjectPtr<Object> data) : ObjectRef(data) {}

 protected:
  friend class CVMPODValue_;
  friend class CVMRetValue;

  /*!
   * \brief Constructor NDArray's Data field from array handle in FFI.
   * \param handle
   * \return
   */
  inline static ObjectPtr<Object> FFIDataFromHandle(CVMArrayHandle handle);
  inline static void FFIDecRef(CVMArrayHandle handle);
  inline static CVMArrayHandle FFIGetHandle(const ObjectRef& nd);
};

class NDArray::ContainerBase {
 public:
  DLTensor dl_tensor;
  void* manager_ctx{nullptr};

 protected:
  std::vector<int64_t> shape_;
};

class NDArray::Container : public Object, public NDArray::ContainerBase {
 public:
  /*! \brief default constructor */
  Container() {
    // Initialize the type index.
    type_index_ = Container::RuntimeTypeIndex();
    dl_tensor.data = nullptr;
    dl_tensor.ndim = 0;
    dl_tensor.shape = nullptr;
    dl_tensor.strides = nullptr;
    dl_tensor.byte_offset = 0;
  }

  Container(void* data, std::vector<int64_t> shape, DLDataType dtype, Device dev) {
    // Initialize the type index.
    type_index_ = Container::RuntimeTypeIndex();
    dl_tensor.data = data;
    shape_ = std::move(shape);
    dl_tensor.ndim = static_cast<int>(shape_.size());
    dl_tensor.shape = BeginPtr(shape_);
    dl_tensor.dtype = dtype;
    dl_tensor.strides = nullptr;
    dl_tensor.byte_offset = 0;
    dl_tensor.device = dev;
  }
  /*!
   * \brief Set the deleter field.
   * \param deleter
   */
  void SetDeleter(FDeleter deleter) { deleter_ = deleter; }

  using Object::DecRef;
  using Object::IncRef;

  // Information for object protocol.
  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeArray;
  static constexpr const uint32_t _type_child_slots = 0;
  static constexpr const uint32_t _type_child_slots_can_overflow = true;
  static constexpr const char* _type_key = "runtime.NDArray";
  CVM_DECLARE_BASE_OBJECT_INFO(NDArray::Container, Object);

 protected:
  friend class RPCWrappedFunc;
  friend class NDArray;
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_NDARRAY_H_
