//
// Created by WangJingYu on 2021/7/3.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_CONTAINER_H_
#define CVM_INCLUDE_CVM_RUNTIME_CONTAINER_H_

#include <cvm/runtime/memory.h>
#include <cvm/runtime/object.h>

#if defined(__cpp_lib_experimental_string_view) && __cpp_lib_experimental_string_view >= 201411
#define CVM_USE_CXX14_STRING_VIEW_HASH 1
#else
#define CVM_USE_CXX14_STRING_VIEW_HASH 0
#endif

#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201606
#define CVM_USE_CXX17_STRING_VIEW_HASH 1
#else
#define CVM_USE_CXX17_STRING_VIEW_HASH 0
#endif

#if CVM_USE_CXX17_STRING_VIEW_HASH
#include <string_view>
#elif CVM_USE_CXX14_STRING_VIEW_HASH
#include <experimental/string_view>
#endif

#include <type_traits>
#include <utility>
#include <vector>

namespace llvm {
// String to llvm object compatibility.
class StringRef;
}  // namespace llvm

namespace cvm {
namespace runtime {

class CVMArgValue;

/*! \brief String-aware ObjectRef hash functor */
struct ObjectHash {
  /*!
   * \brief Calculate the hash code of an ObjectRef
   * \param a The given ObjectRef
   * \return Hash code of a, string hash for strings and pointer address otherwise.
   */
  size_t operator()(const ObjectRef& a) const;
};

/*! \brief String-aware ObjectRef equal functor */
struct ObjectEqual {
  /*!
   * \brief Check if the two ObjectRef are equal
   * \param a One ObjectRef
   * \param b The other ObjectRef
   * \return String equality if both are strings, pointer address equality otherwise.
   */
  bool operator()(const ObjectRef& a, const ObjectRef& b) const;
};

/*!
 * \brief Base template for classes with array like memory layout.
 *
 *      It provides general methods to access the memory. The memory
 *      layout is ArrayType + [ElemType]. The alignment of ArrayType
 *      and ElemType is handled by the memory allocator.
 *
 * \tparam ArrayType The array header type, contains object specific metadata.
 * \tparam ElemType The type of objects stored in the array right after ArrayType.
 *
 * \code
 *  // Example usage of the template to define a simple array wrapper
 *  class ArrayObj : public InplaceArrayBase<ArrayObj, Elem> {
 *  public:
 *    // wrap EmplaceInit to initialize the elements
 *    template <typename Iterator>
 *    void Init(Iterator begin, Iterator end) {
 *      size_t num_elems = std::distance(begin, end);
 *      auto it = begin;
 *      this->size = 0;
 *      for (size_t i = 0; i < num_elems; ++i) {
 *        InplaceArrayBase::EmplaceInit(i, *it++);
 *        this->size++;
 *      }
 *    }
 *  }
 *
 *  void test_function() {
 *    vector<Elem> fields;
 *    auto ptr = make_inplace_array_object<ArrayObj, Elem>(fields, size());
 *    ptr->Init(fields.begin(), fields.end());
 *
 *    // Access the 0th element in the array.
 *    assert(ptr->operator[](0) == fields[0]);
 *  }
 * \endcode
 */
template <typename ArrayType, typename ElemType>
class InplaceArrayBase {
 public:
  /*!
   * \brief Access element at index
   * \param idx The index of the element.
   * \return Const reference to ElemType at the index.
   */
  const ElemType& operator[](size_t idx) const {
    size_t size = Self()->GetSize();
    ICHECK_LT(idx, size) << "Index " << idx << " out of bounds " << size << "\n";
    return *(reinterpret_cast<ElemType*>(AddressOf(idx)));
  }
  /*!
   * \brief Access element at index
   * \param idx The index of the element.
   * \return Reference to ElemType at the index.
   */
  ElemType& operator[](size_t idx) {
    size_t size = Self()->GetSize();
    ICHECK_LT(idx, size) << "Index " << idx << " out of bounds " << size << "\n";
    return *(reinterpret_cast<ElemType*>(AddressOf(idx)));
  }

