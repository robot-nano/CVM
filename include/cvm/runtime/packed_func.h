//
// Created by WangJingYu on 2021/7/5.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
#define CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/data_type.h>
#include <cvm/runtime/ndarray.h>

#include <functional>
#include <limits>

namespace cvm {
namespace runtime {

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
  inline CVMRetValue operator()(Args&&... args) const;

  inline void CallPacked(CVMArgs args, CVMRetValue* rv) const;

  inline FType body() const;

  bool operator==(std::nullptr_t null) const { return body_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return body_ != nullptr; }

 private:
  FType body_;
};

template <typename FType>
class TypedPackedFunc {};

template <typename R, typename... Args>
class TypedPackedFunc<R(Args...)> {
 public:
  using TSelf = TypedPackedFunc<R(Args...)>;

  TypedPackedFunc() = default;

  TypedPackedFunc(std::nullptr_t null) {}  // NOLINT(*)

  inline TypedPackedFunc(PackedFunc packed);  // NOLINT

  inline TypedPackedFunc(const CVMRetValue& value);  // NOLINT

  inline TypedPackedFunc(const CVMArgValue& value);  // NOLINT

  inline TypedPackedFunc(CVMMovableArgValueWithContext_&& value);  // NOLINT

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda, std::string name) {
    this->AssignTypedLambda(typed_lambda, std::move(name));
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda) {  // NOLINT
    this->AssignTypedLambda(typed_lambda);
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TSelf& operator=(FLambda typed_lambda) {  // NOLINT
    this->AssignTypedLambda(typed_lambda);
    return *this;
  }

  TSelf& operator=(PackedFunc packed) {  // NOLINT
    packed_ = std::move(packed);
    return *this;
  }

  CVM_ALWAYS_INLINE R operator()(Args... args) const;

  operator PackedFunc() const { return packed(); }  // NOLINT

  const PackedFunc& packed() const { return packed_; }

 private:
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

#define CVM_CHECK_TYPE_CODE(CODE, T) \
  ICHECK_EQ(CODE, T) << "expected " << ArgTypeCode2Str(T) << " but got " << ArgTypeCode2Str(CODE)

template <typename T>
struct ObjectTypeChecker {};

template <typename T>
struct ObjectTypeChecker<Array<T>> {};

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
    return NDArray();
  }

  template <typename T>
  T* ptr() const {
    return static_cast<T*>(value_.v_handle);
  }

  int type_code() const { return type_code_; }

  template <typename TObjectRef,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  inline bool IsObjectRef() const;

  template <typename TObjectRef>
  inline TObjectRef AsObjectRef() const;

 protected:
  friend class CVMArgsSetter;
  friend class CVMRetValue;
  friend class CVMMovableArgValueWithContext_;
  CVMPODValue_() : type_code_(kCVMNullptr) {}  // NOLINT
  CVMPODValue_(CVMValue value, int type_code) : value_(value), type_code_(type_code) {}
  CVMValue value_;
  int type_code_;
};

class CVMArgValue : public CVMPODValue_ {
 public:
  CVMArgValue() = default;

  CVMArgValue(CVMValue value, int type_code) : CVMPODValue_(value, type_code) {}
  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator NDArray;

