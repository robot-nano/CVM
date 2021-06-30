#ifndef CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
#define CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/container.h>
#include <cvm/runtime/data_type.h>
#include <cvm/runtime/logging.h>
#include <cvm/runtime/module.h>
#include <cvm/runtime/ndarray.h>
#include <cvm/runtime/object.h>

#include <functional>
#include <limits>

#define CVM_CHECK_TYPE_CODE(CODE, T) ICHECK_EQ(CODE, T) << "expected "

namespace cvm {
namespace runtime {

// forward declarations
class CVMArgs;
class CVMArgValue;
class CVMMovableArgValueWithContext_;
class CVMRetValue;
class CVMArgsSetter;

class PackedFunc {
 public:
  using FType = std::function<void(CVMArgs args, CVMRetValue* rv)>;

  PackedFunc() = default;

  PackedFunc(std::nullptr_t null) {}  // NOLINT

  explicit PackedFunc(FType body) : body_(std::move(body)) {}

  template <typename... Args>
  inline CVMRetValue operator()(Args&&... args);

  inline void CallPacked(CVMArgs args, CVMRetValue* rv) const;

  inline FType body() const;

  bool operator==(std::nullptr_t null) const { return body_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return body_ != nullptr; }

 private:
  FType body_;
};

template <typename FType>
class TypedPackedFunc;

/*!
 * \anchor TypedPackedFuncAnchor
 * \brief A PackedFunc wrapper to provide typed function signature.
 * It is backed by a PackedFunc internally.
 *
 * TypedPackedFunc enables compile time type checking.
 * TypedPackedFunc works with the runtime system.
 * - It can be passed as an argument of PackedFunc.
 * - It can be assigned to CVMRetValue.
 * - It can be directly converted to a typed-erased PackedFunc.
 *
 * Developers should prefer TypedPackedFunc over PackedFunc in C++ code
 * as it enables compile time checking.
 * We can construct a TypedPackedFunc from a lambda function
 * with the same signature.
 *
 * \code
 *  // user defined lambda function.
 *  auto addone = [](int x)->int {
 *    return x + 1;
 *  };
 *  // We can directly convert
 *  // lambda function to TypedPackedFunc
 *  TypedPackedFunc<int(int)> ftyped(addone);
 *  // invoke the function.
 *  int y = ftyped(1);
 *  // Can be directly converted to PackedFunc
 *  PackedFunc packed = ftype;
 * \endcode
 * \tparam R The return value of the function.
 * \tparam Args The argument signature of the function.
 */
template <typename R, typename... Args>
class TypedPackedFunc<R(Args...)> {
 public:
  using TSelf = TypedPackedFunc<R(Args...)>;

  TypedPackedFunc() = default;

  TypedPackedFunc(std::nullptr_t null) {}  // NOLINT

  inline TypedPackedFunc(PackedFunc packed);  // NOLINT

  inline TypedPackedFunc(const CVMRetValue& value);  // NOLINT

  inline TypedPackedFunc(const CVMArgValue& value);  // NOLINT

  inline TypedPackedFunc(CVMMovableArgValueWithContext_&& value);  // NOLINT

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda, std::string name) {
    this->template AssignTypedLambda(typed_lambda, name);
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda) {  // NOLINT
    this->template AssignTypedLambda(typed_lambda);
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TSelf& operator=(FLambda typed_lambda) {  // NOLINT
    this->template AssignTypedLambda(typed_lambda);
    return *this;
  }

  TSelf& operator=(PackedFunc packed) {  // NOLINT
    packed_ = std::move(packed);
    return *this;
  }

  CVM_ALWAYS_INLINE R operator()(Args&&... args) const;

  operator PackedFunc() const { return packed(); }  // NOLINT

  const PackedFunc& packed() const { return packed_; }

  bool operator==(std::nullptr_t null) const { return packed_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return packed_ != nullptr; }

 private:
  friend class CVMRetValue;
  PackedFunc packed_;

  template <typename FLambda>
  inline void AssignTypedLambda(FLambda flambda, std::string name);

  template <typename FLambda>
  inline void AssignTypedLambda(FLambda flambda);
};

class CVMArgs {
 public:
  const CVMValue* values;
  const int* type_codes;
  int num_args;

  CVMArgs(const CVMValue* values, const int* type_codes, int num_args)
      : values(values), type_codes(type_codes), num_args(num_args) {}

  inline int size() const;

