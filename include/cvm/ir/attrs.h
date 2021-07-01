//
// Created by WangJingYu on 2021/7/1.
//

#ifndef CVM_INCLUDE_CVM_IR_ATTRS_H_
#define CVM_INCLUDE_CVM_IR_ATTRS_H_

#include <cvm/ir/expr.h>
#include <cvm/node/structural_equal.h>
#include <cvm/node/structural_hash.h>

namespace cvm {

struct AttrError : public Error {
  explicit AttrError(std::string msg) : Error("AttributeError:" + msg) {}
};

class AttrFieldInfoNode : public Object {
 public:
  /*! \brief name of the field */
  String name;
  /*! \brief type docstring information in str. */
  String type_info;
  /*! \brief detailed description of the type */
  String description;

  void VisitAttrs(AttrVisitor* v) {
    v->Visit("name", &name);
    v->Visit("type_info", &type_info);
    v->Visit("description", &description);
  }

  static constexpr const char* _type_key = "AttrFieldInfo";
  static constexpr bool _type_has_method_sequal_reduce = false;
  static constexpr bool _type_has_method_shash_reduce = false;
  CVM_DECLARE_FINAL_OBJECT_INFO(AttrFieldInfoNode, Object);
};

class AttrFieldInfo : public ObjectRef {
 public:
  CVM_DEFINE_OBJECT_REF_METHODS(AttrFieldInfo, ObjectRef, AttrFieldInfoNode);
};

/*!
 * \brief Base class of all attribute class
 * \note Do not subclass AttrBaseNode directly.
 *       subclass AttrsNode instead.
 * \sa AttrsNode
 */
class BaseAttrsNode : public Object {
 public:
  using CVMArgs = runtime::CVMArgs;
  using CVMRetValue = runtime::CVMRetValue;
  /*! \brief virtual destructor */
  virtual ~BaseAttrsNode() {}
  /*! \brief visit function */
  virtual void VisitAttrs(AttrVisitor* v) {}
  /*!
   * \brief Initialize the attributes by sequence of arguments
   * \param args The positional arguments in the form
   *        [key0, value0, key1, value1, ..., key_n, value_n]
   */
  template <typename... Args>
  inline void InitBySeq(Args&&... args);
  /*!
   * \brief Print readable docstring to ostream, add newline.
   * \param os the stream to print the docstring to.
   */
  inline void PrintDocString(std::ostream& os) const;
  /*!
   * \brief Visit attributes that do not equal the default value.
   *
   * \note This is useful to extract fields for concise printing.
   * \param v
   */
  CVM_DLL virtual void VisitNonDefaultAttrs(AttrVisitor* v) = 0;
  /*!
   * \brief Get the field information
   * \return The fields in the Attrs.
   */
  CVM_DLL virtual Array<AttrFieldInfo> ListFieldInfo() const = 0;
  /*!
   * \brief Initialize the attributes by arguments.
   * \param kwargs The key value pairs for initialization.
   *        [key0, value0, key1, value1, ..., key_n, value_n]
   * \param allow_unknown Whether allow additional unknown fields.
   * \note This function throws when the required field is not present.
   */
  CVM_DLL virtual void InitByPackedArgs(const CVMArgs& kwargs, bool allow_unknown = false) = 0;

  static constexpr const bool _type_has_method_sequal_reduce = true;
  static constexpr const bool _type_has_method_shash_reduce = true;
  CVM_DECLARE_BASE_OBJECT_INFO(BaseAttrsNode, Object);
};

class Attrs : public ObjectRef {
 public:
  CVM_DEFINE_OBJECT_REF_METHODS(Attrs, ObjectRef, BaseAttrsNode);
};

/*!
 * \brief Specialized attribute type that is backed by a map.
 *  The DictAttrsNode implements the Attrs behavior,
 *  its fields are directly accessible via object.field_name
 *  like other normal nodes.
 */
class DictAttrsNode : public BaseAttrsNode {
 public:
  Map<String, ObjectRef> dict;

