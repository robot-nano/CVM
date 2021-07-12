//
// Created by WJY on 2021/7/9.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_THREAD_LOCAL_H_
#define CVM_INCLUDE_CVM_RUNTIME_THREAD_LOCAL_H_

#include <mutex>
#include <vector>

namespace cvm {
namespace runtime {

#ifndef CVM_CXX11_THREAD_LOCAL
#if defined(_MSC_VER)
#define CVM_CXX11_THREAD_LOCAL (_MSC_VER >= 1900)
#elif defined(__clang__)
#define CVM_CXX11_THREAD_LOCAL (__has_feature(cxx_thread_local))
#else
#define CVM_CXX11_THREAD_LOCAL (__cplusplus >= 201103L)
#endif
#endif

#ifndef CVM_MODERN_THREAD_LOCAL
#define CVM_MODERN_THREAD_LOCAL 1
#endif

#ifdef __GNUC__
#define MX_THREAD_LOCAL __thread
#elif __STDC_VERSION__ >= 201112L
#define MAX_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define MX_THREAD_LOCAL _declspec(thread)
#endif

template <typename T>
class ThreadLocalStore {
 public:
  static T* Get() {
#if CVM_CXX11_THREAD_LOCAL && CVM_MODERN_THREAD_LOCAL == 1
    static thread_local T inst;
    return &inst;
#else
    static MX_THREAD_LOCAL T* ptr = nullptr;
    if (ptr == nullptr) {
      ptr = new T();
      (*Global()).RegisterDeleter(ptr);
    }
    return ptr;
#endif
  }

 private:
  ThreadLocalStore() = default;

  ~ThreadLocalStore() {
    for (size_t i = 0; i < data_.size(); ++i) {
      delete data_[i];
    }
  }

  static ThreadLocalStore<T>* Global() {
    static ThreadLocalStore<T> inst;
    return &inst;
  }

  void RegisterDeleter(T* str) {
    std::unique_lock<std::mutex> lock(mutex_);
    data_.push_back(str);
    lock.unlock();
  }

  std::mutex mutex_;
  std::vector<T*> data_;
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_THREAD_LOCAL_H_
