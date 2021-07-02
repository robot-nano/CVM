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

  uint32_t type_index() const { return type_index_; }

  std::string GetTypeKey() const { return TypeIndex2Key(type_index_); }

  size_t GetTypeKeyHash() const { return TypeIndex2KeyHash(type_index_); }

  template <typename TargetType>
  inline bool IsInstance() const;
  /*!
   * \return Whether the cell has only one reference
   * \note We use stl style naming to be consistent with known API in shared_ptr
   */
  inline bool unique() const;

  static std::string TypeIndex2Key(uint32_t tindex);

  static size_t TypeIndex2KeyHash(uint32_t tindex);

  static uint32_t TypeKey2Index(const std::string& key);

#if CVM_OBJECT_ATOMIC_REF_COUNTER
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
  // NOTE: the following field is not type index of Object
  // but was intended to be used by sub-classes as default value.
  // The type index of Object is TypeIndex::kRoot
  static constexpr uint32_t _type_index = TypeIndex::kDynamic;

  Object() = default;
  Object(const Object& other) {  // NOLINT
  }
  Object(Object&& other) {  // NOLINT
  }
  Object& operator=(const Object& other) {  // NOLINT
    return *this;
  }
  Object& operator=(Object&& other) {  // NOLINT
    return *this;
  }

 protected:
  /*! \brief Type index(tag) that indicates the type of the object. */
  uint32_t type_index_{0};
  /*! \brief The internal reference counter */
  RefCounterType ref_counter_{0};

  FDeleter deleter_ = nullptr;

  static_assert(sizeof(int32_t) == sizeof(RefCounterType) &&
                    alignof(int32_t) == sizeof(RefCounterType),
                "RefCounter ABI check.");

  static uint32_t GetOrAllocRuntimeTypeIndex(const std::string& skey, uint32_t static_tindex,
                                             uint32_t parent_tindex, uint32_t type_child_slots,
                                             bool type_child_slots_can_overflow);

  inline void IncRef();

  inline void DecRef();

 private:
  inline int use_count() const;

  bool DerivedFrom(uint32_t parent_tindex) const;
  template <typename T>
  friend class ObjectPtr;
};

/*!
 * \brief A custom smart pointer for Object.
 * \tparam T the content data type.
 * \sa make_object
 */
template <typename T>
class ObjectPtr {
 public:
  /*! \brief Default constructor */
  ObjectPtr() = default;
  /*! \brief Default constructor */
  ObjectPtr(std::nullptr_t) {}  // NOLINT
  /*!
   * \brief Copy constructor
   * \param other The value to be moved.
   */
  ObjectPtr(const ObjectPtr<T>& other) : ObjectPtr(other.data_) {}
  /*!
   * \brief Copy constructor
   * \param other The value to be copied.
   */
  template <typename U>
  ObjectPtr(const ObjectPtr<U>& other)  // NOLINT
      : ObjectPtr(other.data_) {
    static_assert(std::is_base_of<T, U>::value,
                  "can only assign of child class ObjectPtr to parent");
  }
  /*!
   * \brief Move constructor
   * \param other The value to be moved.
   */
  ObjectPtr(ObjectPtr<T>&& other)  // NOLINT
      : data_(other.data_) {
    other.data_ = nullptr;
  }
  /*!
   * \brief Move constructor
   * \param other The value to be moved.
   */
  template <typename Y>
  ObjectPtr(ObjectPtr<Y>&& other)  // NOLINT
      : data_(other.data_) {
    static_assert(std::is_base_of<T, Y>::value,
                  "can only assign of child class ObjectPtr to parent");
    other.data_ = nullptr;
  }
  /*! \brief Destructor */
  ~ObjectPtr() { this->reset(); }
  /*!
   * \brief Swap this array with another Object.
   * \param other The other Object
   */
  void swap(ObjectPtr<T>& other) { std::swap(data_, other.data_); }
  /*!
   * \return Get the content of the pointer
   */
  T* get() const { return static_cast<T*>(data_); }
  /*!
   * \return The pointer
   */
  T* operator->() const { return get(); }
  /*!
   * \return The reference
   */
  T& operator*() const { return *get(); }
  /*!
   * \brief Copy assignment
   * \param other The value to be assigned.
   * \return reference to self.
   */
  ObjectPtr<T>& operator=(const ObjectPtr<T>& other) {
    ObjectPtr(other).swap(this);
    return *this;
  }
  /*!
   * \brief Move assignment
   * \param other The value to be assigned.
   * \return reference to self.
   */
  ObjectPtr<T>& operator=(ObjectPtr<T>&& other) {  // NOLINT
    ObjectPtr(std::move(other)).swap(*this);
    return *this;
  }
  /*!
   * \brief
   */
  void reset() {
    if (data_ != nullptr) {
      data_->DecRef();
      data_ = nullptr;
    }
  }
  /*! \return The use count of the ptr, for debug purposes */
  int use_count() const { return data_ != nullptr ? data_->use_count() : 0; }
  /*! \return Whether the reference is unique */
  bool unique() const { return data_ != nullptr && data_->use_count() == 1; }
  /*! \return Whether two ObjectPtr equal each other */
  bool operator==(const ObjectPtr<T>& other) const { return data_ == other.data_; }
  /*! \return Whether two ObjectPtr do not equal each other */
  bool operator!=(const ObjectPtr<T>& other) const { return data_ != other.data_; }
  /*! \return Whether the pointer is nullptr */
  bool operator==(std::nullptr_t null) const { return data_ == nullptr; }
  /*! \return Whether the pointer is not nullptr */
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
  // friend class
  friend class Object;
  friend class ObjectRef;
};