  operator std::string() const {  // NOLINT
    if (type_code_ == kCVMDataType) {
      return DLDataType2String(operator DLDataType());
    } else if (type_code_ == kCVMBytes) {
      CVMByteArray* arr = static_cast<CVMByteArray*>(value_.v_handle);
      return std::string(value_.v_str);
    } else if (type_code_ == kCVMStr) {
      return std::string(value_.v_str);
    } else {
      ICHECK(IsObjectRef<cvm::runtime::String>())
          << "Could not convert CVM object to type " << runtime::Object::TypeIndex2Key(type_code_)
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
  inline operator T() const;
  inline operator DLDataType() const;
  inline operator DataType() const;
};

class CVMMovableArgValue_ : public CVMPODValue_ {
 public:
  CVMMovableArgValue_(CVMValue value, int type_code) : CVMPODValue_(value, type_code) {}

  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator NDArray;
  //  using CVMPODValue_::operator Device ;

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
  inline operator T() const;

 private:
  CVMArgValue AsArgValue() const { return {value_, type_code_}; }
};

class CVMMovableArgValueWithContext_ {
 public:
  CVMMovableArgValueWithContext_(CVMValue value, int type_code, int arg_index,
                                 const std::string* optional_name)
      : value_(value, type_code), arg_index_(arg_index), optional_name_(optional_name) {}

  template <typename T>
  operator T() const {
    // TODO:
  }

 private:
  CVMMovableArgValue_ value_;
  int arg_index_;
  const std::string* optional_name_;
};

class CVMRetValue : public CVMPODValue_ {
 public:
  CVMRetValue() = default;

  CVMRetValue(CVMRetValue&& other) : CVMPODValue_(other.value_, other.type_code_) {  // NOLINT
    other.value_.v_handle = nullptr;
    other.type_code_ = kCVMNullptr;
  }
  ~CVMRetValue() { this->Clear(); }

  using CVMPODValue_::operator double;
  using CVMPODValue_::operator int64_t;
  using CVMPODValue_::operator uint64_t;
  using CVMPODValue_::operator int;
  using CVMPODValue_::operator bool;
  using CVMPODValue_::operator void*;
  using CVMPODValue_::operator DLTensor*;
  using CVMPODValue_::operator NDArray;

  CVMRetValue(const CVMRetValue& other) : CVMPODValue_() { this->Assign(other); }  // NOLINT

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
  };
  // Assign operators
  CVMRetValue& operator=(CVMRetValue&& other) {  // NOLINT
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
  CVMRetValue& operator=(DLDataType value) {
    this->SwitchToPOD(kCVMDataType);
    value_.v_type = value;
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
    this->SwitchToClass(kCVMBytes, std::string(value.data, value.size));
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
  // TODO
  CVMRetValue& operator=(PackedFunc f) {
    if (f == nullptr) {
      this->SwitchToPOD(kCVMNullptr);
    } else {
      this->SwitchToClass(kCVMPackedFuncHandle, std::move(f));
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
   *  This marks the current container as null.
   *  The managed resources are moved to the front-end.
   *  The front end should take charge in managing them.
   *
   * \param ret_value The return value.
   * \param ret_type_code The return type code.
   */
  void MoveToCHost(CVMValue* ret_value, int* ret_type_code) {
    ICHECK(type_code_ != kCVMStr && type_code_ != kCVMBytes);
    *ret_value = value_;
    *ret_type_code = type_code_;
    type_code_ = kCVMNullptr;
  }
  /*! \brief The value field, if the data is POD */
  static CVMRetValue MoveFromCHost(CVMValue value, int type_code) {
    ICHECK(type_code <= kCVMPackedFuncHandle || type_code == kCVMNDArrayHandle);
    CVMRetValue ret;
    ret.value_ = value;
    ret.type_code_ = type_code;
    return ret;
  }

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
      case kCVMNDArrayHandle: {
        *this = other.operator NDArray();
        break;
      }
      case kCVMObjectHandle: {
        SwitchToObject(kCVMObjectHandle,
                       GetObjectPtr<Object>(static_cast<Object*>(other.value_.v_handle)));
        break;
      }
      case kCVMObjectRValueRefArg: {
        //        operator=(other.operator ObjectRef());
        //        break;
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
        NDArray::FFIDecRef(static_cast<CVMArrayHandle>(value_.v_handle));
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

template <typename TObjectRef>
struct PackedFuncValueConverter {
  static TObjectRef From(const CVMArgValue& val) { return val.AsObjectRef<TObjectRef>(); }

  static TObjectRef From(const CVMRetValue& val) { return val.AsObjectRef<TObjectRef>(); }
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

template <size_t I, typename F>
struct for_each_dispatcher<true, I, F> {
  static void run(const F& f) {}
};

template <typename F, typename... Args>
inline void for_each(const F& f, Args&&... args) {
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
  using FType = typename func_signature_helper<decltype(&T::operator())>::type;
};

template <typename R, typename... Args>
struct func_signature_helper<R(Args...)> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};

template <typename R, typename... Args>
struct func_signature_helper<R (*)(Args...)> {
  using FType = R(Args...);
  static_assert(!std::is_reference<R>::value, "TypedPackedFunc return reference");
};

}  // namespace detail

class CVMArgsSetter {
 public:
  CVMArgsSetter(CVMValue* values, int* type_codes) : values_(values), type_codes_(type_codes) {}

  template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
  CVM_ALWAYS_INLINE void operator()(size_t i, T value) const {
    values_[i].v_int64 = static_cast<int64_t>(value);
    type_codes_[i] = kDLInt;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, uint64_t value) const {
    values_[i].v_int64 = static_cast<int64_t>(value);
    ICHECK_LE(value, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    type_codes_[i] = kDLInt;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, double value) const {
    values_[i].v_float64 = value;
    type_codes_[i] = kDLFloat;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, std::nullptr_t value) const {
    values_[i].v_handle = value;
    type_codes_[i] = kCVMNullptr;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, const CVMArgValue& value) const {
    values_[i] = value.value_;
    type_codes_[i] = kCVMOpaqueHandle;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, void* value) const {
    values_[i].v_handle = value;
    type_codes_[i] = kCVMOpaqueHandle;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, DLTensor* value) const {
    values_[i].v_handle = value;
    type_codes_[i] = kCVMDLTensorHandle;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, Device device) const {
    values_[i].v_device = device;
    type_codes_[i] = kDLDevice;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, DLDataType value) const {
    values_[i].v_type = value;
    type_codes_[i] = kCVMDataType;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, DataType value) const {
    operator()(i, value.operator DLDataType());
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, const char* value) const {
    values_[i].v_str = value;
    type_codes_[i] = kCVMStr;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, const std::string& value) const {
    values_[i].v_str = value.c_str();
    type_codes_[i] = kCVMStr;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, const CVMByteArray& value) const {
    values_[i].v_handle = const_cast<CVMByteArray*>(&value);
    type_codes_[i] = kCVMBytes;
  }
  CVM_ALWAYS_INLINE void operator()(size_t i, const PackedFunc& value) const {
    if (value != nullptr) {
      values_[i].v_handle = const_cast<PackedFunc*>(&value);
      type_codes_[i] = kCVMPackedFuncHandle;
    } else {
      values_[i].v_handle = nullptr;
      type_codes_[i] = kCVMNullptr;
    }
  }
  template <typename FType>
  CVM_ALWAYS_INLINE void operator()(size_t i, const TypedPackedFunc<FType>& value) const {
    // TODO
  }
  void operator()(size_t i, const CVMRetValue& value) const {
    if (value.type_code() == kCVMStr) {
      values_[i].v_str = value.ptr<std::string>()->c_str();
      type_codes_[i] = kCVMStr;
    } else {
      ICHECK_NE(value.type_code(), kCVMBytes) << "not enabled";
      values_[i] = value.value_;
      type_codes_[i] = value.type_code();
    }
  }
  template <typename TObjectRef,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  CVM_ALWAYS_INLINE void operator()(size_t i, const TObjectRef& value) const {
    this->template SetObjectRef(i, value);
  }
  template <typename TObjectRef,
            typename = typename std::enable_if<std::is_base_of<
                ObjectRef, typename std::remove_reference<TObjectRef>::type>::value>::type>
  CVM_ALWAYS_INLINE void operator()(size_t i, TObjectRef&& value) const {
    this->template SetObjectRef(i, std::forward<TObjectRef>(value));
  }

 private:
  template <typename TObjectRef>
  inline void SetObjectRef(size_t i, TObjectRef&& value) const;
  CVMValue* values_;
  int* type_codes_;
};

template <typename... Args>
inline CVMRetValue PackedFunc::operator()(Args&&... args) const {
  const int kNumArgs = sizeof...(Args);
  const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;
  CVMValue values[kArraySize];
  int type_codes[kArraySize];
  detail::for_each(CVMArgsSetter(values, type_codes), std::forward<Args>(args)...);
  CVMRetValue rv;
  body_(CVMArgs(values, type_codes, kNumArgs), &rv);
  return rv;
}

inline int CVMArgs::size() const { return num_args; }

inline CVMArgValue CVMArgs::operator[](int i) const {
  ICHECK_LT(i, num_args) << "not enough argument passed, " << num_args << " passed"
                         << " but request arg[" << i << "]";
  return {values[i], type_codes[i]};
}

inline void PackedFunc::CallPacked(CVMArgs args, CVMRetValue* rv) const { body_(args, rv); }

inline PackedFunc::FType PackedFunc::body() const { return body_; }

namespace detail {

template <typename R, int nleft, int index, typename F>
struct unpack_call_dispatcher {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpacked_args) {
    unpack_call_dispatcher<R, nleft - 1, index + 1, F>::run(
        optional_name, f, args_pack, rv, std::forward<Args>(unpacked_args)...,
        CVMMovableArgValueWithContext_(args_pack.values[index], args_pack.type_codes[index], index,
                                       optional_name));
  }
};

template <typename R, int index, typename F>
struct unpack_call_dispatcher<R, 0, index, F> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpacked_args) {
    using RetType = decltype(f(std::forward<Args>(unpacked_args)...));
    if (std::is_same<RetType, R>::value) {
      *rv = f(std::forward<Args>(unpacked_args)...);
    } else {
      *rv = R(f(std::forward<Args>(unpacked_args)...));
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
CVM_ALWAYS_INLINE R TypedPackedFunc<R(Args...)>::operator()(Args... args) const {
  return detail::typed_packed_call_dispatcher<R>::run(packed_, std::forward<Args>(args)...);
}

template <typename R, typename... Args>
template <typename FLambda>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FLambda flambda, std::string name) {
  packed_ = PackedFunc([flambda, name](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function " << name << " expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided.";
    }
    detail::unpack_call<R, sizeof...(Args)>(&name, flambda, args, rv);
  });
}

template <typename R, typename... Args>
template <typename FLambda>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FLambda flambda) {
  packed_ = PackedFunc([flambda](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function <anonymous> expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided. ";
    }
    detail::unpack_call<R, sizeof...(Args)>(nullptr, flambda, args, rv);
  });
}

template <typename TObjectRef, typename>
inline bool CVMPODValue_::IsObjectRef() const {
  using ContainerType = typename TObjectRef::ContainerType;
  if (std::is_base_of<NDArray::ContainerType, ContainerType>::value) {
    return type_code_ == kCVMNDArrayHandle &&
           CVMArrayHandleToObjectHandle(static_cast<CVMArrayHandle>(value_.v_handle))
               ->IsInstance<ContainerType>();
  }
  // TODO
  // TODO
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
  // TODO
}

template <typename T, typename>
inline CVMArgValue::operator T() const {
  return PackedFuncValueConverter<T>::From(*this);
}

inline bool String::CanConvertFrom(const CVMArgValue& val) {
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
}

inline CVMArgValue::operator DataType() const { return DataType(operator DLDataType()); }

template <typename T, typename>
inline CVMMovableArgValue_::operator T() const {
  if (type_code_ == kCVMObjectRValueRefArg) {
    auto **ref = static_cast<Object**>(value_.v_handle);
    if (ObjectTypeChecker<T>::Check(*ref)) {
      return T(ObjectPtr<Object>::MoveFromRValueRefArg(ref));
    }
  }
  // fallback
  return PackedFuncValueConverter<T>::From(AsArgValue());
}

inline const char* ArgTypeCode2Str(int type_code) {
  switch (type_code) {
    case kDLInt:
      return "int";
    case kDLUInt:
      return "uint";
    case kDLFloat:
      return "float";
    case kCVMStr:
      return "str";
    case kCVMBytes:
      return "bytes";
    case kCVMOpaqueHandle:
      return "handle";
    case kCVMNullptr:
      return "NULL";
    case kCVMDLTensorHandle:
      return "ArrayHandle";
    case kCVMDataType:
      return "DLDataType";
    case kDLDevice:
      return "DLDevice";
    case kCVMPackedFuncHandle:
      return "FunctionHandle";
    case kCVMModuleHandle:
      return "ModuleHandle";
    case kCVMNDArrayHandle:
      return "NDArrayContainer";
    case kCVMObjectHandle:
      return "Object";
    case kCVMObjectRValueRefArg:
      return "ObjectRValueRefArg";
    default:
      LOG(FATAL) << "unknown type_code=" << static_cast<int>(type_code);
      return "";
  }
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
