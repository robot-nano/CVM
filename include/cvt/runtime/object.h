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

/*! \brief Base class of all object reference */
class ObjectRef {
 public:
  ObjectRef() = default;
  explicit ObjectRef(ObjectPtr<Object> data) : data_(data) {}

  bool same_as(const ObjectRef& other) const { return data_ == other.data_; }

  bool operator==(const ObjectRef& other) const { return data_ == other.data_; }

  bool operator!=(const ObjectRef& other) const { return data_ != other.data_; }

  bool operator<(const ObjectRef& other) const { return data_.get() < other.data_.get(); }

  bool defined() const { return data_ != nullptr; }

  const Object* get() const { return data_.get(); }

  const Object* operator->() const { return get(); }

  bool unique() const { return data_.unique(); }

  int use_count() const { return data_.use_count(); }

  template <typename ObjectType>
  inline const ObjectType* as() const;

  using ContainerType = Object;

  static constexpr bool _type_is_nullable = true;

 protected:
  ObjectPtr<Object> data_;

  Object* get_mutable() const { return data_.get(); }

  template <typename T>
  static T DowncastNoCheck(ObjectRef ref) {
    return T(std::move(ref.data_));
  }

  static void FFIClearAfterMove(ObjectRef* ref) { ref->data_.data_ = nullptr; }

  template <typename ObjectType>
  static ObjectPtr<ObjectType> GetDataPtr(const ObjectRef& ref) {
    return ObjectPtr<ObjectType>(ref.data_.data_);
  }
  // friend classes.
  friend struct ObjectPtrHash;
  friend class CVTRetValue;
  friend class CVTArgsSetter;
  friend class ObjectInternal;
  template <typename SubRef, typename BaseRef>
  friend SubRef Downcast(BaseRef ref);
};

template <typename BaseType, typename ObjectType>
inline ObjectPtr<BaseType> GetObjectPtr(ObjectType* ptr);

struct ObjectPtrHash {};

/*!
 * \brief helper macro to declare a base object type that can be inherited.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType
 */
#define CVT_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)                                      \
  static_assert(!ParentType::_type_final, "ParentObj marked as final");                         \
  static uint32_t RuntimeTypeIndex() {                                                          \
    static_assert(TypeName::_type_child_slots == 0 || ParentType::_type_child_slots == 0 ||     \
                      TypeName::_type_child_slots < ParentType::_type_child_slots_can_overflow, \
                  "Need to set _type_child_slots when parent specifices it.");                  \
    if (TypeName::_type_index != ::cvt::runtime::TypeIndex::kDynamic) {                         \
      return TypeName::_type_index;                                                             \
    }                                                                                           \
    return _GetOrAllocRuntimeTypeIndex();                                                       \
  }                                                                                             \
  static uint32_t _GetOrAllocRuntimeTypeIndex() {                                               \
    static uint32_t tidx = Object::GetOrAllocRuntimeTypeIndex(                                  \
        TypeName::_type_key, TypeName::_type_index, ParentType::_GetOrAllocRuntimeTypeIndex(),  \
        TypeName::_type_child_slots, TypeName::_type_child_slots_can_overflow);                 \
    return tidx;                                                                                \
  }

/*!
 * \brief helper macro to declare type information in a final class.
 * \param TypeName The name of the current type.
 * \param ParentType The name of the ParentType.
 */
#define CVT_DECLARE_FINAL_OBJECT_INFO(TypeName, ParentType) \
  static const constexpr bool _type_final = true;           \
  static const constexpr int _type_child_slots = 0;         \
  CVT_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)

/*! \brief helper macro to suppress unused warning */
#if defined(__GNUC__)
#define CVT_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define CVT_ATTRIBUTE_UNUSED
#endif

#define CVT_STR_CONCAT_(__x, __y) __x##__y
#define CVT_STR_CONCAT(__x, __y) CVT_STR_CONCAT_(__x, __y)

#define CVT_OBJECT_REG_VAR_DEF static CVT_ATTRIBUTE_UNUSED uint32_t __make_object_tid

/*!
 * \brief Helper macro to register the object type to runtime.
 * Makes sure that the runtime type table is correctly populated.
 *
 * Use this macro in the cc file for each terminal class.
 */
