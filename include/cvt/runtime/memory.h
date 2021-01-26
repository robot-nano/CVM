//
// Created by whale on 2021/1/16.
//

#ifndef CVT_INCLUDE_CVT_RUNTIME_MEMORY_H_
#define CVT_INCLUDE_CVT_RUNTIME_MEMORY_H_

#include <cvt/runtime/object.h>

#include <cstdlib>
#include <type_traits>
#include <utility>

namespace cvt {
namespace runtime {

template <typename Derived>
class ObjAllocatorBase {
 public:
  template <typename T, typename... Args>
  inline ObjectPtr<T> make_object(Args&&... args) {
    using Handler = typename Derived::template Handler<T>;
    static_assert(std::is_base_of<Object, T>::value, "make can only be use to object");
    T* tptr = Handler::New(static_cast<Derived*>(this), std::forward<Args>(args)...);
    tptr->type_index_ = T::RuntimeTypeIndex();
    tptr->deleter_ = Handler::Deleter();
    return ObjectPtr<T>(tptr);
  }

  template <typename ArrayType, typename ElemType, typename... Args>
  inline ObjectPtr<ArrayType> make_inplace_array(size_t num_elems, Args&&... args) {
    using Handler = typename Derived::template ArrayHandler<ArrayType, ElemType>;
    static_assert(std::is_base_of<Object, ArrayType>::value,
                  "make_inplace_array can only be used to create object");
    ArrayType* ptr =
        Handler::New(static_cast<Derived*>(this), num_elems, std::forward<Args>(args)...);
    ptr->type_index_ = ArrayType::RuntimeTypeIndex();
    ptr->deleter_ = Handler::Deleter();
    return ObjectPtr<ArrayType>(ptr);
  }
};

class SimpleAllocator : public ObjAllocatorBase<SimpleAllocator> {
 public:
  template <typename T>
  class Handler {
   public:
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    template <typename... Args>
    static T* New(SimpleAllocator*, Args&&... args) {
      StorageType* data = new StorageType();
      new (data) T(std::forward<Args>(args)...);
      return reinterpret_cast<T*>(data);
    }

    static Object::FDeleter Deleter() { return Deleter_; }

   private:
    static void Deleter_(Object* objptr) {
      T* tptr = static_cast<T*>(objptr);
      tptr->T::~T();
      delete reinterpret_cast<StorageType*>(tptr);
    }
  };

  template <typename ArrayType, typename ElemType>
  class ArrayHandler {
   public:
    using StorageType = typename std::aligned_storage<sizeof(ArrayType), alignof(ArrayType)>::type;
    static_assert(alignof(ArrayType) % alignof(ElemType) == 0 &&
                      sizeof(ArrayType) % alignof(ElemType) == 0,
                  "element alignment constraint");

    template <typename... Args>
    static ArrayType* New(SimpleAllocator*, size_t num_elems, Args&&... args) {
      size_t unit = sizeof(StorageType);
      size_t requested_size = num_elems * sizeof(ElemType) + sizeof(ArrayType);
      size_t num_storage_slots = (requested_size + unit - 1) / unit;
      StorageType* data = new StorageType[num_storage_slots];
      new (data) ArrayType(std::forward<Args>(args)...);
      return reinterpret_cast<ArrayType*>(data);
    }

    static Object::FDeleter Deleter() {}

   private:
    static void Deleter_(Object* objptr) {
      ArrayType* tptr = static_cast<ArrayType*>(objptr);
      tptr->ArrayType::~ArrayType();
      StorageType* p = reinterpret_cast<StorageType*>(tptr);
      delete[] p;
    }
  };
};

template <typename T, typename... Args>
inline ObjectPtr<T> make_object(Args&&... args) {
  return SimpleAllocator().template make_object<T>(std::forward<Args>(args)...);
}

template <typename ArrayType, typename ElemType, typename... Args>
inline ObjectPtr<ArrayType> make_inplace_array_object(size_t num_elems, Args&&... args) {
  return SimpleAllocator().make_inplace_array<ArrayType, ElemType>(num_elems,
                                                                   std::forward<Args>(args)...);
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_MEMORY_H_