  inline CVMArgValue operator[](int i) const;
};

inline const char* ArgTypeCode2Str(int type_code);

template <typename T>
struct ObjectTypeChecker {
  static Optional<String> CheckAndGetMisMatch(const Object* ptr) {
    using ContainerType = typename T::ContainerType;
    if (ptr == nullptr) {
      if (T::_type_is_nullable) {
        return NullOpt;
      } else {
        return String("nullptr");
      }
    }
    if (ptr->template IsInstance<ContainerType>()) {
      return NullOpt;
    } else {
      return String(ptr->GetTypeKey());
    }
  }

  static bool Check(const Object* ptr) {
    using ContainerType = typename T::ContainerType;
    if (ptr == nullptr) return T::_type_is_nullable;
    return ptr->template IsInstance<ContainerType>();
  }
  static std::string TypeName() {
    using ContainerType = typename T::ContainerType;
    return ContainerType::_type_key;
  }
};

template <typename T>
struct ObjectTypeChecker<Array<T>> {
  static Optional<String> CheckAndGetMismatch(const Object* ptr) {
    if (ptr == nullptr) {
      return NullOpt;
    }
    if (!ptr->template IsInstance<ArrayNode>()) {
      return String(ptr->GetTypeKey());
    }
    const ArrayNode* n = static_cast<const ArrayNode*>(ptr);
    for (size_t i = 0; i < n->size(); i++) {
      const ObjectRef& p = (*n)[i];
      Optional<String> check_subtype = ObjectTypeChecker<T>::CheckAndGetMisMatch(p.get());
      if (check_subtype.defined()) {
        return String("Array[index " + std::to_string(i) + ": " + check_subtype.value() + "]");
      }
    }
    return NullOpt;
  }
  static bool Check(const Object* ptr) {
    if (ptr == nullptr) return true;
    if (!ptr->template IsInstance<ArrayNode>()) return false;
    const ArrayNode* n = static_cast<const ArrayNode*>(ptr);
    for (const ObjectRef& p : *n) {
      if (!ObjectTypeChecker<T>::Check(p.get())) {
        return false;
      }
    }
    return true;
  }
  static std::string TypeName() { return "Array[" + ObjectTypeChecker<T>::TypeName() + "]"; }
};

template <typename K, typename V>
struct ObjectTypeChecker<Map<K, V>> {
  static Optional<String> CheckAndGetMismatch(const Object* ptr) {
    if (ptr == nullptr) return NullOpt;
    if (!ptr->template IsInstance<MapNode>()) return String(ptr->GetTypeKey());
    const MapNode* n = static_cast<const MapNode*>(ptr);
    for (const auto& kv : *n) {
      Optional<String> key_type = ObjectTypeChecker<K>::CheckAndGetMisMatch(kv.first.get());
      Optional<String> value_type = ObjectTypeChecker<K>::CheckAndGetMisMatch(kv.first.get());
      if (key_type.defined() || value_type.defined()) {
        std::string key_name =
            key_type.defined() ? std::string(key_type.value()) : ObjectTypeChecker<K>::TypeName();
        std::string value_name = value_type.defined() ? std::string(value_type.value())
                                                      : ObjectTypeChecker<V>::TypeName();
        return String("Map[" + key_name + ", " + value_name + "]");
      }
    }
    return NullOpt;
  }
  static bool Check(const Object* ptr) {
    if (ptr == nullptr) return true;
    if (!ptr->template IsInstance<MapNode>()) return false;
    const MapNode* n = static_cast<const MapNode*>(ptr);
    for (const auto& kv : *n) {
      if (!ObjectTypeChecker<K>::Check(kv.first.get())) return false;
      if (!ObjectTypeChecker<V>::Check(kv.second.get())) return false;
    }
    return true;
  }
  static std::string TypeName() {
    return "Map[" + ObjectTypeChecker<K>::TypeName() + ", " + ObjectTypeChecker<V>::TypeName() +
           "]";
  }
};

class CVMPODValue_ {
 public:
  operator double() const {  // NOLINT
    if (type_code_ == kDLInt) {
      return static_cast<double>(value_.v_int64);
    }
    CVM_CHECK_TYPE_CODE(type_code_, kDLFloat);
    return value_.v_float64;
  }
  operator int64_t() const {  // NOLINT
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64;
  }
  operator uint64_t() const {  // NOLINT
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64;
  }
  operator int() const {  // NOLINT
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    ICHECK_LE(value_.v_int64, std::numeric_limits<int>::max());
    ICHECK_GE(value_.v_int64, std::numeric_limits<int>::min());
    return static_cast<int>(value_.v_int64);
  }
  operator bool() const {  // NOLINT
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64 != 0;
  }
  operator void*() const {  // NOLINT
    if (type_code_ == kCVMNullptr) return nullptr;
    if (type_code_ == kCVMDLTensorHandle) return value_.v_handle;
    CVM_CHECK_TYPE_CODE(type_code_, kCVMOpaqueHandle);
    return value_.v_handle;
  }
  operator DLTensor*() const {  // NOLINT
    if (type_code_ == kCVMDLTensorHandle || type_code_ == kCVMNDArrayHandle) {
      return static_cast<DLTensor*>(value_.v_handle);
    } else {
      if (type_code_ == kCVMNullptr) return nullptr;
      LOG(FATAL) << "Expected "
                 << "DLTensor* or NDArray but got " << ArgTypeCode2Str(type_code_);
      return nullptr;
    }
  }
  operator NDArray() const {  // NOLINT
    if (type_code_ == kCVMNullptr) return NDArray(ObjectPtr<Object>(nullptr));
    CVM_CHECK_TYPE_CODE(type_code_, kCVMNDArrayHandle);
    return NDArray(NDArray::FFIDataFromHandle(static_cast<CVMArrayHandle>(value_.v_handle)));
  }
  operator Module() const {  // NOLINT
    if (type_code_ == kCVMNullptr) {
      return Module(ObjectPtr<Object>(nullptr));
    }
    CVM_CHECK_TYPE_CODE(type_code_, kCVMModuleHandle);
    return Module(ObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
  }
  operator Device() const {  // NOLINT
    CVM_CHECK_TYPE_CODE(type_code_, kDLDevice);
    return value_.v_device;
  }
  int type_code() const { return type_code_; }

  template <typename T>
  T* ptr() const {
    return static_cast<T*>(value_.v_handle);
  }
  // ObjectRef handling
  template <typename TObjectRef,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  inline bool IsObjectRef() const;
  template <typename TObjectRef>
  inline TObjectRef AsObjectRef() const;

 protected:
  friend class CVMArgsSetter;
  friend class CVMRetValue;
  CVMPODValue_() : type_code_(kCVMNullptr) {}
  CVMPODValue_(CVMValue value, int type_code) : value_(value), type_code_(type_code) {}
  CVMValue value_;
  int type_code_;
};

class CVMArgValue : public CVMPODValue_ {
 public:
  CVMArgValue() = default;
  CVMArgValue(CVMValue value, int type_code) : CVMPODValue_(value, type_code) {}
  // reuse converter from parent
  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator NDArray;
  using CVMPODValue_::operator Device;
  using CVMPODValue_::operator Module;
  using CVMPODValue_::AsObjectRef;
  using CVMPODValue_::IsObjectRef;

