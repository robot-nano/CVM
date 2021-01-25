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
  };  // class iterator

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
};  // class MapNode

class SmallMapNode : public MapNode,
                     public runtime::InplaceArrayBase<SmallMapNode, MapNode::KVType> {
 private:
  static constexpr uint64_t kIntSize = 2;
  static constexpr uint64_t kMaxSize = 4;

 public:
  using MapNode::iterator;
  using MapNode::KVType;

  ~SmallMapNode() = default;

 protected:
  friend class MapNode;
  friend class DenseMapNode;
  friend class runtime::InplaceArrayBase<SmallMapNode, MapNode::KVType>;
};  // class SmallMapNode

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

  size_t count(const key_type& key) const { return !Search(key).IsNone(); }

  const mapped_type& at(const key_type& key) const {}

  mapped_type& at(const key_type& key) {}

  iterator find(const key_type& key) const {}

  void earse(const iterator& position) {}

  iterator begin() const {}

  iterator end() const {}

 private:
  ListNode Search(const key_type& key) const {}

  /*! \brief The implicit in-place linked list used to index a chain */
  struct ListNode {
    ListNode() : index(0), block(nullptr) {}

    ListNode(uint64_t index, const DenseMapNode* self)
        : index(index), block(self->data_ + (index / kBlockCap)) {}
    /*! \brief Metadata on the entry */
    uint8_t& Meta() const { return *(block->bytes + index % kBlockCap); }
    /*! \brief Data on the entry */
    KVType& Data() const {
      return *(reinterpret_cast<KVType*>(block->bytes + kBlockCap +
                                         (index % kBlockCap) * sizeof(KVType)));
    }
    /*! \brief Key on the entry */
    key_type& key() const { return Data().first; }
    /*! \brief Value on the entry */
    mapped_type& Val() const { return Data().second; }
    /*! \brief If the entry is head of linked list */
    bool IsHead() const { return (Meta() & 0b10000000) == 0b00000000; }
    /*! \brief If the entry is none */
    bool IsNone() const { return block == nullptr; }
    /*! \brief If the entry is empty slot */
    bool IsEmpty() const { return Meta() == uint8_t(kEmptySlot); }
    /*! \brief If the entry is protected slot */
    bool IsProtected() const { return Meta() == uint8_t(kProtectedSlot); }
    /*! \brief Set the entry to be empty */
    void SetEmpty() const { Meta() = uint8_t(kEmptySlot); }
    /*! \brief Set the entry's jump to its next entry */
    void SetProtected() const { Meta() = uint8_t(kProtectedSlot); }
    /*! \brief Set the entry's jump to its next entry */
    void SetJump(uint8_t jump) const { (Meta() &= 0b10000000) |= jump; }
    /*! \brief Construct a head of linked list in-place */
    void NewHead(KVType v) const {
      Meta() = 0b00000000;
      new (&Data()) KVType(std::move(v));
    }
    /*! \brief Construct a tail of linked list in-place */
    void NewTail(KVType v) const {
      Meta() = 0b00000000;
      new (&Data()) KVType (std::move(v));
    }
    /*! \brief If the entry has next entry on the linked list */
    bool HasNext() const { return kNextProbeLocation[Meta() & 0b01111111] != 0; }
    /*! \brief Move the entry to the next entry on the linked list */
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
    /*! \brief Move the entry to the next entry on the linked list */
    bool MoveToNext(const DenseMapNode* self) { return MoveToNext(self, Meta()); }
    /*! \brief Get the previous entry on the linked list */
    ListNode FindPrev(const DenseMapNode* self) const {
      //TODO:
    }

    /*! \brief Index on the real array */
    uint64_t index;
    /*! \brief Pointer to the actual block */
    Block* block;
  };  // struct ListNode

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
  CVT_DISPATCH_MAP_CONST(this, p, { return p->count(key); });
}

inline const MapNode::mapped_type& MapNode::at(const key_type& key) const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->at(key); })
}

inline MapNode::mapped_type& MapNode::at(const key_type& key) {
//  CVT_DISPATCH_MAP_CONST(this, p, {return p->at(key)})
}

}  // namespace cvt

#endif  // CVT_INCLUDE_CVT_RUNTIME_NODE_CONTAINER_H_
