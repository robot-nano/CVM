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
using runtime::ObjectEqual;
using runtime::ObjectHash;
using runtime::ObjectPtr;
using runtime::ObjectPtrHash;
using runtime::ObjectRef;
using runtime::String;
using runtime::StringObj;

/*! \brief Shared content of all specializations of hash map */
class MapNode : public Object {
 public:
  /*! \brief Type of the keys in the hash map */
  using key_type = ObjectRef;
  /*! \brief Type of the values in the hash map */
  using mapped_type = ObjectRef;
  /*! \brief Type of value stored in the hash map */
  using KVType = std::pair<ObjectRef, ObjectRef>;
  /*! \brief Iterator class */
  class iterator;

  static_assert(std::is_standard_layout<KVType>::value, "KVType is not standard layout");
  static_assert(sizeof(KVType) == 16 || sizeof(KVType) == 8, "sizeof(KVType) incorrect");

  static constexpr const uint32_t _type_index = runtime::TypeIndex::kRuntimeMap;
  static constexpr const char* _type_key = "Map";
  CVT_DECLARE_FINAL_OBJECT_INFO(MapNode, Object);

  /*!
   * \brief Number of elements in the SmallMapNode
   * \return The result
   */
  size_t size() const { return size_; }
  /*!
   * \brief Count the number of times a key exists in the hash map
   * \param key The indexing key
   * \return The result, 0 or 1
   */
  size_t count(const key_type& key) const;
  /*!
   * \brief Index value associated with a key, throw exception if the key does not exist
   * \param key The indexing key
   * \return The const reference to the value
   */
  const mapped_type& at(const key_type& key) const;
  /*!
   * \brief Index value associated with a key, throw exception if the key does not exist
   * \param key The indexing key
   * \return The mutable reference to the value
   */
  mapped_type& at(const key_type& key);
  /*! \brief begin iterator */
  iterator begin() const;
  /*! \brief end iterator */
  iterator end() const;
  /*!
   * \brief Index value associated with a key
   * \param key The indexing key
   * \return The iterator of the entry associated with the key, end iterator if not exists
   */
  iterator find(const key_type& key) const;
  /*!
   * \brief Erase the entry associated with the iterator
   * \param position The iterator
   */
  void erase(const iterator& position);
  /*!
   * \brief Erase the entry associated with the key, do nothing if not exists
   * \param key The indexing key
   */
  void erase(const key_type& key) { erase(find(key)); }

  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = int64_t;
    using value_type = KVType;
    using pointer = KVType*;
    using reference = KVType&;
    /*! \brief Default constructor */
    iterator() : index(0), self(nullptr) {}
    /*! \brief Compare iterators */
    bool operator==(const iterator& other) const {
      return index == other.index && self == other.self;
    }
    /*! \brief Compare iterators */
    bool operator!=(const iterator& other) const { return !(*this == other); }
    /*! \brief De-reference iterators */
    pointer operator->() const;
    /*! \brief De-Reference iterators */
    reference operator*() const { return *((*this).operator->()); }
    /*! \brief Prefix self increment, e.g. ++iter */
    iterator& operator++();
    /*! \brief Prefix self decrement, e.g. --iter */
    iterator& operator--();
    /*! \brief Suffix self increment */
    iterator operator++(int) {
      iterator copy = *this;
      ++(*this);
      return copy;
    }
    /*! \brief Suffix self decrement */
    iterator operator--(int) {
      iterator copy = *this;
      --(*this);
      return copy;
    }

   protected:
    /*! \brief Constructor by value */
    iterator(uint64_t index, const MapNode* self) : index(index), self(self) {}
    /*! \brief The position on the array */
    uint64_t index;
    /*! \brief The container it points to */
    const MapNode* self;

    friend class DenseMapNode;
    friend class SmallMapNode;
  };  // class iterator
  /*!
   * \brief Create an empty container
   * \return The object created
   */
  static inline ObjectPtr<MapNode> Empty();

 protected:
  /*!
   * \brief Create the map using contents from the given iterators.
   * \param first Begin of iterator
   * \param last End of iterator
   * \tparam IterType The type of iterator
   * \return ObjectPtr to the map created
   */
  template <typename IterType>
  static inline ObjectPtr<Object> CreateFromRange(IterType first, IterType last);

  static inline void InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map);

  static inline ObjectPtr<MapNode> CopyFrom(MapNode* from);

  uint64_t slots_;

  uint64_t size_;

  template <typename, typename, typename, typename>
  friend class Map;
};  // class MapNode