  // conversion operator.
  operator std::string() const {  // NOLINT
    if (type_code_ == kCVMDataType) {
      return DLDataType2String(operator DLDataType());
    } else if (type_code_ == kCVMBytes) {
      CVMByteArray* arr = static_cast<CVMByteArray*>(value_.v_handle);
      return std::string(arr->data, arr->size);
    } else if (type_code_ == kCVMStr) {
      return std::string(value_.v_str);
    } else {
      ICHECK(IsObjectRef<cvm::runtime::String>())
          << "Could not convert CVM object of type " << runtime::Object::TypeIndex2Key(type_code_)
          << " to a string.";
      return AsObjectRef<cvm::runtime::String>().operator std::string();
    }
  }
  operator PackedFunc() const {  // NOLINT
    if (type_code_ == kCVMNullptr) return PackedFunc();
    CVM_CHECK_TYPE_CODE(type_code_, kCVMPackedFuncHandle);
    return *ptr<PackedFunc>();
  }
  template <typename FType>
  operator TypedPackedFunc<FType>() const {  // NOLINT
    return TypedPackedFunc<FType>(operator PackedFunc());
  }
  const CVMValue& value() const { return value_; }

  template <typename T, typename = typename std::enable_if<std::is_class<T>::value>::type>
  inline operator T() const;           // NOLINT
  inline operator DLDataType() const;  // NOLINT
  inline operator DataType() const;    // NOLINT
};

class CVMMovableArgValue_ : public CVMPODValue_ {
 public:
  CVMMovableArgValue_(CVMValue value, int type_code) : CVMPODValue_(value, type_code) {}
  // reuse converter from parent
  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator NDArray;
  using CVMPODValue_::operator Device;
  using CVMPODValue_::operator Module;
  // reuse conversion rule from ArgValue.
  operator std::string() const { return AsArgValue().operator std::string(); }  // NOLINT
  operator PackedFunc() const { return AsArgValue().operator PackedFunc(); }    // NOLINT
  template <typename FType>
  operator TypedPackedFunc<FType>() const {  // NOLINT
    return TypedPackedFunc<FType>(operator PackedFunc());
  }
  operator DLDataType() const { return AsArgValue().operator DLDataType(); }  // NOLINT
  operator DataType() const { return AsArgValue().operator DataType(); }      // NOLINT
  operator CVMArgValue() const { return AsArgValue(); }                       // NOLINT