  /*!
   * \brief Destroy the Inplace Array Base object
   */
  ~InplaceArrayBase() {
    if (!(std::is_standard_layout<ElemType>::value && std::is_trivial<ElemType>::value)) {
      size_t size = Self()->GetSize();
      for (size_t i = 0; i < size; ++i) {
        ElemType* fp = reinterpret_cast<ElemType*>(AddressOf(i));
        fp->ElemType::~ElemType();
      }
    }
  }

 protected:
  /*!
   * \brief Construct a value in place with arguments.
   *
   * \tparam Args Type parameters of the arguments.
   * \param idx Index of the element.
   * \param args Arguments to construct the new value.
   *
   * \note Please make sure ArrayType::GetSize returns 0 before first call of
   * EmplaceInit, and increment GetSize by 1 each time EmplaceInit succeeds.
   */
  template <typename... Args>
  void EmplaceInit(size_t idx, Args&&... args) {
    void* field_ptr = AddressOf(idx);
    new (field_ptr) ElemType(std::forward<Args>(args)...);
  }

  /*!
   * \brief Return the self object for the array.
   *
   * \return Pointer to ArrayType
   */
  inline ArrayType* Self() const {
    return static_cast<ArrayType*>(const_cast<InplaceArrayBase*>(this));
  }
  /*!
   * \brief Return the raw pointer to the element at idx.
   *
   * \param idx The index of the element.
   * \return Raw pointer to the element.
   */
  void* AddressOf(size_t idx) const {
    static_assert(
        alignof(ArrayType) % alignof(ElemType) == 0 && sizeof(ArrayType) % alignof(ElemType) == 0,
        "The size and alignment of ArrayType should respect "
        "ElemType's alignment.");

    size_t kDataStart = sizeof(ArrayType);
    ArrayType* self = Self();
    char* data_start = reinterpret_cast<char*>(self) + kDataStart;
    return data_start + idx * sizeof(ElemType);
  }
};

template <typename Converter, typename TIter>
class IterAdapter {
 public:
  using difference_type = typename std::iterator_traits<TIter>::difference_type;
  using value_type = typename Converter::ResultType;
  using pointer = typename Converter::ResultType*;
  using reference = typename Converter::ResultType&;
  using iterator_category = typename std::iterator_traits<TIter>::iterator_category;

  explicit IterAdapter(TIter iter) : iter_(iter) {}
  IterAdapter& operator++() {
    ++iter_;
    return *this;
  }
  IterAdapter& operator--() {
    --iter_;
    return *this;
  }
  IterAdapter operator++(int) {
    IterAdapter copy = *this;
    ++iter_;
    return copy;
  }
  IterAdapter operator--(int) {
    IterAdapter copy = *this;
    --iter_;
    return copy;
  }

  IterAdapter operator+(difference_type offset) const { return IterAdapter(iter_ + offset); }

  IterAdapter operator-(difference_type offset) const { return IterAdapter(iter_ - offset); }

  template <typename T = IterAdapter>
  typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                          typename T::difference_type>::type inline
  operator-(const IterAdapter& rhs) const {
    return iter_ - rhs.iter_;
  }

  bool operator==(IterAdapter other) const { return iter_ == other.iter_; }
  bool operator!=(IterAdapter other) const { return !(*this == other); }
  const value_type operator*() const { return Converter::convert(*iter_); }

 private:
  TIter iter_;
};

template <typename Converter, typename TIter>
class ReverseIterAdapter {
 public:
  using difference_type = typename std::iterator_traits<TIter>::difference_type;
  using value_type = typename Converter::ResultType;
  using pointer = typename Converter::ResultType*;
  using reference = typename Converter::ResultType&;
  using iterator_category = typename std::iterator_traits<TIter>::iterator_category;

  explicit ReverseIterAdapter(TIter iter) : iter_(iter) {}
  ReverseIterAdapter& operator++() {
    --iter_;
    return *this;
  }
  ReverseIterAdapter& operator--() {
    ++iter_;
    return *this;
  }
  ReverseIterAdapter operator++(int) {
    ReverseIterAdapter copy = *this;
    --iter_;
    return copy;
  }
  ReverseIterAdapter operator--(int) {
    ReverseIterAdapter copy = *this;
    ++iter_;
    return copy;
  }
  ReverseIterAdapter operator+(difference_type offset) const {
    return ReverseIterAdapter(iter_ - offset);
  }