/*! \brief A specialization of small-sized hash map */
class SmallMapNode : public MapNode,
                     public runtime::InplaceArrayBase<SmallMapNode, MapNode::KVType> {
 private:
  static constexpr uint64_t kInitSize = 2;
  static constexpr uint64_t kMaxSize = 4;

 public:
  using MapNode::iterator;
  using MapNode::KVType;

  ~SmallMapNode() = default;

  size_t count(const key_type& key) const { return find(key).index < size_; }

  const mapped_type& at(const key_type& key) const {
    iterator itr = find(key);
    ICHECK(itr.index < size_) << "IndexError: key is not in Map";
    return itr->second;
  }

  mapped_type& at(const key_type& key) {
    iterator itr = find(key);
    ICHECK(itr.index < size_) << "IndexError: key is not in Map";
    return itr->second;
  }

  iterator begin() const { return iterator(0, this); }

  iterator end() const { return iterator(size_, this); }

  iterator find(const key_type& key) const {
    KVType* ptr = static_cast<KVType*>(AddressOf(0));
    for (uint64_t i = 0; i < size_; ++i, ++ptr) {
      if (ObjectEqual()(ptr->first, key)) {
        return iterator(i, this);
      }
    }
    return iterator(size_, this);
  }

  void erase(const iterator& position) { Erase(position.index); }

 private:
  /*!
   * \brief Remove a position in SmallMapNode
   * \param index The position to be removed
   */
  void Erase(const uint64_t index) {
    if (index >= size_) {
      return;
    }
    KVType* begin = static_cast<KVType*>(AddressOf(0));
    KVType* last = begin + (size_ - 1);
    if (index + 1 == size_) {
      last->first.ObjectRef::~ObjectRef();
      last->second.ObjectRef::~ObjectRef();
    } else {
      *(begin + index) = std::move(*last);
    }
    size_ -= 1;
  }

  static ObjectPtr<SmallMapNode> Empty(uint64_t n = kInitSize) {
    using cvt::runtime::make_inplace_array_object;
    ObjectPtr<SmallMapNode> p = make_inplace_array_object<SmallMapNode, KVType>(n);
    p->size_ = 0;
    p->slots_ = n;
    return p;
  }

  template <typename IterType>
  static ObjectPtr<SmallMapNode> CreateFromRange(uint64_t n, IterType first, IterType last) {
    ObjectPtr<SmallMapNode> p = Empty(n);
    KVType* ptr = static_cast<KVType*>(p->AddressOf(0));
    for (; first != last; ++first, ++p->size_) {
      new (ptr++) KVType(*first);
    }
    return p;
  }

  static ObjectPtr<SmallMapNode> CopyFrom(SmallMapNode* from) {
    KVType* first = static_cast<KVType*>(from->AddressOf(0));
    KVType* last = first + from->size_;
    return CreateFromRange(from->size_, first, last);
  }

  static void InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map) {
    SmallMapNode* map_node = static_cast<SmallMapNode*>(map->get());
    iterator itr = map_node->find(kv.first);
    if (itr.index < map_node->size_) {
      itr->second = kv.second;
      return;
    }
    if (map_node->size_ < map_node->slots_) {
      KVType* ptr = static_cast<KVType*>(map_node->AddressOf(map_node->size_));
      new (ptr) KVType(kv);
      ++map_node->size_;
      return;
    }
    uint64_t next_size = std::max(map_node->slots_ * 2, uint64_t(kInitSize));
    next_size = std::min(next_size, uint64_t(kMaxSize));
    ICHECK_GT(next_size, map_node->slots_);
    ObjectPtr<Object> new_map = CreateFromRange(next_size, map_node->begin(), map_node->end());
    InsertMaybeReHash(kv, &new_map);
    *map = std::move(new_map);
  }

  uint64_t GetSize() const { return size_; }

  friend class MapNode;
  friend class DenseMapNode;
  friend class runtime::InplaceArrayBase<SmallMapNode, MapNode::KVType>;
};  // class SmallMapNode

