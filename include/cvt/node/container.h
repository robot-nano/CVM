//
// Created by WangJingYu on 2021/1/23.
//

#ifndef CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_
#define CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_

#ifndef USE_FALLBACK_STL_MAP
#define USE_FALLBACK_STL_MAP 1
#endif

#include <cvt/runtime/container.h>
#include <cvt/runtime/memory.h>
#include <cvt/runtime/object.h>
#include <cvt/runtime/packed_func.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cvt {

using runtime::Downcast;
using runtime::make_object;
using runtime::Object;
using runtime::ObjectEqual;
using runtime::ObjectHash;
using runtime::ObjectPtr;
using runtime::ObjectPtrEqual;
using runtime::ObjectPtrHash;
using runtime::ObjectRef;
using runtime::String;
using runtime::StringObj;

class MapNode : public Object {
 public:
  using key_type = ObjectRef;
  using mapped_type = ObjectRef;
  using ContainerType = std::unordered_map<ObjectRef, ObjectRef, ObjectHash, ObjectEqual>;
  using iterator = ContainerType::iterator;
  using const_iterator = ContainerType::const_iterator;
  using KVType = ContainerType::value_type;

  static_assert(std::is_standard_layout<KVType>::value, "KVType is not standard layout");
  static_assert(sizeof(KVType) == 16 || sizeof(KVType) == 8, "sizeof(KVType) incorrect");

  static constexpr const uint32_t _type_index = runtime::TypeIndex::kRuntimeMap;
  static constexpr const char* _type_key = "Map";
  CVT_DECLARE_FINAL_OBJECT_INFO(MapNode, Object);

  size_t size() const { return data_.size(); }

  size_t count(const key_type& key) const { return data_.count(key); }

  const mapped_type& at(const key_type& key) const { return data_.at(key); }

  mapped_type& at(const key_type& key) { return data_.at(key); }

  iterator begin() { return data_.begin(); }

  const_iterator begin() const { return data_.begin(); }

  iterator end() { return data_.end(); }

  const_iterator end() const { return data_.end(); }

  const_iterator find(const key_type& key) const { return data_.find(key); }

  iterator find(const key_type& key) { return data_.find(key); }

  void erase(const iterator& position) { data_.erase(position); }

  void erase(const key_type& key) { data_.erase(key); }

  static ObjectPtr<MapNode> Empty() { return make_object<MapNode>(); }

 protected:
  template <typename IterType>
  static ObjectPtr<Object> CreateFromRange(IterType first, IterType last) {
    ObjectPtr<MapNode> p = make_object<MapNode>();
    p->data_ = ContainerType(first, last);
    return p;
  }

  static void InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map) {
    MapNode* map_node = static_cast<MapNode*>(map->get());
    map_node->data_[kv.first] = kv.second;
  }

  static ObjectPtr<MapNode> CopyFrom(MapNode* from) {
    ObjectPtr<MapNode> p = make_object<MapNode>();
    p->data_ = ContainerType(from->data_.begin(), from->data_.end());
    return p;
  }

  ContainerType data_;
  template <typename, typename, typename, typename>
  friend class Map;
};

template <typename K, typename V,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, K>::value>::type,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, V>::value>::type>
class Map : public ObjectRef {
 public:
  using key_type = K;
  using mapped_type = V;
  class iterator;
  /*!
   * \brief default constructor
   */
  Map() { data_ = MapNode::Empty(); }
  /*!
   * \brief move constructor
   * \param other source
   */
  Map(Map<K, V>&& other) { data_ = std::move(other.data_); }
  /*!
   * \brief copy constructor
   * \param other source
   */
  Map(const Map<K, V>& other) : ObjectRef(other.data_) {}
  /*!
   * \brief copy assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Map<K, V>& operator=(Map<K, V>&& other) {
    data_ = std::move(other.data_);
    return *this;
  }
  /*!
   * \brief move assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Map<K, V>& operator=(const Map<K, V>& other) {
    data_ = other.data_;
    return *this;
  }

  explicit Map(ObjectPtr<Object> n) : ObjectRef(n) {}

  template <typename IterType>
  Map(IterType begin, IterType end) {
    data_ = MapNode::CreateFromRange(begin, end);
  }

  Map(std::initializer_list<std::pair<K, V>> init) {
    data_ = MapNode::CreateFromRange(init.begin(), init.end());
  }

  Map(std::vector<std::pair<K, V>> init) {
    data_ = MapNode::CreateFromRange(init.begin(), init.end());
  }

  const V at(const K& key) const { return DowncastNoCheck<V>(GetMapNode()->at(key)); }

  const V operator[](const K& key) const { return this->at(key); }

  size_t size() const {
    MapNode* n = GetMapNode();
    return n == nullptr ? 0 : n->size();
  }

  size_t count(const K& key) const {
    MapNode* n = GetMapNode();
    return n == nullptr ? 0 : GetMapNode()->count(key);
  }

  bool empty() const { return size() == 0; }

  void set(const K& key, const V& value) {
    CopyOnWrite();
    MapNode::InsertMaybeReHash(MapNode::KVType(key, value), &data_);
  }
  /*! \return begin iterator */
  iterator begin() const { return iterator(GetMapNode()->begin()); }
  /*! \return end iterator */
  iterator end() const { return iterator(GetMapNode()->end()); }
  /*! \return find the key and returns the associated iterator */
  iterator find(const K& key) const { return iterator(GetMapNode()->find(key)); }

  void erase(const K& key) { CopyOnWrite()->erase(key); }

  MapNode* CopyOnWrite() {
    if (data_.get() == nullptr) {
      data_ = MapNode::Empty();
    } else if (!data_.unique()) {
      data_ = MapNode::CopyFrom(GetMapNode());
    }
    return GetMapNode();
  }
  /*! \brief specify container node */
  using ContainerType = MapNode;

  /*! \brief Iterator of the hash map */
  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = int64_t;
    using value_type = const std::pair<K, V>;
    using pointer = value_type*;
    using reference = value_type;

    iterator() : itr() {}

    /*! \brief Compare iterators */
    bool operator==(const iterator& other) const { return itr == other.itr; }
    /*! \brief Compare iterators */
    bool operator!=(const iterator& other) const { return itr != other.itr; }
    /*! \brief De-reference iterators is not allowed */
    pointer operator->() const = delete;
    /*! \brief De-reference iterators */
    reference operator*() const {
      auto& kv = *itr;
      return std::make_pair(DowncastNoCheck<K>(kv.first), DowncastNoCheck<V>(kv.second));
    }
    /*! \brief Prefix self increment, e.g. ++iter */
    iterator& operator++() {
      ++itr;
      return *this;
    }
    /*! \brief Suffix self increment */
    iterator operator++(int) {
      iterator copy = *this;
      ++(*this);
      return copy;
    }

   private:
    iterator(const MapNode::iterator& itr)  // NOLINT(*)
        : itr(itr) {}

    template <typename, typename, typename, typename>
    friend class Map;

    MapNode::iterator itr;
  };

 private:
  MapNode* GetMapNode() const { return static_cast<MapNode*>(data_.get()); }
};

template <typename K, typename V,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, K>::value>::type,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, V>::value>::type>
inline Map<K, V> Merge(Map<K, V> lhs, const Map<K, V>& rhs) {
  for (const auto& p : rhs) {
    lhs.set(p.first, p.second);
  }
  return std::move(lhs);
}

}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_
