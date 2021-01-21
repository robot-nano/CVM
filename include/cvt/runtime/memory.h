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

class SimpleObjAllocator {
 public:
  template <typename T>
  class Handler {
   public:
    using StorageType = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    template <typename... Args>
    static T* New(SimpleObjAllocator*, Args&&... args) {
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

  template <typename T, typename... Args>
  inline ObjectPtr<T> make_object(Args&&... args) {
    T* ptr = Handler<T>::New(static_cast<SimpleObjAllocator*>(this), std::forward<Args>(args)...);
    ptr->type_index_ = T::RuntimeTypeIndex();
    ptr->deleter_ = Handler<T>::Deleter();
    return ObjectPtr<T>(ptr);
  }
};

template <typename T, typename... Args>
inline ObjectPtr<T> make_object(Args&&... args) {
  return SimpleObjAllocator().template make_object<T>(std::forward<Args>(args)...);
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_MEMORY_H_