  template <typename T,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, T>::value>::type>
  inline operator T() const;  // NOLINT

 private:
  CVMArgValue AsArgValue() const { return CVMArgValue(value_, type_code_); }
};

class CVMMovableArgValueWithContext_ {
 public:
  CVMMovableArgValueWithContext_(CVMValue value, int type_code, int arg_index,
                                 const std::string* optional_name)
      : value_(value, type_code), arg_index_(arg_index), optional_name_(optional_name) {}

  template <typename T>
  operator T() const {  // NOLINT
    try {
      return value_;
    } catch (Error& e) {
      LOG(FATAL) << "In function " << (optional_name_ == nullptr ? "<anonymous>" : *optional_name_)
                 << ": error while converting argument " << arg_index_ << ": " << e.what();
      throw;
    }
  }

 private:
  CVMMovableArgValue_ value_;
  int arg_index_;
  const std::string* optional_name_;
};

class CVMRetValue : public CVMPODValue_ {
 public:
  CVMRetValue() = default;

  CVMRetValue(CVMRetValue&& other) : CVMPODValue_(other.value_, other.type_code_) {
    other.value_.v_handle = nullptr;
    other.type_code_ = kCVMNullptr;
  }
  ~CVMRetValue() { this->Clear(); }
  // reuse converter from parent
  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator Device;
  using CVMPODValue_::operator NDArray;
  using CVMPODValue_::operator Module;
  using CVMPODValue_::AsObjectRef;
  using CVMPODValue_::IsObjectRef;

