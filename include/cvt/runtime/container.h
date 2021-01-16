//
// Created by WangJingYu on 2021/1/12.
//

#ifndef CVT_CONTAINER_H
#define CVT_CONTAINER_H

#include <cvt/runtime/object.h>

#include <cstring>
#include <string>

namespace cvt {
namespace runtime {

class StringObj : public Object {
 public:
  const char* data;

  uint64_t size;

  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeString;
  static constexpr const char* _type_key = "runtime.String";
  CVT_DECLARE_FINAL_OBJECT_INFO(StringObj, Object);

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
  friend String operator+(const String& lhs, const std::string & rhs);
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

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_CONTAINER_H