/*!
 * \brief A specialization of hash map that implements the idea of array-based hash map.
 * Another reference implementation can be found [1].
 *
 * A. Overview
 *
 * DenseMapNode did several improvements over traditional separate chaining hash,
 * in terms of cache locality, memory footprints and data organization.
 *
 * A1. Implicit linked list. For better cache locality, instead of using linked list
 * explicitly for each bucket, we store list data into a single array that spans contiguously
 * in memory, and then carefully design access patterns to make sure most of them fall into
 * a single cache line.
 *
 * A2. 1-byte metadata. There is only 1 byte overhead for each slot in the array to indexing and
 * traversal. This can be divided in 3 parts.
 * 1) Reserved code: (0b11111111)_2 indicates a slot is empty; (0b11111110)_2 indicates protected,
 * which means the slot is empty buyt not allowed to be written.
 * 2) If not empty or protected, the highest bit is used to indicate whether data in the slot is
 * head of a linked list.
 * 3) The rest 7 bits are used as the "next pointer" (i.e. pointer to the next element). On 64-bit
 * architecture, an ordinary pointer can take up to 8 bytes, which is not acceptable overhead when
 * dealing with 16-byte ObjectRef pairs. Based on a commonly noticed fact that the lists are
 * relatively short (length <= 3) in hash maps, we follow [1]'s idea that only allows the pointer to
 * be one of the 126 possible value, i.e. if the next element of i-th slot is (i + x)-th element,
 * then x must be one of the 126 pre-defined values.
 *
 *  A3. Data blocking. We organize the array in the way that every 16 elements forms a data block.
 *  The 16-byte metadata of those 16 elements are stored together, followed by the real data, i.e.
 *  16 key-value pairs.
 *
 *  B. Implementation details
 *
 *  B1. Power-of-2 table size and Fibonacci Hashing. We use power-of-two as table size to avoid
 *  module for more efficient arithmetics. To make the hash-to-slot mapping distribute more evenly,
 *  we use the Fibonacci Hashing [2] trick.
 *
 *  B2. Traverse a linked list in the array.
 *  1) List head. Assume Fibonacci Hashing maps a given key to slot i, if metadata at slot i
 *  indicates the it is list head, then we found the head; otherwise the list is empty. No probing
 *  is done in this procedure. 2) Next element. To find the next element of a non-empty slot i, we
 *  look at the last 7 bits of the metadata at slot i. If they are all zeros, then it is the end of
 *  list; otherwise, we know that the next element is (i + candidates[the-last-7-bits]).
 *
 *  B3. InsertMaybeReHash an element. Following B2, we first traverse the linked list to see if this
 *  element is in the linked list, and if not, we put is at the end by probing the next empty
 *  position in one of the 126 candidate positions. If the linked list does not even exist, but the
 *  slot for list head has been occupied by another linked list, we should find this intruder
 * another place.
 *
 *  B4. Quadratic probing with triangle numbers. In open address hashing, it is provable that
 * probing with triangle numbers can traverse power-of-2-sized table [3]. In our algorithm, we
 * follow the suggestion in [1] that also use triangle numbers for "next pointer" as well as sparing
 * for list head.
 *
 */
class DenseMapNode : public MapNode {
 private:
  static constexpr int kBlockCap = 16;
  static constexpr double kMaxLoadFactor = 0.99;
  static constexpr uint8_t kEmptySlot = uint8_t(0b11111111);
  static constexpr uint8_t kProtectedSlot = uint8_t(0b11111110);
  static constexpr int kNumJumpDists = 126;
  struct ListNode;
  struct Block {
    uint8_t bytes[kBlockCap + kBlockCap * sizeof(KVType)];
  };
  static_assert(sizeof(Block) == kBlockCap * (sizeof(KVType) + 1), "sizeof(Block) incorrect");
  static_assert(std::is_standard_layout<Block>::value, "Block is not standard layout");

 public:
  using MapNode::iterator;

  ~DenseMapNode() {}

 private:
  ListNode Search(const key_type& key) const {
    if (this->size_ == 0) {
      return ListNode();
    }
  }

  struct ListNode {
    ListNode() : index(0), block(nullptr) {}