  template <typename T = ReverseIterAdapter>
  typename std::enable_if<std::is_same<iterator_category, std::random_access_iterator_tag>::value,
                          typename T::difference_type>::type inline
  operator-(const ReverseIterAdapter& rhs) const {
    return rhs.iter_ - iter_;
  }

 private:
  TIter iter_;
};

/*! \brief array node content in array */
class ArrayNode : public Object, public InplaceArrayBase<ArrayNode, ObjectRef> {
 public:
  /*! \return The size of the array */
  size_t size() const { return size_; }
  /*!
   * \brief Read i-th element from array.
   * \param i The index
   * \return the i-th element.
   */
  ObjectRef at(int64_t i) const { return this->operator[](i); }
  /*! \return begin constant iterator */
  const ObjectRef* begin() const { return static_cast<ObjectRef*>(InplaceArrayBase::AddressOf(0)); }
  /*! \return end constant iterator */
  const ObjectRef* end() const { return begin() + size_; }
  /*! \brief Release reference to all the elements */
  void clear() { ShrinkBy(size_); }
  /*!
   * \brief Set i-th element of the array in-place
   * \param i The index
   * \param item The value to be set
   */
  void SetItem(int64_t i, ObjectRef item) { this->operator[](i) = std::move(item); }
  /*!
   * \brief Constructs a container and copy from another
   * \param cap The capacity of the container
   * \param from Source of the copy
   * \return Ref-counted ArrayNode requested
   */
  static ObjectPtr<ArrayNode> CopyFrom(int64_t cap, ArrayNode* from) {
    int64_t size = from->size_;
    ICHECK_GE(cap, size) << "ValueError: not enough capacity";
    ObjectPtr<ArrayNode> p = ArrayNode::Empty(cap);
    ObjectRef* write = p->MutableBegin();
    const ObjectRef* read = from->begin();
    // To ensure safety, size is only incremented after the initialization succeeds.
    for (int64_t& i = p->size_ = 0; i < size; ++i) {
      new (write++) ObjectRef(*read++);
    }
    return p;
  }
  /*!
   * \brief Constructs a container and move from another
   * \param cap The capacity of the container
   * \param from Source of the move
   * \return Ref-Counted ArrayNode requested
   */
  static ObjectPtr<ArrayNode> MoveFrom(int64_t cap, ArrayNode* from) {
    int64_t size = from->size_;
    ICHECK_GE(cap, size) << "ValueError: not enough capacity";
    ObjectPtr<ArrayNode> p = ArrayNode::Empty(cap);
    ObjectRef* write = p->MutableBegin();
    ObjectRef* read = from->MutableBegin();
    for (int64_t& i = p->size_ = 0; i < size; ++i) {
      new (write++) ObjectRef(std::move(*read++));
    }
    from->size_ = 0;
    return p;
  }
  /*!
   * \brief Constructs a container with n elements. Each element is a copy of val
   * \param n The size of the container
   * \param val The init value
   * \return Ref-counted ArrayNode requested
   */
  static ObjectPtr<ArrayNode> CreateRepeated(int64_t n, const ObjectRef& val) {
    ObjectPtr<ArrayNode> p = ArrayNode::Empty(n);
    ObjectRef* itr = p->MutableBegin();
    for (int64_t& i = p->size_ = 0; i < n; ++i) {
      new (itr++) ObjectRef(val);
    }
    return p;
  }

  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeArray;
  static constexpr const char* _type_key = "Array";
  CVM_DECLARE_FINAL_OBJECT_INFO(ArrayNode, Object);

 private:
  /*! \brief Size of initialized memory, used by InplaceArrayBase. */
  size_t GetSize() const { return this->size_; }

  /*! \return begin mutable iterator */
  ObjectRef* MutableBegin() const {
    return static_cast<ObjectRef*>(InplaceArrayBase::AddressOf(0));
  }

  /*! \return end mutable iterator */
  ObjectRef* MutableEnd() const { return MutableBegin() + size_; }