  CVMRetValue(const CVMRetValue& other) : CVMPODValue_() { this->Assign(other); }
  // conversion operators
  operator std::string() const {  // NOLINT
    if (type_code_ == kCVMDataType) {
      return DLDataType2String(operator DLDataType());
    } else if (type_code_ == kCVMBytes) {
      return *ptr<std::string>();
    }
    CVM_CHECK_TYPE_CODE(type_code_, kCVMStr);
    return *ptr<std::string>();
  }
  operator DLDataType() const {  // NOLINT
    if (type_code_ == kCVMStr) {
      return String2DLDataType(operator std::string());
    }
    CVM_CHECK_TYPE_CODE(type_code_, kCVMDataType);
    return value_.v_type;
  }
  operator DataType() const { return DataType(operator DLDataType()); }  // NOLINT
  operator PackedFunc() const {                                          // NOLINT
    if (type_code_ == kCVMNullptr) return PackedFunc();
    CVM_CHECK_TYPE_CODE(type_code_, kCVMPackedFuncHandle);
    return *ptr<PackedFunc>();
  }
  template <typename FType>
  operator TypedPackedFunc<FType>() const {  // NOLINT
    return TypedPackedFunc<FType>(operator PackedFunc());
  }
  // Assign operators
  CVMRetValue& operator=(CVMRetValue&& other) {
    this->Clear();
    value_ = other.value_;
    type_code_ = other.type_code_;
    other.type_code_ = kCVMNullptr;
    return *this;
  }
  CVMRetValue& operator=(double value) {
    this->SwitchToPOD(kDLFloat);
    value_.v_float64 = value;
    return *this;
  }
  CVMRetValue& operator=(std::nullptr_t value) {
    this->SwitchToPOD(kCVMNullptr);
    value_.v_handle = value;
    return *this;
  }
  CVMRetValue& operator=(void* value) {
    this->SwitchToPOD(kCVMOpaqueHandle);
    value_.v_handle = value;
    return *this;
  }
  CVMRetValue& operator=(int64_t value) {
    this->SwitchToPOD(kDLInt);
    value_.v_int64 = value;
    return *this;
  }
  CVMRetValue& operator=(int value) {
    this->SwitchToPOD(kDLInt);
    value_.v_int64 = value;
    return *this;
  }
  CVMRetValue& operator=(DLDevice value) {
    this->SwitchToPOD(kDLDevice);
    value_.v_device = value;
    return *this;
  }
  CVMRetValue& operator=(DLDataType t) {
    this->SwitchToPOD(kCVMDataType);
    value_.v_type = t;
    return *this;
  }
  CVMRetValue& operator=(const DataType& other) { return operator=(other.operator DLDataType()); }
  CVMRetValue& operator=(bool value) {
    this->SwitchToPOD(kDLInt);
    value_.v_int64 = value;
    return *this;
  }
  CVMRetValue& operator=(std::string value) {
    this->SwitchToClass(kCVMStr, std::move(value));
    return *this;
  }
  CVMRetValue& operator=(CVMByteArray value) {
    this->SwitchToClass(kCVMStr, value);
    return *this;
  }
  CVMRetValue& operator=(NDArray other) {
    if (other.data_ != nullptr) {
      this->Clear();
      type_code_ = kCVMNDArrayHandle;
      value_.v_handle = NDArray::FFIGetHandle(other);
      ObjectRef::FFIClearAfterMove(&other);
    } else {
      SwitchToPOD(kCVMNullptr);
    }
    return *this;
  }
  CVMRetValue& operator=(Module m) {
    SwitchToObject(kCVMModuleHandle, std::move(m.data_));
    return *this;
  }
  CVMRetValue& operator=(PackedFunc f) {
    if (f == nullptr) {
      this->SwitchToPOD(kCVMNullptr);
    } else {
      this->SwitchToClass(kCVMPackedFuncHandle, f);
    }
    return *this;
  }
  template <typename FType>
  CVMRetValue& operator=(const TypedPackedFunc<FType>& f) {
    return operator=(f.packed());
  }
  CVMRetValue& operator=(const CVMRetValue& other) {
    this->Assign(other);
    return *this;
  }
  CVMRetValue& operator=(const CVMArgValue& other) {
    this->Assign(other);
    return *this;
  }
  CVMRetValue& operator=(CVMMovableArgValue_&& other) {
    this->Assign(other);
    return *this;
  }
  /*!
   * \brief Move the value back to front-end via C API.
   *    This marks the current container as null.
   *    The managed resources are moved to the front-end.
   *    The front end should take charge in managing them.
   *
   * \param ret_value The return value.
   * \param ret_type_code The return type code.
   */
  void MoveToCHost(CVMValue* ret_value, int* ret_type_code) {
    // cannot move str; need specially handle.
    ICHECK(type_code_ != kCVMStr && type_code_ != kCVMBytes);
    *ret_value = value_;
    *ret_type_code = type_code_;
    type_code_ = kCVMNullptr;
  }
  /*!
   * \brief Construct a new CVMRetValue by
   *    moving from return value stored via C API.
   * \param value the value
   * \param type_code The type code.
   * \return The created CVMRetValue.
   */
  static CVMRetValue MoveFromCHost(CVMValue value, int type_code) {
    // Can move POD and everything under the object system.
    ICHECK(type_code <= kCVMPackedFuncHandle || type_code == kCVMNDArrayHandle);
    CVMRetValue ret;
    ret.value_ = value;
    ret.type_code_ = type_code;
    return ret;
  }
  /*! \brief The value field, if the data is POD */
  const CVMValue& value() const {
    ICHECK(type_code_ != kCVMObjectHandle && type_code_ != kCVMPackedFuncHandle &&
           type_code_ != kCVMModuleHandle && type_code_ != kCVMStr)
        << "CVMRetValue.value can only be used for POD data.";
    return value_;
  }
  // ObjectRef handling
  template <typename TObjectRef,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  inline CVMRetValue& operator=(TObjectRef other);
  template <typename T, typename = typename std::enable_if<std::is_class<T>::value>::type>
  inline operator T() const;

 private:
  template <typename T>
  void Assign(const T& other) {
    switch (other.type_code()) {
      case kCVMStr: {
        SwitchToClass<std::string>(kCVMStr, other);
        break;
      }
      case kCVMBytes: {
        SwitchToClass<std::string>(kCVMBytes, other);
        break;
      }
      case kCVMPackedFuncHandle: {
        SwitchToClass<PackedFunc>(kCVMPackedFuncHandle, other);
        break;
      }
      case kCVMModuleHandle: {
        *this = other.operator Module();
        break;
      }
      case kCVMNDArrayHandle: {
        *this = other.operator NDArray();
        break;
      }
      case kCVMObjectHandle: {
        // Avoid operator ObjectRef as we already know it is not NDArray/Module
        SwitchToObject(kCVMObjectHandle,
                       GetObjectPtr<Object>(static_cast<Object*>(other.value_.v_handle)));
        break;
      }
      case kCVMObjectRValueRefArg: {
        operator=(other.operator ObjectRef());
        break;
      }
      default: {
        SwitchToPOD(other.type_code());
        value_ = other.value_;
        break;
      }
    }
  }

