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
class CVMMoveableArgValueWithContext_;
class CVMRetValue;
class CVMArgsSetter;

class PackedFunc {
 public:
  using FType = std::function<void(CVMArgs args, CVMRetValue* rv)>;

  PackedFunc() = default;

  PackedFunc(std::nullptr_t null) {}

  explicit PackedFunc(FType body) : body_(body) {}

  template <typename... Args>
  inline CVMRetValue operator()(Args&&... args) const;

  inline void CallPacked(CVMArgs args, CVMRetValue* rv) const;

  inline FType body() const;

  bool operator==(std::nullptr_t null) const { return body_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return body_ != nullptr; }

 private:
  FType body_;
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

class CVMPODValue_ {
 public:
  operator double() const {
    if (type_code_ == kDLInt) {
      return static_cast<double>(value_.v_int64);
    }
    CVM_CHECK_TYPE_CODE(type_code_, kDLFloat);
    return value_.v_float64;
  }
  operator int64_t() const {
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64;
  }
  operator uint64_t() const {
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64;
  }
  operator int() const {
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    ICHECK_LE(value_.v_int64, std::numeric_limits<int>::max());
    ICHECK_GE(value_.v_int64, std::numeric_limits<int>::min());
    return static_cast<int>(value_.v_int64);
  }
  operator bool() const {
    CVM_CHECK_TYPE_CODE(type_code_, kDLInt);
    return value_.v_int64 != 0;
  }
  operator void*() const {
    if (type_code_ == kCVMNullptr) return nullptr;
    if (type_code_ == kCVMDLTensorHandle) return value_.v_handle;
    CVM_CHECK_TYPE_CODE(type_code_, kCVMOpaqueHandle);
    return value_.v_handle;
  }
  operator DLTensor*() const {
    if (type_code_ == kCVMDLTensorHandle || type_code_ == kCVMNDArrayHandle) {
      return static_cast<DLTensor*>(value_.v_handle);
    } else {
      if (type_code_ == kCVMNullptr) return nullptr;
      LOG(FATAL) << "Expected "
                 << "DLTensor* or NDArray but got " << ArgTypeCode2Str(type_code_);
      return nullptr;
    }
  }
  operator NDArray() const {
    if (type_code_ == kCVMNullptr) return NDArray(ObjectPtr<Object>(nullptr));
    CVM_CHECK_TYPE_CODE(type_code_, kCVMNDArrayHandle);
    return NDArray();
  }

  template <typename T>
  T* ptr() const {
    return static_cast<T*>(value_.v_handle);
  }

  int type_code() const { return type_code_; }

 protected:
  CVMPODValue_() : type_code_(kCVMNullptr) {}
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

  operator PackedFunc() const {
    if (type_code_ == kCVMNullptr) return PackedFunc();
    CVM_CHECK_TYPE_CODE(type_code_, kCVMPackedFuncHandle);
    return *ptr<PackedFunc>();
  }
};

class CVMRetValue : public CVMPODValue_ {
 public:
  CVMRetValue() = default;

  CVMRetValue(CVMRetValue&& other) : CVMPODValue_(other.value_, other.type_code_) {
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

  CVMRetValue(const CVMRetValue& other) : CVMPODValue_() { this->Assign(other); }

  operator std::string() const {
    if (type_code_ == kCVMDataType) {
      return DLDataType2String(operator DLDataType());
    }
  }
  operator DLDataType() const {
    if (type_code_ == kCVMStr) {
      
    }
  }
  operator PackedFunc() const {}

  CVMRetValue& operator=(NDArray other) {}

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
