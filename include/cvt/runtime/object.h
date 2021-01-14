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

  static constexpr const char* _type_key = "runtime.Object";

  static uint32_t _GetOrAllocRuntimeTypeIndex() { return TypeIndex::kRoot; }
  static uint32_t RuntimeTypeIndex() { return TypeIndex::kRoot; }

  // Default object type properties for sub-classes
  static constexpr bool _type_final = false;
  static constexpr uint32_t _type_child_slots = 0;
  static constexpr bool _type_child_slots_can_overflow = true;
  // member information
  static constexpr bool _type_has_method_visit_attrs = true;
  static constexpr bool _type_has_method_sequal_reduce = false;
  static constexpr bool _type_has_method_shash_reduce = false;
  // Note: the following field is not type index of Object
  // but was intended to be used by sub-classes as default value.
  // The type index of Object is TypeIndex::kRoot
  static constexpr uint32_t _type_index = TypeIndex::kDynamic;

  Object() = default;

  Object(const Object& other) {  // NOLINT(*)
  }
  Object(Object&& other) {  // NOLINT(*)
  }
  Object& operator=(const Object& other) {  // NOLINT(*)
    return *this;
  }
  Object& operator=(Object&& other) {  // NOLINT(*)
    return *this;
  }

 protected:
  uint32_t type_index_{0};
  RefCounterType ref_counter_{0};

  FDeleter deleter_ = nullptr;

  static_assert(sizeof(int32_t) == sizeof(RefCounterType) &&
                    alignof(int32_t) == sizeof(RefCounterType),
                "RefCounter ABI check.");

  static uint32_t GetOrAllocRuntimeTypeIndex(const std::string& key, uint32_t static_tindex,
                                             uint32_t parent_tindex, uint32_t type_child_slots,
                                             bool type_child_slots_can_overflow);

  inline void IncRef();

  inline void DecRef();

 private:
  inline int use_count() const;
  bool DerivedFrom(uint32_t parent_tindex) const;
  // friend classes
  template <typename>
  friend class ObjAllocatorBase;
  template <typename>
  friend class ObjectPtr;
  friend class CVTRetValue;
  friend class ObjectInternal;
};  // Object

template <typename RelayRefType, typename ObjectType>
inline RelayRefType GetRef(const ObjectType* ptr);

template <typename SubRef, typename BaseRef>
inline SubRef Downcast(BaseRef ref);

/*!
 * \brief A custom smart pointer for Object.
 * \tparam T the content data type.
 * \sa make_object
 */
template <typename T>
class ObjectPtr {
 public:
  ObjectPtr() {}
  ObjectPtr(std::nullptr_t) {}          // NOLINT(*)
  ObjectPtr(const ObjectPtr<T>& other)  // NOLINT(*)
      : ObjectPtr(other.data_) {}

  template <typename U>
  ObjectPtr(const ObjectPtr<U>& other)  // NOLINT(*)
      : ObjectPtr(other.data_) {
    static_assert(std::is_base_of<T, U>::value,
                  "can only assign of child class ObjectPtr to parent");
  }

  ObjectPtr(ObjectPtr<T>&& other)  // NOLINT(*)
      : data_(other.data_) {
    other.data_ = nullptr;
  }

  template <typename Y>
  ObjectPtr(ObjectPtr<Y>&& other)  // NOLINT(*)
      : data_(other.data_) {
    static_assert(std::is_base_of<T, Y>::value,
                  "can only assign of child class ObjectPtr to parent");
    other.data_ = nullptr;
  }

  ~ObjectPtr() { this->reset(); }

  void swap(ObjectPtr<T>& other) {  // NOLINT(*)
    std::swap(data_, other.data_);
  }

  T* get() const { return static_cast<T*>(data_); }

  T* operator->() const { return get(); }

  T& operator*() const {  // NOLINT(*)
    return *get();
  }

  ObjectPtr<T>& operator=(const ObjectPtr<T>& other) {
    ObjectPtr(other).swap(*this);
    return *this;
  }

  ObjectPtr<T>& operator=(ObjectPtr<T>&& other) {  // NOLINT(*)
    ObjectPtr(std::move(other)).swap(*this);       // NOLINT(*)
    return *this;
  }

  /*! \brief reset the content of ptr to be nullptr */
  void reset() {
    if (data_ != nullptr) {
      data_->DecRef();
      data_ = nullptr;
    }
  }

  int use_count() const { return data_ != nullptr ? data_->use_count() : 0; }

  bool unique() const { return data_ != nullptr && data_->use_count() == 1; }

  bool operator==(const ObjectPtr<T>& other) const { return data_ == other.data_; }

  bool operator!=(const ObjectPtr<T>& other) const { return data_ != other.data_; }

  bool operator==(std::nullptr_t null) const { return data_ == nullptr; }

  bool operator!=(std::nullptr_t null) const { return data_ != nullptr; }

 private:
  /*! \brief internal pointer field */
  Object* data_{nullptr};
  /*!
   * \brief constructor from Object
   * \param data The data pointer
   */
  explicit ObjectPtr(Object* data) : data_(data) {
    if (data != nullptr) {
      data_->IncRef();
    }
  }
  /*!
   * \brief Move an ObjectPtr from an RValueRef argument.
   * \param ref The rvalue reference.
   * \return the moved result.
   */
  static ObjectPtr<T> MoveFromRValueRefArg(Object** ref) {
    ObjectPtr<T> ptr;
    ptr.data_ = *ref;
    *ref = nullptr;
    return ptr;
  }
  // friend classes
  friend class Object;
  friend class ObjectRef;
  friend struct ObjectPtrHash;
  template <typename>
  friend class ObjectPtr;
  template <typename>
  friend class ObjAllocatorBase;
  friend class CVTPODValue_;
  friend class CVTArgsSetter;
  friend class CVTRetValue;
  friend class CVTArgValue;
  friend class CVTMovableArgValue_;
  template <typename RelayRefType, typename ObjType>
  friend RelayRefType GetRef(const ObjType* ptr);
  template <typename BaseType, typename ObjType>
  friend ObjectPtr<BaseType> GetObjectPtr(ObjType* ptr);
};

#define CVT_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType) \
  
#if CVT_OBJECT_ATOMIC_REF_COUNTER

inline void Object::IncRef() { ref_counter_.fetch_add(1, std::memory_order_relaxed); }

inline void Object::DecRef() {
  if (ref_counter_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    if (this->deleter_ != nullptr) {
      (*this->deleter_)(this);
    }
  }
}

inline int Object::use_count() const { return ref_counter_.load(std::memory_order_relaxed); }

#else

inline void Object::IncRef() { ++ref_counter_; }

inline void Object::DecRef() {
  if (--ref_counter_ == 0) {
    if (this->deleter_ != nullptr) {
      (*this->deleter_)(this);
    }
  }
}

inline int Object::use_count() const { return ref_counter_; }

#endif  // CVT_OBJECT_ATOMIC_REF_COUNTER

template <typename TargetType>
inline bool Object::IsInstance() const {
  const Object* self = this;
  if (self != nullptr) {
    if (std::is_same<TargetType, Object>::value) return true;
  }
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_RUNTIME_OBJECT_H_