  void SwitchToPOD(int type_code) {
    if (type_code_ != type_code) {
      this->Clear();
      type_code_ = type_code;
    }
  }
  template <typename T>
  void SwitchToClass(int type_code, T v) {
    if (type_code_ != type_code) {
      this->Clear();
      type_code_ = type_code;
      value_.v_handle = new T(v);
    } else {
      *static_cast<T*>(value_.v_handle) = v;
    }
  }
  void SwitchToObject(int type_code, ObjectPtr<Object> other) {
    if (other.data_ != nullptr) {
      this->Clear();
      type_code_ = type_code;
      // move the handle out
      value_.v_handle = other.data_;
      other.data_ = nullptr;
    } else {
      SwitchToPOD(kCVMNullptr);
    }
  }

  void Clear() {
    if (type_code_ == kCVMNullptr) return;
    switch (type_code_) {
      case kCVMStr:
      case kCVMBytes:
        delete ptr<std::string>();
        break;
      case kCVMPackedFuncHandle:
        delete ptr<PackedFunc>();
        break;
      case kCVMNDArrayHandle: {
        // TODO:
        break;
      }
      case kCVMModuleHandle: {
        static_cast<Object*>(value_.v_handle)->DecRef();
        break;
      }
      case kCVMObjectHandle: {
        static_cast<Object*>(value_.v_handle)->DecRef();
        break;
      }
    }
    type_code_ = kCVMNullptr;
  }
};

class CVMArgsSetter {
 public:
  CVMArgsSetter(CVMValue* values, int* type_codes) : values_(values), type_codes_(type_codes) {}

  template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
  CVM_ALWAYS_INLINE void operator()(std::size_t i, T value) const {
    values_[i].v_int64 = static_cast<int64_t>(value);
    type_codes_[i] = kDLInt;
  }

 private:
  CVMValue* values_;
  int* type_codes_;
};

template <typename TObjectRef>
struct PackedFuncValueConverter {
  static TObjectRef From(const CVMArgValue& val) { return val.template AsObjectRef<TObjectRef>(); }
  static TObjectRef From(const CVMRetValue& val) { return val.template AsObjectRef<TObjectRef>(); }
};

namespace detail {

template <bool stop, std::size_t I, typename F>
struct for_each_dispatcher {
  template <typename T, typename... Args>
  static void run(const F& f, T&& value, Args&&... args) {
    f(I, std::forward<T>(value));
    for_each_dispatcher<sizeof...(Args) == 0, (I + 1), F>::run(f, std::forward<Args>(args)...);
  }
};

template <std::size_t I, typename F>
struct for_each_dispatcher<true, I, F> {
  static void run(const F& f) {}
};

template <typename F, typename... Args>
void for_each(const F& f, Args&&... args) {
  for_each_dispatcher<sizeof...(Args) == 0, 0, F>::run(f, std::forward<Args>(args)...);
}

template <typename T>
struct func_signature_helper {
  using FType = void;
};

template <typename T, typename R, typename... Args>
struct func_signature_helper<R (T::*)(Args...)> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};

template <typename T, typename R, typename... Args>
struct func_signature_helper<R (T::*)(Args...) const> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};

template <typename T>
struct function_signature {
  using FType = typename func_signature_helper<decltype(&T::operator())>::FType;
};

template <typename R, typename... Args>
struct function_signature<R(Args...)> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};

template <typename R, typename... Args>
struct function_signature<R(*)(Args...)> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};
}  // namespace detail

inline int CVMArgs::size() const { return num_args; }

inline CVMArgValue CVMArgs::operator[](int i) const {
  ICHECK_LT(i, num_args) << "not enough argument passed, " << num_args << " passed"
                         << " but request arg[" << i << "].";
  return CVMArgValue(values[i], type_codes[i]);
}

template <typename... Args>
inline CVMRetValue PackedFunc::operator()(Args&&... args) {
  const int kNumArgs = sizeof...(Args);
  const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;
  CVMValue values[kArraySize];
  int type_codes[kArraySize];
  detail::for_each(CVMArgsSetter(values, type_codes), std::forward<Args>(args)...);
  CVMRetValue rv;
  body_(CVMArgs(values, type_codes, kNumArgs), &rv);
  return rv;
}

void PackedFunc::CallPacked(CVMArgs args, CVMRetValue* rv) const { body_(args, rv); }

PackedFunc::FType PackedFunc::body() const { return body_; }

namespace detail {

template <typename R, int nleft, int index, typename F>
struct unpack_call_dispatcher {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpack_args) {
    unpack_call_dispatcher<R, nleft - 1, index + 1, F>::run(
        optional_name, f, args_pack, rv, std::forward<Args>(unpack_args)...,
        CVMMovableArgValueWithContext_(args_pack.values[index], args_pack.type_codes[index], index,
                                       optional_name));
  }
};