  static ObjectPtr<ArrayNode> Empty(int64_t n = kInitSize) {
    ICHECK_GE(n, 0);
    ObjectPtr<ArrayNode> p = make_inplace_array_object<ArrayNode, ObjectRef>(n);
    p->capacity_ = n;
    p->size_ = 0;
    return p;
  }
  /*!
   * \brief Inplace-initialize the elements starting idx from [first, last)
   * \tparam IterType The type of iterator
   * \param idx The starting point
   * \param first Begin of iterator
   * \param last End of iterator
   * \return Self
   */
  template <typename IterType>
  ArrayNode* InitRange(int64_t idx, IterType first, IterType last) {
    ObjectRef* itr = MutableBegin() + idx;
    for (; first != last; ++first) {
      ObjectRef ref = *first;
      new (itr++) ObjectRef(std::move(ref));
    }
    return this;
  }
  /*!
   * \brief Move elements from right to left, requires src_begin > dst
   * \param dst Destination
   * \param src_begin The start point of copy (inclusive)
   * \param src_end The end point of copy (exclusive)
   * \return Self
   */
  ArrayNode* MoveElementsLeft(int64_t dst, int64_t src_begin, int64_t src_end) {
    ObjectRef* from = MutableBegin() + src_begin;
    ObjectRef* to = MutableBegin() + dst;
    while (src_begin++ != src_end) {
      *to++ = std::move(*from++);
    }
    return this;
  }
  /*!
   * \brief Move elements from left to right, requires src_begin < dst
   * \param dst Destination
   * \param src_begin The start point of move (inclusive)
   * \param src_end The end point of move (exclusive)
   * \return Self
   */
  ArrayNode* MoveElementsRight(int64_t dst, int64_t src_begin, int64_t src_end) {
    ObjectRef* from = MutableBegin() + src_end;
    ObjectRef* to = MutableBegin() + (src_end - src_begin + dst);
    while (src_begin++ != src_end) {
      *--to = std::move(*--from);
    }
    return this;
  }
  /*!
   * \brief Enlarges the size of the array
   * \param delta Size enlarged, should be positive
   * \param val Default value
   * \return Self
   */
  ArrayNode* EnlargeBy(int64_t delta, const ObjectRef& val = ObjectRef(nullptr)) {
    ObjectRef* itr = MutableEnd();
    while (delta-- > 0) {
      new (itr++) ObjectRef(val);
      ++size_;
    }
    return this;
  }
  /*!
   * \brief Shrinks the size of the the array
   * \param delta Size shrinked, should be positive
   * \return Self
   */
  ArrayNode* ShrinkBy(int64_t delta) {
    ObjectRef* itr = MutableEnd();
    while (delta-- > 0) {
      (--itr)->ObjectRef::~ObjectRef();
      --size_;
    }
    return this;
  }

  int64_t size_;
  int64_t capacity_;

  /*! \brief Initial size of ArrayNode */
  static constexpr int64_t kInitSize = 4;
  /*! \brief Expansion factor of the Array */
  static constexpr int64_t kIncFactor = 2;

  // CRTP parent class
  friend InplaceArrayBase<ArrayNode, ObjectRef>;

  // Reference class
  template <typename, typename>
  friend class Array;

