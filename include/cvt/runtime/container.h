//
// Created by WangJingYu on 2021/1/12.
//

#ifndef CVT_CONTAINER_H
#define CVT_CONTAINER_H

#include <cvt/runtime/memory.h>
#include <cvt/runtime/object.h>

#include <cstring>
#include <string>

namespace cvt {
namespace runtime {

struct ObjectHash {
  size_t operator()(const ObjectRef& a) const;
};

struct ObjectEqual {
  bool operator()(const ObjectRef& a, const ObjectRef& b) const;
};

class StringObj : public Object {
 public:
  const char* data;

  uint64_t size;

  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "runtime.String";
//  ///////////////////////////////////////
  //TODO:
//  static const constexpr bool _type_final = true;
//  static const constexpr int _type_child_slots = 0;
//  static_assert(!Object::_type_final, "ParentObj marked as final");
//  static uint32_t RuntimeTypeIndex() {
//    static_assert(StringObj::_type_child_slots == 0 || Object::_type_child_slots == 0 ||
//                  StringObj::_type_child_slots < Object::_type_child_slots_can_overflow,
//                  "Need to set _type_child_slots when parent specifices it");
//    if (StringObj::_type_index != ::cvt::runtime::TypeIndex::kDynamic) {
//      return StringObj::_type_index;
//    }
//    return _GetOrAllocRuntimeTypeIndex();
//  }
//  static uint32_t _GetOrAllocRuntimeTypeIndex() {
//    static uint32_t tidx = Object::GetOrAllocRuntimeTypeIndex(
//        StringObj::_type_key, StringObj::_type_index, Object::_GetOrAllocRuntimeTypeIndex(),
//        StringObj::_type_child_slots, StringObj::_type_child_slots_can_overflow);
//    return tidx;
//  }
  CVT_DECLARE_FINAL_OBJECT_INFO(StringObj, Object);
//  CVT_DECLARE_BASE_OBJECT_INFO(StringObj, Object);

 private:
  class FromStd;

  friend class String;
};

class String : public ObjectRef {
 public:
  /*!
   * \brief Construct an empty string.
   */
  String() : String(std::string()) {}

  String(std::string other);  // NOLINT(*)

  String(const char* other)  // NOLINT(*)
      : String(std::string(other)) {}

  inline String& operator=(std::string other);

  inline String& operator=(const char* other);

  int compare(const String& other) const {
    return memcmp(data(), other.data(), size(), other.size());
  }

  int compare(const std::string& other) const {
    return memcmp(data(), other.data(), size(), other.size());
  }

  int compare(const char* other) const { return memcmp(data(), other, size(), std::strlen(other)); }

  const char* c_str() const { return get()->data; }

  size_t size() const {
    const auto* ptr = get();
    return ptr->size;
  }

  size_t length() const { return size(); }

  bool empty() const { return size() == 0; }

  char at(size_t pos) const {
    if (pos < size()) {
      return data()[pos];
    } else {
      throw std::out_of_range("cvt::String index out of bounds");
    }
  }

  const char* data() const { return get()->data; }

  operator std::string() const { return std::string(get()->data, size()); }

  static size_t HashBytes(const char* data, size_t size) {
#if CVT_USE_CXX17_STRING_VIEW_HASH
    return std::hash<std::string_view>()(std::string_view(data, size));
#elif CVT_USE_CXX14_STRING_VIEW_HASH
    return std::hash<std::experimental::string_view>()(std::experimental::string_view(data, size));
#else
    return std::hash<std::string>()(std::string(data, size));
#endif
  }

  CVT_DEFINE_NOTNULLABLE_OBJECT_REF_METHODS(String, ObjectRef, StringObj);

 private:
  static int memcmp(const char* lhs, const char* rhs, size_t lhs_count, size_t rhs_count);

  static String Concat(const char* lhs, size_t lhs_size, const char* rhs, size_t rhs_size) {
    std::string ret(lhs, lhs_size);
    ret.append(rhs, rhs_size);
    return String(ret);
  }

  // Overload + operator
  friend String operator+(const String& lhs, const String& rhs);
  friend String operator+(const String& lhs, const std::string& rhs);
  friend String operator+(const std::string& lhs, const String& rhs);
  friend String operator+(const String& lhs, const char* rhs);
  friend String operator+(const char* lhs, const String& rhs);
};

class StringObj::FromStd : public StringObj {
 public:
  explicit FromStd(std::string other) : data_container(std::move(other)) {}

 private:
  /*! \brief Container that holds the memory. */
  std::string data_container;