template <typename R, int index, typename F>
struct unpack_call_dispatcher<R, 0, index, F> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpack_args) {
    using RetType = decltype(f(std::forward<Args>(unpack_args)...));
    if (std::is_same<RetType, R>::value) {
      *rv = f(std::forward<Args>(unpack_args)...);
    } else {
      *rv = R(f(std::forward<Args>(unpack_args)...));
    }
  }
};

template <int index, typename F>
struct unpack_call_dispatcher<void, 0, index, F> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpacked_args) {
    f(std::forward<Args>(unpacked_args)...);
  }
};

template <typename R, int nargs, typename F>
CVM_ALWAYS_INLINE void unpack_call(const std::string* optional_name, const F& f,
                                   const CVMArgs& args, CVMRetValue* rv) {
  ICHECK_EQ(nargs, args.size()) << "Function "
                                << (optional_name == nullptr ? "<anonymous>" : *optional_name)
                                << " expects " << nargs << " arguments but " << args.size()
                                << " were provided";
  unpack_call_dispatcher<R, nargs, 0, F>::run(optional_name, f, args, rv);
}

template <typename R>
struct typed_packed_call_dispatcher {
  template <typename... Args>
  CVM_ALWAYS_INLINE static R run(const PackedFunc& pf, Args&&... args) {
    return pf(std::forward<Args>(args)...);
  }
};

template <>
struct typed_packed_call_dispatcher<void> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const PackedFunc& pf, Args&&... args) {
    pf(std::forward<Args>(args)...);
  }
};
}  // namespace detail

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(PackedFunc packed) : packed_(std::move(packed)) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const CVMRetValue& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const CVMArgValue& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(CVMMovableArgValueWithContext_&& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
template <typename FType>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda, std::string name) {
  packed_ = PackedFunc([flambda, name](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function " << name << " expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided.";
    }
    detail::unpack_call<R, sizeof...(Args)>(&name, flambda, args, rv);
  });
}

template <typename R, typename... Args>
template <typename FType>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda) {
  packed_ = PackedFunc([flambda](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function <anonymous> expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided.";
    }
    detail::unpack_call<R, sizeof...(Args)>(nullptr, flambda, args, rv);
  });
}

template <typename R, typename... Args>
CVM_ALWAYS_INLINE R TypedPackedFunc<R(Args...)>::operator()(Args&&... args) const {
  return detail::typed_packed_call_dispatcher<R>::run(packed_, std::forward<Args>(args)...);
}

template <typename TObjectRef, typename>
inline bool CVMPODValue_::IsObjectRef() const {
  using ContainerType = typename TObjectRef::ContainerType;
  if (std::is_base_of<NDArray::ContainerType, ContainerType>::value) {
    return type_code_ == kCVMNDArrayHandle &&
           CVMArrayHandleToObjectHandle(static_cast<CVMArrayHandle>(value_.v_handle))
               ->template IsInstance<ContainerType>();
  }
  if (std::is_base_of<Module::ContainerType, ContainerType>::value) {
    return type_code_ == kCVMModuleHandle &&
           static_cast<Object*>(value_.v_handle)->template IsInstance<ContainerType>();
  }
  return (std::is_base_of<ContainerType, NDArray::ContainerType>::value &&
          type_code_ == kCVMNDArrayHandle) ||
         (std::is_base_of<ContainerType, Module::ContainerType>::value &&
          type_code_ == kCVMModuleHandle) ||
         (type_code_ == kCVMObjectHandle &&
          ObjectTypeChecker<TObjectRef>::Check(static_cast<Object*>(value_.v_handle)));
}