  // To specialize make_object<ArrayNode>
  friend ObjectPtr<ArrayNode> make_object<>();
};

/*!
 * \brief Array, container representing a continuous sequence of ObjectRefs.
 *
 *  Array implements in-place copy-on-write semantics.
 *
 * As in typical copy-on-write, a method which would typically mutate the array
 * instead opaquely copies the underlying container, and then acts on its copy.
 *
 * If the array has reference count equal to one, we directly update the
 * container in place without copying. This is optimization is sound because
 * when the reference count is equal to one this reference is guaranteed to be
 * the sole pointer to the container.
 *
 * operator[] only provides const access, use Set to mutate the content.
 * \tparam T The content ObjectRef type.
 */
template <typename T,
          typename = typename std::enable_if<std::is_base_of<ObjectRef, T>::value>::type>
class Array : public ObjectRef {
 public:
  using value_type = T;
  /*! \brief default constructor */
  Array() { data_ = ArrayNode::Empty(); }
  /*!
   * \brief move constructor
   * \param other source
   */
  Array(Array<T>&& other) : ObjectRef() {  // NOLINT
    data_ = std::move(other.data_);
  }
  /*!
   * \brief copy constructor
   * \param other source
   */
  Array(const Array<T>& other) : ObjectRef() {  // NOLINT
    data_ = other.data_;
  }
  /*!
   * \brief constructor from pointer
   * \param n the container pointer
   */
  explicit Array(ObjectPtr<Object> n) : ObjectRef(n) {}
  /*!
   * \brief Constructor from iterator
   * \tparam IterType The type of iterator
   * \param first begin of iterator
   * \param last end of iterator
   */
  template <typename IterType>
  Array(IterType first, IterType last) {
    Assign(first, last);
  }
  /*!
   * \brief Constructor from initializer list
   * \param init The initializer list
   */
  Array(std::initializer_list<T> init) { Assign(init.begin(), init.end()); }
  /*!
   * \brief Constructor from vector
   * \param init The init vector
   */
  Array(const std::vector<T>& init) {  // NOLINT
    Assign(init.begin(), init.end());
  }
  /*!
   * \brief Constructs a container with n elements. Each element is a copy of val
   * \param n The size of the container
   * \param val The init value
   */
  explicit Array(const size_t n, const T& val) { data_ = ArrayNode::CreateRepeated(n, val); }
  /*!
   * \brief move assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(Array<T>&& other) {
    data_ = std::move(other.data_);
    return *this;
  }
  /*!
   * \brief Copy assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(const Array<T>& other) {
    data_ = other.data_;
    return *this;
  }

 public:
  // iterators
  struct ValueConverter {
    using ResultType = T;
    static T convert(const ObjectRef& n) { return DowncastNoCheck<T>(n); }
  };

  using iterator = IterAdapter<ValueConverter, const ObjectRef*>;
  using reverse_iterator = ReverseIterAdapter<ValueConverter, const ObjectRef*>;

  /*! \return begin iterator */
  iterator begin() const { return iterator(GetArrayNode()->begin()); }

  /*! \return end iterator */
  iterator end() const { return iterator(GetArrayNode()->end()); }

  /*! \return rbegin iterator */
  reverse_iterator rbegin() const {
    // ArrayNode:end() is never nullptr
    return reverse_iterator(GetArrayNode()->end() - 1);
  }

  /*! \return rend iterator */
  reverse_iterator rend() const {
    // ArrayNode:begin() is never nullptr
    return reverse_iterator(GetArrayNode()->begin() - 1);
  }

  /*!
   * \brief Immutably read i-th element from array.
   * \param i The index
   * \return the i-th element.
   */
  const T operator[](int64_t i) const {
    ArrayNode* p = GetArrayNode();
    ICHECK(p != nullptr) << "ValueError: cannot index a null array";
    ICHECK(0 <= i && i < p->size_)
        << "IndexError: indexing " << i << " on an array of size " << p->size_;
    return DowncastNoCheck<T>(*(p->begin() + i));
  }

  /*! \return The size of the array */
  size_t size() const {
    ArrayNode* p = GetArrayNode();
    return p == nullptr ? 0 : GetArrayNode()->size_;
  }

  /*! \brief The capacity of the array */
  size_t capacity() const {
    ArrayNode* p = GetArrayNode();
    return p == nullptr ? 0 : p->capacity_;
  }

  /*! \return Whether array is empty */
  bool empty() const { return size() == 0; }

  /*! \return The first element of the array */
  const T front() const {
    ArrayNode* p = GetArrayNode();
    ICHECK(p != nullptr) << "ValueError: cannot index a null array";
    ICHECK_GT(p->size_, 0) << "IndexError: cannot index an empty array";
    return DowncastNoCheck<T>(*(p->begin()));
  }

  /*! \return The underlying ArrayNode */
  ArrayNode* GetArrayNode() const { return static_cast<ArrayNode*>(data_.get()); }

