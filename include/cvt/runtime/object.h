//
// Created by WangJingYu on 2021/1/12.
//

#ifndef CVT_INCLUDE_RUNTIME_OBJECT_H_
#define CVT_INCLUDE_RUNTIME_OBJECT_H_

#include <cvt/runtime/c_runtime_api.h>
#include <cvt/support/logging.h>

#include <string>
#include <type_traits>
#include <utility>

#ifndef CVT_OBJECT_ATOMIC_REF_COUNTER
#define CVT_OBJECT_ATOMIC_REF_COUNTER 1
#endif

#if CVT_OBJECT_ATOMIC_REF_COUNTER
#include <atomic>
#endif

namespace cvt {
namespace runtime {

struct TypeIndex {
  enum {
    /*! \brief Root object type. */
    kRoot = 0,
    /*! \brief runtime::String. */
    kRuntimeString = 1,
    /*! \brief runtime::Vector. */
    kRuntimeVector = 2,
    /*! \brief runtime::Map. */
    kRuntimeMap = 3,
    kStaticIndexEnd,
    /*! \brief Type index is allocated during runtime. */
    kDynamic = kStaticIndexEnd
  };
};  // namespace TypeIndex

class CVT_DLL Object {
 public:
  typedef void (*FDeleter)(Object* self);

  uint32_t type_index() const { return type_index_; }

  std::string GetTypeKey() const { return TypeIndex2Key(type_index_); }

  size_t GetTypeKeyHash() const { return TypeIndex2KeyHash(type_index_); }

  template <typename TargetType>
  inline bool IsInstance() const;

  static std::string TypeIndex2Key(uint32_t tindex);

  static size_t TypeIndex2KeyHash(uint32_t tindex);

  static uint32_t TypeKey2Index(const std::string& key);

#if CVT_OBJECT_ATOMIC_REF_COUNTER
  using RefCounterType = std::atomic<int32_t>;
#else
  using RefCounterType = int32_t;
#endif

 protected:
  uint32_t type_index_{0};
  RefCounterType ref_counter_type_{0};

  FDeleter deleter_ = nullptr;

  static_assert(sizeof(int32_t) == sizeof(RefCounterType) &&
                    alignof(int32_t) == sizeof(RefCounterType),
                "RefCounter ABI check.");
};

template <typename TargetType>
inline bool Object::IsInstance() const {
  const Object* self = this;
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_RUNTIME_OBJECT_H_