template <typename TObjectRef>
inline TObjectRef CVMPODValue_::AsObjectRef() const {
  static_assert(std::is_base_of<ObjectRef, TObjectRef>::value,
                "Conversion only works for ObjectRef");
  using ContainerType = typename TObjectRef::ContainerType;

  if (type_code_ == kCVMNullptr) {
    ICHECK(TObjectRef::_type_is_nullable)
        << "Expect a not null value of " << ContainerType::_type_key;
    return TObjectRef(ObjectPtr<Object>(nullptr));
  }
  // NOTE: the following code can be optimized by constant folding.
  if (std::is_base_of<NDArray::ContainerType, ContainerType>::value) {
    // Casting to a sub-class of NDArray
    CVM_CHECK_TYPE_CODE(type_code_, kCVMNDArrayHandle);
    ObjectPtr<Object> data =
        NDArray::FFIDataFromHandle(static_cast<CVMArrayHandle>(value_.v_handle));
    ICHECK(data->template IsInstance<ContainerType>())
        << "Expected " << ContainerType::_type_key << " but got " << data->GetTypeKey();
    return TObjectRef(data);
  }
  if (std::is_base_of<Module::ContainerType, ContainerType>::value) {
    CVM_CHECK_TYPE_CODE(type_code_, kCVMModuleHandle);
    ObjectPtr<Object> data = GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle));
    ICHECK(data->template IsInstance<ContainerType>())
        << "Expected " << ContainerType ::_type_key << " but got " << data->GetTypeKey();
    return TObjectRef(data);
  }
  if (type_code_ == kCVMObjectHandle) {
    // normal object type check.
    Object* ptr = static_cast<Object*>(value_.v_handle);
    Optional<String> checked_type = ObjectTypeChecker<TObjectRef>::CheckAndGetMisMatch(ptr);
    ICHECK(!checked_type.defined()) << "Expected " << ObjectTypeChecker<TObjectRef>::TypeName
                                    << ", but got " << checked_type.value();
    return TObjectRef(GetObjectPtr<Object>(ptr));
  } else if (type_code_ == kCVMObjectRValueRefArg) {
    Object* ptr = *static_cast<Object**>(value_.v_handle);
    Optional<String> checked_type = ObjectTypeChecker<TObjectRef>::CheckAndGetMisMatch(ptr);
    ICHECK(!checked_type.defined()) << "Expected " << ObjectTypeChecker<TObjectRef>::TypeName()
                                    << ", but got " << checked_type.value();
    return TObjectRef(GetObjectPtr<Object>(ptr));
  } else if (std::is_base_of<ContainerType, NDArray::ContainerType>::value &&
             type_code_ == kCVMNDArrayHandle) {
    // Casting to a base class that NDArray can sub-class
    ObjectPtr<Object> data =
        NDArray::FFIDataFromHandle(static_cast<CVMArrayHandle>(value_.v_handle));
    return TObjectRef(data);
  } else if (std::is_base_of<ContainerType, Module::ContainerType>::value &&
             type_code_ == kCVMModuleHandle) {
    // Casting to a base class that Module can sub-class
    return TObjectRef(GetObjectPtr<Object>(static_cast<Object*>(value_.v_handle)));
  } else {
    CVM_CHECK_TYPE_CODE(type_code_, kCVMObjectHandle);
    return TObjectRef(ObjectPtr<Object>(nullptr));
  }
}

template <typename T, typename>
inline CVMArgValue::operator T() const {
  return PackedFuncValueConverter<T>::From(*this);
}

bool String::CanConvertFrom(const CVMArgValue& val) {
  return val.type_code() == kCVMStr || val.IsObjectRef<cvm::runtime::String>();
}

inline CVMArgValue::operator DLDataType() const {
  if (String::CanConvertFrom(*this)) {
    return String2DLDataType(PackedFuncValueConverter<String>::From(*this).operator std::string());
  }
  // None type
  if (type_code_ == kCVMNullptr) {
    DLDataType t;
    t.code = kCVMOpaqueHandle;
    t.bits = 0;
    t.lanes = 0;
    return t;
  }
  CVM_CHECK_TYPE_CODE(type_code_, kCVMDataType);
  return value_.v_type;
};

inline CVMArgValue::operator DataType() const { return DataType(operator DLDataType()); }

template <typename T, typename>
inline CVMMovableArgValue_::operator T() const {
  if (type_code_ == kCVMObjectRValueRefArg) {
    auto** ref = static_cast<Object**>(value_.v_handle);
    if (ObjectTypeChecker<T>::Check(*ref)) {
      return T(ObjectPtr<Object>::MoveFromRValueRefArg(ref));
    }
  }
  // fallback
  return PackedFuncValueConverter<T>::From(AsArgValue());
}

template <typename TObjectRef, typename>
inline CVMRetValue& CVMRetValue::operator=(TObjectRef other) {
  using ContainerType = typename TObjectRef::ContainerType;
  const Object* ptr = other.get();
  if (ptr != nullptr) {
    if (std::is_base_of<NDArray::ContainerType, ContainerType>::value ||
        (std::is_base_of<ContainerType, NDArray::ContainerType>::value &&
         ptr->template IsInstance<NDArray::ContainerType>())) {
      return operator=(NDArray(std::move(other.data_)));
    }
    if (std::is_base_of<Module::ContainerType, ContainerType>::value ||
        (std::is_base_of<ContainerType, Module::ContainerType>::value &&
         ptr->template IsInstance<Module::ContainerType>())) {
      return operator=(Module(std::move(other.data_)));
    }
    SwitchToObject(kCVMObjectHandle, std::move(other.data_));
  } else {
    SwitchToPOD(kCVMNullptr);
  }
  return *this;
}

template <typename T, typename>
inline CVMRetValue::operator T() const {
  return PackedFuncValueConverter<T>::From(*this);
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