  /*!
   * \brief reset the array to content from iterator.
   * \tparam IterType The type of iterator
   * \param first begin of iterator
   * \param last end of iterator
   */
  template <typename IterType>
  void Assign(IterType first, IterType last) {
    int64_t cap = std::distance(first, last);
    ICHECK_GE(cap, 0) << "ValueError: cannot construct an Array of negative size";
    ArrayNode* p = GetArrayNode();
    if (p != nullptr && data_.unique() && p->capacity_ >= cap) {
      // do not have to make new space
      p->clear();
    } else {
      // create new space
      data_ = ArrayNode::Empty(cap);
      p = GetArrayNode();
    }
    // To ensure exception safety, size is only incremented after the initialization succeeds
    ObjectRef* itr = p->MutableBegin();
    for (int64_t& i = p->size_ = 0; i < cap; ++i, ++first, ++itr) {
      new (itr) ObjectRef(*first);
    }
  }

  /*!
   * \brief Copy on write semantics
   *  Do nothing if current handle is the unique copy of the array.
   *  Otherwise make a new copy of the array to ensure the current handle
   *  hold a unique copy.
   *
   * \return Handle to the internal node container(which guarantees to unique)
   */
  ArrayNode* CopyOnWrite() {
    if (data_ == nullptr) {
      return SwitchContainer(ArrayNode::kInitSize);
    }
    if (!data_.unique()) {
      return SwitchContainer(capacity());
    }
    return static_cast<ArrayNode*>(data_.get());
  }

  /*! \brief specify container node */
  using ContainerType = ArrayNode;

 private:
  /*!
   * \brief Implement copy-on-write semantics, and ensures capacity is enough for extra elements.
   * \param reverse_extra Number of extra slots needed
   * \return ArrayNode pointer to the unique copy
   */
  ArrayNode* CopyOnWrite(int64_t reverse_extra) {
    ArrayNode* p = GetArrayNode();
    if (p == nullptr) {
      // necessary to get around the constexpr address issue before c++17
      const int64_t kInitSize = ArrayNode::kInitSize;
      return SwitchContainer(std::max(kInitSize, reverse_extra));
    }
    if (p->capacity_ >= p->size_ + reverse_extra) {
      return CopyOnWrite();
    }
    int64_t cap = p->capacity_ * ArrayNode::kIncFactor;
    cap = std::max(cap, p->size_ + reverse_extra);
    return SwitchContainer(cap);
  }
  /*!
   * \brief Move or copy the ArrayNode to new
   * \param capacity The capacity requirement of the new address
   */
  ArrayNode* SwitchContainer(int64_t capacity) {
    if (data_ == nullptr) {
      data_ = ArrayNode::Empty(capacity);
    } else if (data_.unique()) {
      data_ = ArrayNode::MoveFrom(capacity, GetArrayNode());
    } else {
      data_ = ArrayNode::CopyFrom(capacity, GetArrayNode());
    }
    return static_cast<ArrayNode*>(data_.get());
  }
};

class StringObj : public Object {
 public:
  const char* data;
  uint64_t size;

  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeString;
  static constexpr const char* _type_key = "runtime.String";
  CVM_DECLARE_FINAL_OBJECT_INFO(StringObj, Object);

 private:
  class FromStd;

  friend class String;
};

class String : public ObjectRef {
 public:
  String() : String(std::string()) {}

  String(std::string other);

  String(const char* other)  // NOLINT
      : String(std::string(other)) {}

  inline String& operator=(std::string other);

  inline String& operator=(const char* other);

  int compare(const String& other) const {
    return memncmp(data(), other.data(), size(), other.size());
  }

  int compare(const std::string& other) const {
    return memncmp(data(), other.data(), size(), other.size());
  }

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
      throw std::out_of_range("cvm::String index out of bounds");
    }
  }

  const char* data() const { return get()->data; }

  operator std::string() const { return std::string(get()->data, size()); }  // NOLINT

  inline operator llvm::StringRef() const;

  inline static bool CanConvertFrom(const CVMArgValue& val);

  static size_t HashBytes(const char* data, size_t size) {
#if CVM_USE_CXX17_STRING_VIEW_HASH
    return std::hash<std::string_view>()(std::string_view(data, size));
#elif CVM_USE_CXX14_STRING_VIEW_HASH
    return std::hash<std::experimental::string_view>()(std::experimental::string_view(data, size));
#else
    return std::hash<std::string>()(std::string(data, size));
#endif
  }

