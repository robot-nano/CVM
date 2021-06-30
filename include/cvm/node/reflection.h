//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_REFLECTION_H_
#define CVM_INCLUDE_CVM_NODE_REFLECTION_H_

#include <cvm/node/structural_equal.h>
#include <cvm/node/structural_hash.h>
#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/container.h>
#include <cvm/runtime/data_type.h>
#include <cvm/runtime/ndarray.h>
#include <cvm/runtime/object.h>
#include <cvm/runtime/packed_func.h>

#include <functional>

namespace cvm {

using runtime::Object;
using runtime::ObjectPtr;
using runtime::ObjectRef;

/*!
 * \brief Visitor class for to get the attributes of a AST/IR node.
 *  The content is going to be called for each field.
 *
 *  Each objects that wants reflection will need to implement
 *  a VisitAttrs function and call visitor->Visit on each of its field.
 */
class AttrVisitor {
 public:
  CVM_DLL virtual ~AttrVisitor() = default;
  CVM_DLL virtual void Visit(const char* key, double* value) = 0;
  CVM_DLL virtual void Visit(const char* key, int64_t* value) = 0;
  CVM_DLL virtual void Visit(const char* key, uint64_t* value) = 0;
  CVM_DLL virtual void Visit(const char* key, int* value) = 0;
  CVM_DLL virtual void Visit(const char* key, bool* value) = 0;
  CVM_DLL virtual void Visit(const char* key, std::string* value) = 0;
  CVM_DLL virtual void Visit(const char* key, void** value) = 0;
  CVM_DLL virtual void Visit(const char* key, runtime::DataType* value) = 0;
  CVM_DLL virtual void Visit(const char* key, runtime::ObjectRef* value) = 0;
  template <typename ENum, typename = typename std::enable_if<std::is_enum<ENum>::value>::type>
  void Visit(const char* key, ENum* ptr) {
    static_assert(std::is_same<int, typename std::underlying_type<ENum>::type>::value,
                  "declare enum to be enum int to use visitor");
    this->Visit(key, reinterpret_cast<int*>(ptr));
  }
};

class ReflectionVTable {
 public:
  typedef void (*FVisitAttrs)(Object* self, AttrVisitor* visitor);

  typedef bool (*FSEqualReduce)(const Object* self, const Object* other, SEqualReducer equal);

  typedef void (*FSHashReduce)(const Object* self, SHashReducer hash_reduce);

  typedef ObjectPtr<Object> (*FCreate)(const std::string& repr_bytes);

  typedef std::string (*FReprBytes)(const Object* self);

  inline void VisitAttrs(Object* self, AttrVisitor* visitor) const;

  inline void GetReprBytes(const Object* self, std::string* repr_bytes) const;

  bool SEqualReduce(const Object* self, const Object* other, SEqualReducer equal) const;

  void SHashReduce(const Object* self, SHashReducer hash_reduce) const;

  CVM_DLL ObjectPtr<Object> CreateInitObject(const std::string& type_key,
                                             const std::string& repr_bytes = "") const;

  CVM_DLL ObjectRef CreateObject(const std::string& type_key, const runtime::CVMArgs& kwargs);

  CVM_DLL ObjectRef CreateObject(const std::string& type_key, const Map<String, ObjectRef>& kwargs);

  CVM_DLL runtime::CVMRetValue GetAttr(Object* self, const String& attr_name) const;

  CVM_DLL std::vector<std::string> ListAttrNames(Object* self) const;

  CVM_DLL static ReflectionVTable* Global();

  class Registry;
  template <typename T, typename TraitName>
  inline Registry Register();

 private:
  std::vector<FVisitAttrs> fvisit_attrs_;
  std::vector<FSEqualReduce> fsequal_reduce_;
  std::vector<FSHashReduce> fshash_reduce_;
  std::vector<FCreate> fcreate_;
  std::vector<FReprBytes> frepr_bytes_;
};

class ReflectionVTable::Registry {
 public:
  Registry(ReflectionVTable* parent, uint32_t type_index)
      : parent_(parent), type_index_(type_index) {}
  /*!
   * \brief Set fcreate function.
   * \param f The creator function.
   * \return rference to self.
   */
  Registry& set_creator(FCreate f) {
    ICHECK_LT(type_index_, parent_->fcreate_.size());
    parent_->fcreate_[type_index_] = f;
    return *this;
  }
  /*!
   * \brief Set bytes repr function.
   * \param f The ReprBytes function.
   * \return reference to self.
   */
  Registry& set_repr_bytes(FReprBytes f) {
    ICHECK_LT(type_index_, parent_->frepr_bytes_.size());
    parent_->frepr_bytes_[type_index_] = f;
    return *this;
  }

 private:
  ReflectionVTable* parent_;
  uint32_t type_index_;
};

namespace detail {

template <typename T, bool = T::_type_has_method_visit_attrs>
struct ImplVisitAttrs {
  static constexpr const std::nullptr_t VisitAttrs = nullptr;
};

template <typename T>
struct ImplVisitAttrs<T, true> {
  static void VisitAttrs(T* self, AttrVisitor* v) { self->VisitAttrs(v); }
};

template <typename T, bool = T::_type_has_method_sequal_reduce>
struct ImplSEqualReduce {
  static constexpr const std::nullptr_t SEqualReduce = nullptr;
};

template <typename T, bool = T::_type_has_method_shash_reduce>
struct ImplSHashReduce {
  static constexpr const std::nullptr_t SHashReduce = nullptr;
};

template <typename T>
struct ReflectionTrait : public ImplVisitAttrs<T>,
                         public ImplSEqualReduce<T>,
                         public ImplSHashReduce<T> {};

template <typename T, typename TraitName,
          bool = std::is_null_pointer<decltype(TraitName::VisitAttrs)>::value>
struct SelectVisitAttrs {
  static constexpr const std::nullptr_t VisitAttrs = nullptr;
};

template <typename T, typename TraitName>
struct SelectVisitAttrs<T, TraitName, false> {
  static void VisitAttrs(Object* self, AttrVisitor* v) {
    TraitName::VisitAttrs(static_cast<T*>(self), v);
  }
};

}  // namespace detail

template <typename T, typename TraitName>
inline ReflectionVTable::Registry ReflectionVTable::Register() {
  uint32_t tindex = T::RuntimeTypeIndex();
  if (tindex >= fvisit_attrs_.size()) {
    fvisit_attrs_.resize(tindex + 1, nullptr);
    fcreate_.resize(tindex + 1, nullptr);
    frepr_bytes_.resize(tindex + 1, nullptr);
    fsequal_reduce_.resize(tindex + 1, nullptr);
    fshash_reduce_.resize(tindex + 1, nullptr);
  }
  // functor that implements the redirection.
}

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_REFLECTION_H_
