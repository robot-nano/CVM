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

#if (USE_FALLBACK_STL_MAP != 0)

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

#else

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
};

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
    using ::cvt::runtime::make_inplace_array_object;
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
  /*!
   * \brief Increment the pointer
   * \param index The pointer to be incremented
   * \return The increased pointer
   */
  uint64_t IncItr(uint64_t index) const { return index + 1 < size_ ? index + 1 : size_; }
  /*!
   * \brief Decrement the pointer
   * \param index The pointer to be decremented
   * \return The decreased pointer
   */
  uint64_t DecItr(uint64_t index) const { return index > 0 ? index - 1 : size_; }
  /*!
   * \brief De-reference the pointer
   * \param index The pointer to be dereferenced
   * \return The result
   */
  KVType* DeRefItr(uint64_t index) const { return static_cast<KVType*>(AddressOf(index)); }
  /*! \brief A size function used by InplaceArrayBase */
  uint64_t GetSize() const { return size_; }

 protected:
  friend class MapNode;
  friend class DenseMapNode;
  friend class runtime::InplaceArrayBase<SmallMapNode, MapNode::KVType>;
};

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

  const mapped_type& at(const key_type& key) const { return At(key); }

  mapped_type& at(const key_type& key) { return At(key); }

  iterator find(const key_type& key) const {
    ListNode node = Search(key);
    return node.IsNone() ? end() : iterator(node.index, this);
  }

  void erase(const iterator& position) {
    uint64_t index = position.index;
    if (position.self != nullptr && index <= this->slots_) {
    }
  }

  iterator begin() const {
    if (slots_ == 0) {
      return iterator(0, this);
    }
    for (uint64_t index = 0; index <= slots_; ++index) {
      if (!ListNode(index, this).IsEmpty()) {
        return iterator(index, this);
      }
    }
    return iterator(slots_ + 1, this);
  }

  iterator end() const { return slots_ == 0 ? iterator(0, this) : iterator(slots_ + 1, this); }

 private:
  ListNode Search(const key_type& key) const {
    if (this->size_ == 0) {
      return ListNode();
    }
    for (ListNode iter = GetListHead(ObjectHash()(key)); !iter.IsNone(); iter.MoveToNext(this)) {
      if (ObjectEqual()(key, iter.Key())) {
        return iter;
      }
    }
    return ListNode();
  }

  mapped_type& At(const key_type& key) const {
    ListNode iter = Search(key);
    ICHECK(!iter.IsNone()) << "IndexError: key is not in Map";
    return iter.Val();
  }

  bool TryInsert(const key_type& key, ListNode* result) {
    if (slots_ == 0) {
      return false;
    }
    // required that `iter` to be the head of a linked list through which we can iterator
    ListNode iter = IndexFromHash(ObjectHash()(key));
    // `iter` can be: 1) empty; 2) body of an irrelevant list; 3) head of the relevant list
    // Case 1: empty
    if (iter.IsEmpty()) {
      iter.NewHead(KVType(key, ObjectRef(nullptr)));
      this->size_ += 1;
      *result = iter;
      return true;
    }
    // Case 2: body of an irrelevant list
    if (!iter.IsHead()) {
      // we move the elements around and construct the single-element linked list
      return IsFull() ? false : TrySpareListHead(iter, key, result);
    }
    // Case 3: head of the relevant list
    // we iterate through the linked list until the end
    // make sure `iter` is the previous element of `next`
    ListNode next = iter;
    do {
      // find equal item, do not insert
      if (ObjectEqual()(key, next.Key())) {
        *result = next;
        return true;
      }
      // make sure `iter` is the previous element of `next`
      iter = next;
    } while (next.MoveToNext(this));
    // `iter` is the tail of the linked list
    // always check capacity before insertion
    if (IsFull()) {
      return false;
    }
    // find the next empty slot
    uint8_t jump;
    if (!iter.GetNextEmpty(this, &jump, result)) {
      return false;
    }
    result->NewTail(KVType(key, ObjectRef(nullptr)));
    // link `iter` to `empty`, and move forward
    iter.SetJump(jump);
    this->size_ += 1;
    return true;
  }

  bool TrySpareListHead(ListNode target, const key_type& key, ListNode* result) {
    // `target` is not the head of the linked list
    // move the original item of `target` (if any)
    // and construct new item on the position `target`
    // To make `target` empty, we
    // 1) find `w` the previous element of `target` in the linked list
    // 2) copy the linked list starting from `r = target`
    // 3) paste them after `w`
    // read from the linked list after `r`
    ListNode r = target;
    // write to the tail of `w`
    ListNode w = target.FindPrev(this);
    // after `target` is moved, we disallow writing to the slot
    bool is_first = true;
    uint8_t r_meta, jump;
    ListNode empty;
    do {
      // `jump` describes how `w` is jumped to `empty`
      // rehash if there is no empty space after `w`
      if (!w.GetNextEmpty(this, &jump, &empty)) {
        return false;
      }
      // move `r` to `empty`
      empty.NewTail(std::move(r.Data()));
      // clear the metadata of `r`
      r_meta = r.Meta();
      if (is_first) {
        is_first = false;
        r.SetProtected();
      } else {
        r.SetEmpty();
      }
      // link `w` to `empty`, and move forward
      w.SetJump(jump);
      w = empty;
      // move `r` forward as well
    } while (r.MoveToNext(this, r_meta));
    // finally we have done moving the linked list
    // fill data_ into `target`
    target.NewHead(KVType(key, ObjectRef(nullptr)));
    this->size_ += 1;
    *result = target;
    return true;
  }

  void ReleaseMemory() {
    delete[] data_;
    data_ = nullptr;
    slots_ = 0;
    size_ = 0;
    fib_shift_ = 63;
  }

  static ObjectPtr<DenseMapNode> Empty(uint32_t fib_shift, uint64_t n_slots) {
    ICHECK_GT(n_slots, uint64_t(SmallMapNode::kMaxSize));
    ObjectPtr<DenseMapNode> p = make_object<DenseMapNode>();
    uint64_t n_blocks = CalcNumBlocks(n_slots - 1);
    Block* block = p->data_ = new Block[n_blocks];
    p->slots_ = n_slots - 1;
    p->size_ = 0;
    p->fib_shift_ = fib_shift;
    for (uint64_t i = 0; i < n_blocks; ++i, ++block) {
      std::fill(block->bytes, block->bytes + kBlockCap, uint8_t(kEmptySlot));
    }
    return p;
  }

  static void InsertMaybeReHash(const KVType& kv, ObjectPtr<Object>* map) {
    DenseMapNode* map_node = static_cast<DenseMapNode*>(map->get());
    ListNode iter;
    // Try to insert. If succeed, we simply return
    if (map_node->TryInsert(kv.first, &iter)) {
      iter.Val() = kv.second;
      return;
    }
    ICHECK_GT(map_node->slots_, uint64_t(SmallMapNode::kMaxSize));
    // otherwise, start rehash
    ObjectPtr<Object> p = Empty(map_node->fib_shift_ - 1, map_node->slots_ * 2 + 2);
    // Insert the given `kv` into the new hash map
    InsertMaybeReHash(kv, &p);
    uint64_t n_blocks = CalcNumBlocks(map_node->slots_);
    // Then Insert data from the original block.
    for (uint64_t bi = 0; bi < n_blocks; ++bi) {
      uint8_t* meta_ptr = map_node->data_[bi].bytes;
      KVType* data_ptr = reinterpret_cast<KVType*>(map_node->data_[bi].bytes + kBlockCap);
      for (int j = 0; j < kBlockCap; ++j, ++meta_ptr, ++data_ptr) {
        uint8_t& meta = *meta_ptr;
        if (meta != uint8_t(kProtectedSlot) && meta != uint8_t(kEmptySlot)) {
          meta = uint8_t(kEmptySlot);
          KVType kv = std::move(*data_ptr);
          InsertMaybeReHash(kv, &p);
        }
      }
    }
    map_node->ReleaseMemory();
    *map = p;
  }

  bool IsFull() const { return size_ + 1 > (slots_ + 1) * kMaxLoadFactor; }

  uint64_t IncItr(uint64_t index) const {
    for (++index; index <= slots_; ++index) {
      if (!ListNode(index, this).IsEmpty()) {
        return index;
      }
    }
    return slots_ + 1;
  }

  uint64_t DecItr(uint64_t index) const {
    while (index != 0) {
      index -= 1;
      if (!ListNode(index, this).IsEmpty()) {
        return index;
      }
    }
    return slots_ + 1;
  }

  KVType* DeRefItr(uint64_t index) const { return &ListNode(index, this).Data(); }

  ListNode IndexFromHash(uint64_t hash_value) const {
    return ListNode(FibHash(hash_value, fib_shift_), this);
  }

  ListNode GetListHead(uint64_t hash_value) const {
    ListNode node = IndexFromHash(hash_value);
    return node.IsHead() ? node : ListNode();
  }

  static uint64_t CalcNumBlocks(uint64_t n_slots_m1) {
    uint64_t n_slots = n_slots_m1 > 0 ? n_slots_m1 + 1 : 0;
    return (n_slots + kBlockCap - 1) / kBlockCap;
  }

  static void CalcTableSize(uint64_t cap, uint32_t* fib_shift, uint64_t* n_slots) {
    uint32_t shift = 64;
    uint64_t slots = 1;
    for (uint64_t c = cap; c; c >>= 1) {
      shift -= 1;
      slots <<= 1;
    }
    ICHECK_GT(slots, cap);
    if (slots < cap * 2) {
      *fib_shift = shift - 1;
      *n_slots = slots << 1;
    } else {
      *fib_shift = shift;
      *n_slots = slots;
    }
  }

  static uint64_t FibHash(uint64_t hash_value, uint32_t fib_shift) {
    constexpr uint64_t coeff = 11400714819323198485ull;
    return (coeff * hash_value) >> fib_shift;
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

    bool HasNext() const { return kNextProbeLocation[Meta() & 0b10000000] != 0; }

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

    ListNode FindPrev(const DenseMapNode* self) const {}

    bool GetNextEmpty(const DenseMapNode* self, uint8_t* jump, ListNode* result) const {
      for (uint8_t idx = 1; idx < kNumJumpDists; ++idx) {
        ListNode candidate((index + kNextProbeLocation[idx]) & (self->slots_), self);
        if (candidate.IsEmpty()) {
          *jump = idx;
          *result = candidate;
          return true;
        }
      }
      return false;
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
      // larger triangle numbers
      8515, 19110, 42778, 96141, 216153,
      486591, 1092981, 2458653, 5532801, 12442566,
      27993903, 62983476, 141717030, 318844378, 717352503,
      1614057336, 3631522476, 8170957530, 18384510628, 41364789378,
      93070452520, 209408356380, 471168559170, 1060128894105, 2385289465695,
      5366898840628, 12075518705635, 27169915244790, 61132312065111, 137547689707000,
      309482283181501, 696335127828753, 1566753995631385, 3525196511162271, 7931691992677701,
      17846306936293605, 40154190677507445, 90346928918121501, 203280589587557251, 457381325854679626,
      1029107982097042876, 2315492959180353330, 5209859154120846435,
  };
  /* clang-format on */
  friend class MapNode;
};

