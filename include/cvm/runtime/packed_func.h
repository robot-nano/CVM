#ifndef CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
#define CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/container.h>
#include <cvm/runtime/data_type.h>
#include <cvm/runtime/logging.h>
#include <cvm/runtime/module.h>
#include <cvm/runtime/ndarray.h>
#include <cvm/runtime/object.h>

#include <functional>
#include <limits>

namespace cvm {
namespace runtime {

// forward declarations
class CVMArgs;
class CVMArgValue;
class CVMMovableArgValueWithContext_;
class CVMRetValue;
class CVMArgsSetter;

class PackedFunc {
 public:
  using FType = std::function<void(CVMArgs args, CVMRetValue* rv)>;
  /*! \brief default constructor */
  PackedFunc() = default;
  /*! \brief constructor from null */
  PackedFunc(std::nullptr_t null) {}  // NOLINT(*)
  /*!
   * \brief constructing a packed function from a std::function.
   * \param body the internal container of packed function.
   */
  explicit PackedFunc(FType body) : body_(std::move(body)) {}
  /*!
   * \brief Call packed function by directly passing in unpacked format.
   * \tparam Args arguments to be passed
   * \param args Arguments to be passed
   *
   * \code
   *    // Example code on how to call packed function
   *    void CallPacked(PackedFunc f) {
   *        // call like normal functions by pass in arguments
   *        // return value is automatically converted back
   *        int rvalue = f(1, 2.0);
   *    }
   * \endcode
   */
  template <typename... Args>
  inline CVMRetValue operator()(Args&&... args) const;
  /*!
   * \brief Call the function in packed format.
   * \param args The arguments.
   * \param rv The return value.
   */
  inline void CallPacked(CVMArgs args, CVMRetValue* rv) const;
  /*! \return the internal body function */
  inline FType body() const;
  /*! \return Whether the packed function is nullptr */
  bool operator==(std::nullptr_t null) const { return body_ == nullptr; }
  /*! \return Whether the packed function is not nullptr */
  bool operator!=(std::nullptr_t null) const { return body_ != nullptr; }

 private:
  FType body_;
};


class CVMArgs {
 public:

};

class CVMRetValue {

};

namespace detail {

template <typename F, typename... Args>
inline void for_each(const F& f, Args&&... args) {

}

}  // namespace detail

class CVMArgsSetter {
 public:
  CVMArgsSetter(CVMValue* values, int* type_codes) : values_(values), type_codes_(type_codes) {}

 private:
  CVMValue* values_;
  int* type_codes_;
};

template <typename... Args>
inline CVMRetValue PackedFunc::operator()(Args&&... args) const {
  const int kNumArgs = sizeof...(Args);
  const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;
  CVMValue values[kArraySize];
  int type_codes[kArraySize];
  detail::for_each(CVMArgsSetter(values, type_codes), std::forward<Args>(args)...);
  CVMRetValue rv;
  body_(CVMArgs(values, type_codes, kNumArgs), &rv);
  return rv;
}

inline void PackedFunc::CallPacked(CVMArgs args, CVMRetValue* rv) const { body_(args, rv); }

inline PackedFunc::FType PackedFunc::body() const { return body_; }

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