#define CVT_REGISTER_OBJECT_TYPE(TypeName) \
  CVT_STR_CONCAT(CVT_OBJECT_REG_VAR_DEF, __COUNTER__) = TypeName::_GetOrAllocRuntimeTypeIndex()

/*!
 * \brief Define the default copy/move constructor and assign operator
 * \param TypeName The class typename.
 * */
#define CVT_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName) \
  TypeName(const TypeName& other) = default;              \
  TypeName(TypeName&& other) = default;                   \
  TypeName& operator=(const TypeName& other) = default;   \
  TypeName& operator=(TypeName&& other) = default;

/*!
 * \brief Define object reference methods.
 * \param TypeName The object type name
 * \param
 */
#define CVT_DEFINE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)                        \
  TypeName() = default;                                                                        \
  explicit TypeName(::cvt::runtime::ObjectPtr<::cvt::runtime::Object> n) : ParentType(n) {}    \
  CVT_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                           \
  const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); } \
  const ObjectName* get() const { return operator->(); }                                       \
  using ContainerType = ObjectName;

/*!
 * \brief Define object reference methods that is not nullable.
 *
 * \param TypeName The object type name
 * \param ParentType The parent type of the objectRef
 * \param ObjectName The type name of the object.
 */
#define CVT_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)            \
  explicit TypeName(::cvt::runtime::ObjectPtr<::cvt::runtime::Object> n) : ParentType(n) {}    \
  CVT_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                           \
  const ObjectName* operator->() const { return static_cast<const ObjectName*>(data_.get()); } \
  const ObjectName* get() const { return operator->(); }                                       \
  static constexpr bool _type_is_nullable = false;                                             \
  using ContainerType = ObjectName;

#define CVT_DEFINE_MUTABLE_OBJECT_REF_METHODS(TypeName, ParentType, ObjectName)             \
  TypeName() = default;                                                                     \
  CVT_DEFINE_DEFAULT_COPY_MOVE_AND_ASSIGN(TypeName);                                        \
  explicit TypeName(::cvt::runtime::ObjectPtr<::cvt::runtime::Object> n) : ParentType(n) {} \
  ObjectName* operator->() const { return static_cast<ObjectName*>(data_.get()); }          \
  using ContainerType = ObjectName;

#define CVT_DEFINE_OBJECT_REF_COW_METHOD(ObjectName)     \
  ObjectName* CopyOnWrite() {                            \
    ICHECK(data_ != nullptr);                            \
    if (!data_.unique()) {                               \
      auto n = make_object<ObjectName>(*(operator->())); \
      ObjectPtr<Object>(std::move(n)).swap(data_);       \
    }                                                    \
    return static_cast<ObjectName*>(data_.get());        \
  }

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
    // Everything is a subclass of object.
    if (std::is_same<TargetType, Object>::value) return true;
    if (TargetType::_type_final) {
      // if the target type is a final type
      // then we only need to check equivalence.
      return self->type_index_ == TargetType::RuntimeTypeIndex();
    } else {
      uint32_t begin = TargetType::RuntimeTypeIndex();

      if (TargetType::_type_child_slots != 0) {
        uint32_t end = begin + TargetType::_type_child_slots;
        if (self->type_index_ >= begin && self->type_index_ < end) return true;
      } else {
        if (self->type_index_ == begin) return true;
      }
      if (!TargetType::_type_child_slots_can_overflow) return false;

      if (self->type_index_ < TargetType::RuntimeTypeIndex()) return false;

      return self->DerivedFrom(TargetType::RuntimeTypeIndex());
    }
  } else {
    return false;
  }
}

template <typename SubRef, typename BaseRef>
inline SubRef Downcast(BaseRef ref) {
  if (ref.defined()) {
    ICHECK(ref->template IsInstance<typename SubRef::ContainerType>())
        << "Downcast from " << ref->GetTypeKey() << " to " << SubRef::ContainerType::_type_key
        << " failed.";
  } else {
    ICHECK(SubRef::_type_is_nullable) << "Downcast from nullptr to not nullable reference of "
                                      << SubRef::ContainerType::_type_key;
  }
  return SubRef(std::move(ref.data_));
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_INCLUDE_RUNTIME_OBJECT_H_