    ListNode(uint64_t index, const DenseMapNode* self)
        : index(index), block(self->data_ + (index / kBlockCap)) {}

    uint8_t& Meta() const { return *(block->bytes + index % kBlockCap); }

    KVType& Data() const {
        return *(reinterpret_cast<KVType*>(block->bytes + kBlockCap +
                                           (index % kBlockCap) * sizeof(KVType)));
    }

    key_type& Key() const { return Data().first; }

    mapped_type& Val() const { return Data().second; }

    bool IsHead() const { return (Meta() & 0b10000000) == 0b00000000; }

    bool IsNone() const { return block == nullptr; }

    bool IsEmpty() const { return Meta() == uint8_t(kEmptySlot); }

    bool IsProtected() const { return Meta() == uint8_t(kProtectedSlot); }

    void SetEmpty() const { Meta() = uint8_t(kEmptySlot); }

    void SetProtected() const { Meta() = uint8_t(kProtectedSlot); }

    void SetJump(uint8_t jump) const { (Meta() &= 0b10000000) |= jump; }

    void NewHead(KVType v) const {
      Meta() = 0b00000000;
      new (&Data()) KVType(std::move(v));
    }

    void NewTail(KVType v) const {
      Meta() = 0b10000000;
      new (&Data()) KVType(std::move(v));
    }

    bool HasNext() const { return kNextProbeLocation[Meta() & 0b01111111] != 0; }

    bool MoveToNext(const DenseMapNode* self, uint8_t meta) {
      uint64_t offset = kNextProbeLocation[meta & 0b01111111];
      if (offset == 0) {
        index = 0;
        block = nullptr;
        return false;
      }
      index = (index + offset) & (self->slots_);
      block = self->data_ + (index / kBlockCap);
      return true;
    }

    bool MoveToNext(const DenseMapNode* self) { return MoveToNext(self, Meta()); }

    ListNode FindPrev(const DenseMapNode* self) const {

    }

    uint64_t index;
    Block* block;
  };

 protected:
  /*! \brief fib shift in Fibonacci Hashing*/
  uint32_t fib_shift_;
  /*! \brief array of data blocks */
  Block* data_;
  /* clang-format off */
  /*! \brief Candidates of probing distance */
  CVT_DLL static constexpr uint64_t kNextProbeLocation[kNumJumpDists]{
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
      // Quadratic probing with triangle numbers. See also:
      // 1) https://en.wikipedia.org/wiki/Quadratic_probing
      // 2) https://fgiesen.wordpress.com/2015/02/22/triangular-numbers-mod-2n/
      // 3) https://github.com/skarupke/flat_hash_map
      21, 28, 36, 45, 55, 66, 78, 91, 105, 120,
      136, 153, 171, 190, 210, 231, 253, 276, 300, 325,
      351, 378, 406, 435, 465, 496, 528, 561, 595, 630,
      666, 703, 741, 780, 820, 861, 903, 946, 990, 1035,
      1081, 1128, 1176, 1225, 1275, 1326, 1378, 1431, 1485, 1540,
      1596, 1653, 1711, 1770, 1830, 1891, 1953, 2016, 2080, 2145,
      2211, 2278, 2346, 2415, 2485, 2556, 2628,
      // Larger triangle numbers
      8515, 19110, 42778, 96141, 216153,
      486591, 1092981, 2458653, 5532801, 12442566,
      27993904, 62983476, 141717030, 318844378, 717352503,
      1614057336, 3631522476, 8170957530, 18384510628, 41364789378,
      93070452520, 209408356380, 471168559170, 1060128894105, 2385289465695,
      5366898840628, 12075518705635, 27169915244790, 61132312065111, 137547689707000,
      309482283181501, 696335127828753, 1566753995631385, 3525196511162271, 7931691992677701,
      17846306936293605, 40154190677507445, 90346928918121501, 203280589587557251, 457381325854679626,
      1029107982097042876, 2315492959180353330, 5209859154120846435,
  };
  /* clang-format on */
  friend class MapNode;
};  // class DenseMapNode

