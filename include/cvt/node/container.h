//
// Created by WangJingYu on 2021/1/23.
//

#ifndef CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_
#define CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_

#include <cvt/runtime/container.h>
#include <cvt/runtime/memory.h>
#include <cvt/runtime/object.h>
#include <cvt/runtime/packed_func.h>

#include <algorithm>
#include <string>
#include <utility>

namespace cvt {

using runtime::Downcast;
using runtime::make_object;
using runtime::Object;
using runtime::ObjectPtr;
using runtime::ObjectPtrHash;
using runtime::ObjectRef;
using runtime::String;
using runtime::StringObj;

class MapNode : public Object {
 public:
  using key_type = ObjectRef;
  using mapped_type = ObjectRef;
  using KVType = std::pair<ObjectRef, ObjectRef>;

  class iterator;

  static_assert(std::is_standard_layout<KVType>::value, "KVType is not standard layout");
  static_assert(sizeof(KVType) == 16 || sizeof(KVType) == 8, "sizeof(KVType) incorrect");

  static constexpr const uint32_t _type_index = runtime::TypeIndex::kRuntimeMap;
  static constexpr const char* _type_key = "Map";
  CVT_DECLARE_FINAL_OBJECT_INFO(MapNode, Object);

  size_t size() const { return size_; }

  size_t count(const key_type& key) const;

  const mapped_type& at(const key_type& key) const;

  mapped_type& at(const key_type& key);

  iterator begin() const;

  iterator end() const;

  iterator find(const key_type& key) const;

  void erase(const iterator& position);

  void erase(const key_type& key) { erase(find(key)); }

  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = int64_t;
    using value_type = KVType;
    using pointer = KVType*;
    using reference = KVType&;

    iterator() : index(0), self(nullptr) {}

    bool operator==(const iterator& other) const {
      return index == other.index && self == other.self;
    }

    bool operator!=(const iterator& other) const { return !(*this == other); }

    pointer operator->() const;

    reference operator*() const { return *((*this).operator->()); }

    iterator& operator++();

    iterator& operator--();

    iterator operator++(int) {
      iterator copy = *this;
      ++(*this);
      return copy;
    }

    iterator operator--(int) {
      iterator copy = *this;
      --(*this);
      return copy;
    }

   protected:
    iterator(uint64_t index, const MapNode* self) : index(index), self(self) {}

    uint64_t index;
    const MapNode* self;
    friend class DenseMapNode;
    friend class SmallMapNode;
  };

  static inline ObjectPtr<MapNode> Empty();

 protected:
  template <typename IterType>
  static inline ObjectPtr<Object> CreateFromRange(IterType first, IterType last);

  static inline void InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map);

  static inline ObjectPtr<MapNode> CopyFrom(MapNode* from);

  uint64_t slots_;

  uint64_t size_;

  template <typename, typename, typename, typename>
  friend class Map;
};



#define CVT_DISPATCH_MAP_CONST(base, var, body) \
  {                                             \
    using TSmall = const SmallMapNode*;         \
    using TDense = const DenseMapNode*;         \
    uint64_t slots = base->slots_;              \
    if (slots <= SmallMapNode::kMaxSize) {      \
      TSmall var = static_cast<TSmall>(base);   \
      body;                                     \
    } else {                                    \
      TDense var = static_cast<TDense>(base);   \
      body;                                     \
    }                                           \
  }

inline size_t MapNode::count(const key_type& key) const {
//  CVT_DISPATCH_MAP_CONST(this, p, {return p->count(key); });
}

}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_
