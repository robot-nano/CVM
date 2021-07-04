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
  const ObjectRef at(int64_t i) const { return this->operator[](i); }
  /*! \return begin constant iterator */
  const ObjectRef* begin() const { return static_cast<ObjectRef*>(InplaceArrayBase::AddressOf(0)); }
  /*! \return end constant iterator */
  const ObjectRef* end() const { return begin() + size_; }
  void clear() {}

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

  ArrayNode* ShrinkBy(int64_t delta) {}

  int64_t size_;
  int64_t capacity_;

  /*! \brief Initial size of ArrayNode */
  static constexpr int64_t kInitSize = 4;
  /*! \brief Expansion factor of the Array */
  static constexpr int64_t kIncFactor = 2;

  // CRTP parent class
  friend InplaceArrayBase<ArrayNode, ObjectRef>;
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_CONTAINER_H_