#define CVT_DISPATCH_MAP(base, var, body)     \
  {                                           \
    using TSmall = SmallMapNode*;             \
    using TDense = DenseMapNode*;             \
    uint64_t slots = base->slots_;            \
    if (slots <= SmallMapNode::kMaxSize) {    \
      TSmall var = static_cast<TSmall>(base); \
      body;                                   \
    } else {                                  \
      TDense var = static_cast<TDense>(base); \
      body;                                   \
    }                                         \
  }

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

inline MapNode::iterator::pointer MapNode::iterator::operator->() const {
  CVT_DISPATCH_MAP_CONST(self, p, { return p->DeRefItr(index); });
}

inline MapNode::iterator& MapNode::iterator::operator++() {
  CVT_DISPATCH_MAP_CONST(self, p, {
    index = p->IncItr(index);
    return *this;
  });
}

inline MapNode::iterator& MapNode::iterator::operator--() {
  CVT_DISPATCH_MAP_CONST(self, p, {
    index = p->IncItr(index);
    return *this;
  });
}

inline size_t MapNode::count(const key_type& key) const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->count(key); });
}

inline const MapNode::mapped_type& MapNode::at(const key_type& key) const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->at(key); })
}

inline MapNode::mapped_type& MapNode::at(const key_type& key) {
  CVT_DISPATCH_MAP(this, p, { return p->at(key); })
}

inline MapNode::iterator MapNode::begin() const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->begin(); });
}

inline MapNode::iterator MapNode::end() const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->end(); });
}

inline MapNode::iterator MapNode::find(const key_type& key) const {
  CVT_DISPATCH_MAP_CONST(this, p, { return p->find(key); });
}

#undef CVT_DISPATCH_MAP
#undef CVT_DISPATCH_MAP_CONST

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
  int64_t _cap = std::distance(first, last);
  if (_cap < 0) {
    return SmallMapNode::Empty();
  }
  uint64_t cap = static_cast<uint64_t>(_cap);
  if (cap < SmallMapNode::kMaxSize) {
    return SmallMapNode::CreateFromRange(cap, first, last);
  }
  uint32_t fib_shift;
  uint64_t n_slots;
  DenseMapNode::CalcTableSize(cap, &fib_shift, &n_slots);
  ObjectPtr<Object> obj = DenseMapNode::Empty(fib_shift, n_slots);
  int i = 0;
  for (; first != last; ++first) {
    i++;
    KVType kv(*first);
    DenseMapNode::InsertMaybeReHash(kv, &obj);
  }
  return obj;
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

#endif

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