  CVM_DEFINE_NOTNULLABLE_OBJECT_REF_METHOD(String, ObjectRef, StringObj);

 private:
  static int memncmp(const char* lhs, const char* rhs, size_t lhs_count, size_t rhs_count);

  static String Concat(const char* lhs, size_t lhs_size, const char* rhs, size_t rhs_size) {
    std::string ret(lhs, lhs_size);
    ret.append(rhs, rhs_size);
    return String(ret);
  }
};

class StringObj::FromStd : public StringObj {
 public:
  explicit FromStd(std::string other) : data_container(std::move(other)) {}

 private:
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

struct NullOptType {};

/*!
 * \brief Optional container that to represent to a Nullable variant of T.
 * \tparam T The original ObjectRef.
 *
 * \code
 *
 *  Optional<String> opt0 = nullptr;
 *  Optional<String> opt1 = String("xyz");
 *  ICHECK(opt0 == nullptr);
 *  ICHECK(opt1 == "xyz");
 *
 * \endcode
 */
template <typename T>
class Optional : public ObjectRef {
 public:
  using ContainerType = typename T::ContainerType;
  static_assert(std::is_base_of<ObjectRef, T>::value, "Optional is only defined for ObjectRef.");

  Optional() = default;
  Optional(const Optional<T>&) = default;
  Optional(Optional<T>&&) = default;
  Optional<T>& operator=(const Optional<T>&) = default;
  Optional<T>& operator=(Optional<T>&&) = default;

  explicit Optional(ObjectPtr<Object> ptr) : ObjectRef(ptr) {}

  Optional(NullOptType) {}  // NOLINT(*)

  explicit Optional(std::nullptr_t) {}

  Optional<T>& operator=(std::nullptr_t) {
    data_ = nullptr;
    return *this;
  }

  Optional(T other)  // NOLINT
      : ObjectRef(std::move(other)) {}
  Optional<T>& operator=(T other) {
    ObjectRef::operator=(std::move(other));
    return *this;
  }

  explicit Optional(int val) = delete;
  Optional<T>& operator=(int val) = delete;

  T value() const {
    ICHECK(data_ != nullptr);
    return T(data_);
  }

  T value_or(T default_value) const { return data_ != nullptr ? T(data_) : default_value; }

  explicit operator bool() const { return *this != nullptr; }

  bool operator==(std::nullptr_t) const { return data_ == nullptr; }
  bool operator!=(std::nullptr_t) const { return data_ != nullptr; }
  auto operator==(const Optional<T>& other) const {
    using RetType = decltype(value() == other.value());
    if (same_as(other)) return RetType(true);
    if (*this != nullptr && other != nullptr) {
      return value() == other.value();
    } else {
      return RetType(false);
    }
  }
  auto operator!=(const Optional<T>& other) const {
    using RetType = decltype(value() != other.value());
    if (same_as(other)) return RetType(false);
    if (*this != nullptr && other != nullptr) {
      return value() != other.value();
    } else {
      return RetType(true);
    }
  }
  auto operator==(const T& other) const {
    using RetType = decltype(value() == other);
    if (same_as(other)) return RetType(true);
    if (*this != nullptr) return value() == other;
    return RetType(false);
  }
  auto operator!=(const T& other) const { return !(*this == other); }
  template <typename U>
  auto operator==(const U& other) const {
    using RetType = decltype(value() == other);
    if (*this == nullptr) return RetType(false);
    return value() == other;
  }
  template <typename U>
  auto operator!=(const U& other) const {
    using RetType = decltype(value() != other);
    if (*this == nullptr) return RetType(true);
    return value() != other;
  }
  static constexpr bool _type_is_nullable = true;
};

class ClosureObj : public  Object {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeClosure;
  static constexpr const char* _type_key = "runtime.Closure";
  CVM_DECLARE_BASE_OBJECT_INFO(ClosureObj, Object);
};

class Closure : public ObjectRef {
 public:
  CVM_DEFINE_OBJECT_REF_METHOD(Closure, ObjectRef, ClosureObj);
};



}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_CONTAINER_H_