  friend class String;
};

inline String::String(std::string other) {
  auto ptr = make_object<StringObj::FromStd>(std::move(other));
  ptr->size = ptr->data_container.size();
  ptr->data = ptr->data_container.data();
  data_ = std::move(ptr);
}

inline String& String::operator=(std::string other) {
  String replace(std::move(other));
  data_.swap(replace.data_);
  return *this;
}

inline String& String::operator=(const char* other) { return operator=(std::string(other)); }

inline String operator+(const String& lhs, const String& rhs) {
  size_t lhs_size = lhs.size();
  size_t rhs_size = rhs.size();
  return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const String& lhs, const std::string& rhs) {
  size_t lhs_size = lhs.size();
  size_t rhs_size = rhs.size();
  return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const std::string& lhs, const String& rhs) {
  size_t lhs_size = lhs.size();
  size_t rhs_size = rhs.size();
  return String::Concat(lhs.data(), lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const char* lhs, const String& rhs) {
  size_t lhs_size = std::strlen(lhs);
  size_t rhs_size = rhs.size();
  return String::Concat(lhs, lhs_size, rhs.data(), rhs_size);
}

inline String operator+(const String& lhs, const char* rhs) {
  size_t lhs_size = lhs.size();
  size_t rhs_size = std::strlen(rhs);
  return String::Concat(lhs.data(), lhs_size, rhs, rhs_size);
}

// Overload < operator
inline bool operator<(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) < 0; }

inline bool operator<(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) > 0; }

inline bool operator<(const String& lhs, const String& rhs) { return lhs.compare(rhs) < 0; }

inline bool operator<(const String& lhs, const char* rhs) { return lhs.compare(rhs) < 0; }

inline bool operator<(const char* lhs, const String& rhs) { return rhs.compare(lhs) > 0; }

// Overload > operator
inline bool operator>(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) > 0; }

inline bool operator>(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) < 0; }

inline bool operator>(const String& lhs, const String& rhs) { return lhs.compare(rhs) > 0; }

inline bool operator>(const String& lhs, const char* rhs) { return lhs.compare(rhs) > 0; }

inline bool operator>(const char* lhs, const String& rhs) { return rhs.compare(lhs) < 0; }

// Overload  <= operator
inline bool operator<=(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) <= 0; }

inline bool operator<=(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) >= 0; }

inline bool operator<=(const String& lhs, const String& rhs) { return lhs.compare(rhs) <= 0; }

inline bool operator<=(const String& lhs, const char* rhs) { return lhs.compare(rhs) <= 0; }

inline bool operator<=(const char* lhs, const String& rhs) { return rhs.compare(lhs) >= 0; }

// Overload >= operator
inline bool operator>=(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) >= 0; }

inline bool operator>=(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) <= 0; }

inline bool operator>=(const String& lhs, const String& rhs) { return lhs.compare(rhs) >= 0; }

inline bool operator>=(const String& lhs, const char* rhs) { return lhs.compare(rhs) >= 0; }

inline bool operator>=(const char* lhs, const String& rhs) { return rhs.compare(lhs) <= 0; }

// Overload == operator
inline bool operator==(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) == 0; }

inline bool operator==(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) == 0; }

inline bool operator==(const String& lhs, const String& rhs) { return lhs.compare(rhs) == 0; }

inline bool operator==(const String& lhs, const char* rhs) { return lhs.compare(rhs) == 0; }

inline bool operator==(const char* lhs, const String& rhs) { return rhs.compare(lhs) == 0; }

// Overload != operator
inline bool operator!=(const String& lhs, const std::string& rhs) { return lhs.compare(rhs) != 0; }

inline bool operator!=(const std::string& lhs, const String& rhs) { return rhs.compare(lhs) != 0; }

inline bool operator!=(const String& lhs, const String& rhs) { return lhs.compare(rhs) != 0; }

inline bool operator!=(const String& lhs, const char* rhs) { return lhs.compare(rhs) != 0; }

inline bool operator!=(const char* lhs, const String& rhs) { return rhs.compare(lhs) != 0; }

inline std::ostream& operator<<(std::ostream& out, const String& input) {
  out.write(input.data(), input.size());
  return out;
}

inline int String::memcmp(const char* lhs, const char* rhs, size_t lhs_count, size_t rhs_count) {
  if (rhs == lhs && lhs_count == rhs_count) return 0;

  for (size_t i = 0; (i < lhs_count) && (i < rhs_count); ++i) {
    if (lhs[i] < rhs[i]) return -1;
    if (lhs[i] > rhs[i]) return 1;
  }
  if (lhs_count < rhs_count) {
    return -1;
  } else if (lhs_count > rhs_count) {
    return 1;
  } else {
    return 0;
  }
}

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_CONTAINER_H
