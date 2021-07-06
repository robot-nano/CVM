//
// Created by WangJingYu on 2021/7/6.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_DATA_TYPE_H_
#define CVM_INCLUDE_CVM_RUNTIME_DATA_TYPE_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/logging.h>

namespace cvm {
namespace runtime {

class DataType {
 public:
  enum TypeCode {
    kInt = kDLInt,
    kUInt = kDLUInt,
    kFloat = kDLFloat,
    kHandle = CVMArgTypeCode::kCVMOpaqueHandle,
    kBFloat = kDLBfloat,
    kCustomBegin = 129
  };
  DataType() = default;

  explicit DataType(DLDataType dtype) : data_(dtype) {}

  DataType(int code, int bits, int lanes) {
    data_.code = static_cast<uint8_t>(code);
    data_.bits = static_cast<uint8_t>(bits);
    data_.lanes = static_cast<uint16_t>(lanes);
    if (code == kBFloat) {
      ICHECK_EQ(bits, 16);
    }
  }

  int code() const { return static_cast<int>(data_.code); }
  int bits() const { return static_cast<int>(data_.bits); }
  int bytes() const { return (bits() + 7) / 8; }
  int lanes() const { return static_cast<int>(data_.lanes); }
  bool is_scalar() const { return lanes() == 1; }
  bool is_bool() const { return code() == DataType::kUInt && bits() == 1; }
  bool is_float() const { return code() == DataType::kFloat; }
  bool is_float16() const { return is_float() && bits() == 16; }
  bool is_bfloat16() const { return code() == kBFloat && bits() == 16; }
  bool is_int() const { return code() == kInt; }
  bool is_uint() const { return code() == kUInt; }
  bool is_handle() const { return code() == kHandle && !is_void(); }
  bool is_vector() const { return lanes() > 1; }
  bool is_vector_bool() const { return is_vector() && bits() == 1; }
  bool is_void() const { return code() == kHandle && bits() == 0 && lanes() == 0; }

  DataType with_lanes(int lanes) const { return DataType(data_.code, data_.bits, lanes); }

  DataType with_bits(int bits) const { return DataType(data_.code, bits, data_.lanes); }

  DataType element_of() const { return with_lanes(1); }

  bool operator==(const DataType& other) const {
    return data_.code == other.data_.code && data_.bits == other.data_.bits &&
           data_.lanes == other.data_.lanes;
  }

  bool operator!=(const DataType& other) const { return !operator==(other); }

  operator DLDataType() const { return data_; }

  static DataType Int(int bits, int lanes = 1) { return DataType(kDLInt, bits, lanes); }

  static DataType UInt(int bits, int lanes = 1) { return DataType(kDLUInt, bits, lanes); }

  static DataType Float(int bits, int lanes = 1) { return DataType(kDLFloat, bits, lanes); }

  static DataType BFloat(int bits, int lanes = 1) { return DataType(kDLBfloat, bits, lanes); }

  static DataType Bool(int lanes = 1) { return DataType::UInt(1, lanes); }

  static DataType Handle(int bits = 64, int lanes = 1) { return DataType(kHandle, bits, lanes); }

  static DataType Void() { return DataType(kHandle, 0, 0); }

  static DataType ShapeIndex() {
    if (std::is_signed<cvm_index_t>::value) {
      return DataType::Int(sizeof(cvm_index_t) * 8);
    } else {
      return DataType::UInt(sizeof(cvm_index_t) * 8);
    }
  }

 private:
  DLDataType data_;
};

CVM_DLL std::string GetCustomTypeName(uint8_t type_code);

CVM_DLL uint8_t ParseCustomDataType(const std::string& s, const char** scan);

inline const char* DLDataTypeCode2Str(DLDataTypeCode type_code);

inline DLDataType String2DLDataType(const std::string& s);

inline const char* DLDataTypeCode2Str(DLDataTypeCode type_code) {
  switch (static_cast<int>(type_code)) {
    case kDLInt:
      return "int";
    case kDLUInt:
      return "uint";
    case kDLFloat:
      return "float";
    case DataType::kHandle:
      return "handle";
    case kDLBfloat:
      return "bfloat";
    default:
      LOG(FATAL) << "unknown type_code=" << static_cast<int>(type_code);
      return "";
  }
}

inline std::ostream& operator<<(std::ostream& os, DLDataType t) {
  if (t.bits == 1 && t.lanes == 1 && t.code == kDLUInt) {
    os << "bool";
    return os;
  }
  if (DataType(t).is_void()) {
    return os << "void";
  }
  if (t.code < DataType::kCustomBegin) {
    os << DLDataTypeCode2Str(static_cast<DLDataTypeCode>(t.code));
  } else {
    os << "custom[" << GetCustomTypeName(t.code) << "]";
  }
  if (t.code == kCVMOpaqueHandle) return os;
  os << static_cast<int>(t.bits);
  if (t.lanes != 1) {
    os << 'x' << static_cast<int>(t.lanes);
  }
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const DataType& dtype) {
  return os << dtype.operator DLDataType();
}

inline std::string DLDataType2String(DLDataType t) {
  if (t.bits == 0) return "";
  std::ostringstream os;
  os << t;
  return os.str();
}

inline DLDataType String2DLDataType(const std::string& s) {
  DLDataType t;
  // handle void type
  if (s.length() == 0) {
    t = DataType::Void();
    return t;
  }
  t.bits = 32;
  t.lanes = 1;
  const char* scan;
  if (s.substr(0, 3) == "int") {
    t.code = kDLInt;
    scan = s.c_str() + 3;
  } else if (s.substr(0, 4) == "uint") {
    t.code = kDLUInt;
    scan = s.c_str() + 4;
  } else if (s.substr(0, 5) == "float") {
    t.code = kDLFloat;
    scan = s.c_str() + 5;
  } else if (s.substr(0, 6) == "handle") {
    t.code = kCVMOpaqueHandle;
    t.bits = 64;  // handle uses 64 bit by default.
    scan = s.c_str() + 6;
  } else if (s == "bool") {
    t.code = kDLUInt;
    t.bits = 1;
    t.lanes = 1;
    return t;
  } else if (s.substr(0, 6) == "bfloat") {
    t.code = DataType::kBFloat;
    scan = s.c_str() + 6;
  } else if (s.substr(0, 6) == "custom ") {
    t.code = ParseCustomDataType(s, &scan);
  } else {
    scan = s.c_str();
    LOG(FATAL) << "unknown type " << s;
  }
  char* xdelim;  // emulate sscanf("%ux%u", bits, lanes)
  uint8_t bits = static_cast<uint8_t>(strtoul(scan, &xdelim, 10));
  if (bits != 0) t.bits = bits;
  char* endpt = xdelim;
  if (*xdelim == 'x') {
    t.lanes = static_cast<uint16_t>(strtoul(xdelim + 1, &endpt, 10));
  }
  ICHECK(endpt == s.c_str() + s.length()) << "unknown type " << s;
  return t;
}

}  // namespace runtime

using DataType = runtime::DataType;

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_DATA_TYPE_H_