/*! \brief Base class of all object reference */
class ObjectRef {
 public:
  /*! \brief Default constructor */
  ObjectRef() = default;
  /*! \brief Constructor from existing object ptr */
  explicit ObjectRef(ObjectPtr<Object> data) : data_(data) {}
  /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result.
   */
  bool same_as(const ObjectRef& other) const { return data_ == other.data_; }
  /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result.
   */
  bool operator==(const ObjectRef& other) const { return data_ == other.data_; }
  /*!
   * \brief Comparator
   * \param other Another object ref.
   * \return the compare result
   */
  bool operator!=(const ObjectRef& other) const { return data_ != other.data_; }
  /*!
   * \brief Comparator
   * \param other Another object ref by address.
   * \return the compare result.
   */
  bool operator<(const ObjectRef& other) const { return data_.get() < other.data_.get(); }
  /*!
   * \return Whether the object is defined(not null)
   */
  bool defined() const { return data_ != nullptr; }
  /*! \brief The internal object pointer */
  const Object* get() const { return data_.get(); }
  /*! \brief The internal object pointer */
  const Object* operator->() const { return get(); }
  /*! \return Whether the reference is unique */
  bool unique() const { return data_.unique(); }
  /*! \return The use count of the ptr, for debug purposes */
  int use_count() const { return data_.use_count(); }
  /*!
   * \brief Try to downcast the internal Object to a
   *  raw pointer of corresponding type.
   *
   *  The function will return a nullptr if the cast failed.
   *
   *  if (const Add *add = node_ref.As<Add>()) {
   *    // This is an add node
   *  }
   *
   * \tparam ObjectType The target type, must be a subtype of Object
   */
  template <typename ObjectType>
  inline const ObjectType* as() const;

  /*! \brief type indicate the container type. */
  using ContainerType = Object;
  // Default type properties for the reference class.
  static constexpr bool _type_is_nullable = true;

 protected:
  /*! \brief Internal pointer that backs the reference. */
  ObjectPtr<Object> data_;
  /*! \return return a mutable internal ptr, can be used by sub-classes. */
  Object* get_mutable() const { return data_.get(); }
  /*!
   * \brief Internal helper function downcast a ref without check.
   * \note Only used for internal dev purposes.
   * \tparam T The target reference type.
   * \return The casted result.
   */
  template <typename T>
  static T DowncastNoCheck(ObjectRef ref) {
    return T(std::move(ref.data_));
  }
  /*!
   * \brief Clear the object ref data field without DecRef
   *        after we successfully moved the field.
   * \param ref The reference data.
   */
  static void FFIClearAfterMove(ObjectRef* ref) { ref->data_.data_ = nullptr;}
  /*!
   * \brief Internal helper function get data_ as ObjectPtr of ObjectType.
   * \note Only used for internal dev purpose.
   * \tparam ObjectType The corresponding dev purpose.
   * \return The corresponding type.
   */
  template <typename ObjectType>
  static ObjectPtr<ObjectType> GetDataPtr(const ObjectRef& ref) {
    return ObjectPtr<ObjectType>(ref.data_.data_);
  }
};

