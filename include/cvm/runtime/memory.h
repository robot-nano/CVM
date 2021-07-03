//
// Created by WangJingYu on 2021/7/3.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_MEMORY_H_
#define CVM_INCLUDE_CVM_RUNTIME_MEMORY_H_

#include <cvm/runtime/object.h>

namespace cvm {
namespace runtime {

template <typename T, typename... Args>
inline ObjectPtr<T> make_object(Args&&... args);

template <typename Derived>
class ObjAllocatorBase {
 public:
  template <typename T, typename... Args>
  inline ObjectPtr<T> make_object(Args&&... args) {
    using Handler = typename Derived::template Handler<T>;
    T* ptr = Handler::New(static_cast<Derived*>(this), std::forward<Args>(args)...);
    ptr->type_index_ = T::RuntimeTypeIndex();
    ptr->deleter_ = Handler::Deleter();
    return ObjectPtr<T>(ptr);
  }
};

class SimpleAllocator : public ObjAllocatorBase<SimpleAllocator> {
 public:
  template <typename T>
  class Handler {
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

   public:
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
};

template <typename T, typename... Args>
inline ObjectPtr<T> make_object(Args&&... args) {
  return SimpleAllocator().template make_object<T>(std::forward<Args>(args)...);
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_MEMORY_H_