  bool SEqualReduce(const DictAttrsNode* other, SEqualReducer equal) const {
    return equal(dict, other->dict);
  }

  void SHashReduce(SHashReducer hash_reduce) const { return hash_reduce(dict); }

  void VisitAttrs(AttrVisitor* v) final;
  void VisitNonDefaultAttrs(AttrVisitor* v) final;
  void InitByPackedArgs(const runtime::CVMArgs& args, bool allow_unknown) final;
  Array<AttrFieldInfo> ListFieldInfo() const final;
  // type info
  static constexpr const char* _type_key = "DictAttrs";
  CVM_DECLARE_FINAL_OBJECT_INFO(DictAttrsNode, BaseAttrsNode);
};

namespace detail {
using runtime::CVMArgValue;

struct AttrNopEntry {
  using TSelf = AttrNopEntry;
};

class AttrNormalVisitor {
 public:
  explicit AttrNormalVisitor(AttrVisitor* visitor) : visitor_(visitor) {}
  template <typename T>
  AttrNopEntry operator()(const char* key, T* value) {
    visitor_->template Visit(key, value);
    return AttrNopEntry();
  }

 private:
  AttrVisitor* visitor_;
};

class AttrsSEqualVisitor {
 public:
  bool result_{true};
  AttrsSEqualVisitor(const Object* lhs, const Object* rhs, const SEqualReducer& equal)
      : lhs_(lhs), rhs_(rhs), equal_(equal) {}

  template <typename T>
  AttrNopEntry operator()(const char* key, T* lhs_value) {
    if (!result_) return AttrNopEntry();
    const T* rhs_value = reinterpret_cast<const T*>(
        reinterpret_cast<const char*>(rhs_) +
        (reinterpret_cast<const char*>(lhs_value) - reinterpret_cast<const char*>(lhs_)));
    if (!equal_(*lhs_value, *rhs_value)) {
      result_ = false;
    }
    return AttrNopEntry();
  }

 private:
  const Object* lhs_;
  const Object* rhs_;
  const SEqualReducer& equal_;
};

class AttrsSHashVisitor {
 public:
  explicit AttrsSHashVisitor(const SHashReducer& hash_reducer) : hash_reducer_(hash_reducer) {}

  template <typename T>
  AttrNopEntry operator()(const char* key, T* value) {
    hash_reducer_(*value);
    return AttrNopEntry();
  }

 private:
  const SHashReducer& hash_reducer_;
};

/*! \brief helper entry that does initialization, set default */
template <typename T>
struct AttrInitEntry {
  using TSelf = AttrInitEntry<T>;

  const char* type_key_;

  const char* key_;

  T* value_;

  bool value_missing_{false};

  AttrInitEntry() = default;

  AttrInitEntry(AttrInitEntry&& other) {
    type_key_ = other.type_key_;
    key_ = other.key_;
    value_ = other.value_;
    value_missing_ = other.value_missing_;
    other.value_missing_ = false;
  }

  ~AttrInitEntry() {
    if (value_missing_) {
      std::ostringstream os;
      os << type_key_ << ": Cannot find required field \'" << key_ << "\' during initialization."
         << "If the key is defined check that its type matches the declared type.";
    }
  }
};

template <typename FFind>
class AttrInitVisitor {
 public:
  size_t hit_count_{0};

  AttrInitVisitor(const char* type_key, FFind ffind) : type_key_(type_key), ffind_(ffind) {}

  template <typename T>
  AttrInitEntry<T> operator()(const char* key, T* value) {}

 private:
  const char* type_key_;
  FFind ffind_;
};

}  // namespace detail

template <typename DerivedType>
class AttrsNode : public BaseAttrsNode {
 public:
  void VisitAttrs(AttrVisitor* v) override {}
};

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_IR_ATTRS_H_