#define CVM_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)                                     \
  static_assert(!ParentType::_type_final, "ParentObj marked as final");                        \
  static uint32_t RuntimeTypeIndex() {                                                         \
    static_assert(TypeName::_type_child_slots == 0 || ParentType::_type_child_slots == 0 ||    \
                      TypeName::_type_child_slots < ParentType::_type_child_slots,             \
                  "Need to set _type_child_slots when parent specifies it.");                  \
    if (TypeName::_type_index != ::cvm::runtime::TypeIndex::kDynamic) {                        \
      return TypeName::_type_index;                                                            \
    }                                                                                          \
    return _GetOrAllocRuntimeTypeIndex();                                                      \
  }                                                                                            \
  static uint32_t _GetOrAllocRuntimeTypeIndex() {                                              \
    static uint32_t tindex = Object::GetOrAllocRuntimeTypeIndex(                               \
        TypeName::_type_key, TypeName::_type_index, ParentType::_GetOrAllocRuntimeTypeIndex(), \
        TypeName::_type_child_slots, TypeName::_type_child_slots_can_overflow);                \
    return tindex;                                                                             \
  }

#define CVM_DECLARE_FINAL_OBJECT_INFO(TypeName, ParentType) \
  static const constexpr bool _type_final = true;           \
  static const constexpr int _type_child_slots = 0;         \
  CVM_DECLARE_BASE_OBJECT_INFO(TypeName, ParentType)

#if defined(__GNUC__)
#define CVM_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define CVM_ATTRIBUTE_UNUSED
#endif

#define CVM_STR_CONCAT_(__x, __y) __x##__y
#define CVM_STR_CONCAT(__x, __y) CVM_STR_CONCAT_(__x, __y)

#define CVM_OBJECT_REG_VAR_DEF static CVM_ATTRIBUTE_UNUSED uint32_t __make_object_tid

#define CVM_REGISTER_OBJECT_TYPE(TypeName) \
  CVM_STR_CONCAT(CVM_OBJECT_REG_VAR_DEF, __COUNTER__) = TypeName::_GetOrAllocRuntimeTypeIndex()

template <typename TargetType>
inline bool Object::IsInstance() const {
  const Object* self = this;
  // NOTE: the following code can be optimized by
  // compiler dead-code elimination for already known constants.
  if (self != nullptr) {
    // Everything is a subclass of object.
    if (std::is_same<TargetType, Object>::value) return true;
    if (TargetType::_type_final) {
      // if the target type is a final type
      // then we only need to check the equivalence.
      return self->type_index_ == TargetType::RuntimeTypeIndex();
    } else {
      // if target type is a non-leaf type
      // Check if type index falls into the range of reserved slots.
      uint32_t begin = TargetType::RuntimeTypeIndex();
      // The condition will be optimized by constant-folding
      if (TargetType::_type_child_slots != 0) {
        uint32_t end = begin + TargetType::_type_child_slots;
        if (self->type_index_ >= begin && self->type_index_ < end) return true;
      } else {
        if (self->type_index_ == begin) return true;
      }
      if (!TargetType::_type_child_slots_can_overflow) return false;
      // Invariance: parent index
      if (self->type_index_ < TargetType::RuntimeTypeIndex()) return false;
      // The rare slower-path, check type hierarchy
      return self->DerivedFrom(TargetType::RuntimeTypeIndex());
    }
  } else {
    return false;
  }
}

inline bool Object::unique() const { return use_count() == 1; }

// Implementation details below
// Object reference counting.
#if CVM_OBJECT_ATOMIC_REF_COUNTER

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

#endif

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_RUNTIME_OBJECT_H_