//#define CVT_DISPATCH_MAP_CONST(base, var, body) \
//  {                                             \
//    using TSmall = const SmallMapNode*;         \
//    using TDense = const DenseMapNode*;         \
//    uint64_t slots = base->slots_;              \
//    if (slots <= SmallMapNode::kMaxSize) {      \
//      TSmall var = static_cast<TSmall>(base);   \
//      body;                                     \
//    } else {                                    \
//      TDense var = static_cast<TDense>(base);   \
//      body;                                     \
//    }                                           \
//  }
//
// inline size_t MapNode::count(const key_type& key) const {
//  CVT_DISPATCH_MAP_CONST(this, p, { return p->count(key); });
//}
//
// inline const MapNode::mapped_type& MapNode::at(const key_type& key) const {
//  CVT_DISPATCH_MAP_CONST(this, p, { return p->at(key); })
//}
//
// inline MapNode::mapped_type& MapNode::at(const key_type& key) {
//  //  CVT_DISPATCH_MAP_CONST(this, p, {return p->at(key)})
//}

inline ObjectPtr<MapNode> MapNode::Empty() { return SmallMapNode::Empty(); }

inline ObjectPtr<MapNode> MapNode::CopyFrom(MapNode* from) {
  if (from->slots_ <= SmallMapNode::kMaxSize) {
    return SmallMapNode::CopyFrom(static_cast<SmallMapNode*>(from));
  } else {
    return DenseMapNode::CopyFrom(static_cast<DenseMapNode*>(from));
  }
}

template <typename IterType>
inline ObjectPtr<Object> MapNode::CreateFromRange(IterType first, IterType last) {
//  int64_t _cap = std::distance(first, last);
//  if (_cap < 0) return SmallMapNode::Empty();
//
//  uint64_t cap = static_cast<uint64_t>(_cap);
//  if (cap < SmallMapNode::kMaxSize) {
//    return SmallMapNode::CreateFromRange(cap, first, last);
//  }
//  uint32_t fib_shift;
//  uint64_t n_slots;
//  DenseMapNode::CalcTableSize(cap, &fib_shift, &n_slots);
//  ObjectPtr<Object> obj = DenseMapNode::Empty(fib_shift, n_slots);
//  for (; first != last; ++first) {
//    KVType kv(*first);
//    DenseMapNode::InsertMaybeReHash(kv, &obj);
//  }
//  return obj;
}

inline void MapNode::InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map) {
  constexpr uint64_t kSmallMapMaxSize = SmallMapNode::kMaxSize;
  MapNode* base = static_cast<MapNode*>(map->get());
  if (base->slots_ < kSmallMapMaxSize) {
    SmallMapNode::InsertMaybeReHash(kv, map);
  } else if (base->slots_ == kSmallMapMaxSize) {
    if (base->size_ < base->slots_) {
      SmallMapNode::InsertMaybeReHash(kv, map);
    } else {
      ObjectPtr<Object> new_map = MapNode::CreateFromRange(base->begin(), base->end());
      DenseMapNode::InsertMaybeReHash(kv, &new_map);
      *map = std::move(new_map);
    }
  } else {
    DenseMapNode::InsertMaybeReHash(kv, map);
  }
}

template <typename K, typename V,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, K>::value>::type,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, V>::value>::type>
class Map : public ObjectRef {
 public:
  using key_type = K;
  using mapped_type = V;
  class iterator;

  Map() { data_ = MapNode::Empty(); }

  Map(Map<K, V>&& other) { data_ = std::move(other.data_); }

  Map(const Map<K, V>& other) : ObjectRef(other.data_) {}

  Map<K, V>& operator=(Map<K, V>&& other) {
    data_ = std::move(other.data_);
    return *this;
  }

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

  iterator begin() const { return iterator(GetMapNode()->begin()); }

  iterator end() const { return iterator(GetMapNode()->end()); }

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

  using ContainerType = MapNode;

  class iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = int64_t;
    using value_type = const std::pair<K, V>;
    using pointer = value_type*;
    using reference = value_type;

    iterator() : itr() {}

    bool operator==(const iterator& other) const { return itr == other.itr; }

    bool operator!=(const iterator& other) const { return itr != other.itr; }

    pointer operator->() const = delete;

    reference operator*() const {
      auto& kv = *itr;
      return std::make_pair(DowncastNoCheck<K>(kv.first), DowncastNoCheck<V>(kv.second));
    }

    iterator& operator++() {
      ++itr;
      return *this;
    }

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
