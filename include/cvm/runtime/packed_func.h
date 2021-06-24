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

#define CVM_CHECK_TYPE_CODE(CODE, T) ICHECK_EQ(CODE, T) << "expected "

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

  PackedFunc() = default;

  PackedFunc(std::nullptr_t null) {}  // NOLINT(*)

  explicit PackedFunc(FType body) : body_(std::move(body)) {}

  template <typename... Args>
  inline CVMRetValue operator()(Args&&... args);

  bool operator==(std::nullptr_t null) const { return body_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return body_ != nullptr; }

 private:
  FType body_;
};

template <typename FType>
class TypedPackedFunc;

template <typename R, typename... Args>
class TypedPackedFunc<R(Args...)> {
 public:
  using TSelf = TypedPackedFunc<R(Args...)>;

  TypedPackedFunc() = default;

  TypedPackedFunc(std::nullptr_t null) {}  // NOLINT(*)

  inline TypedPackedFunc(PackedFunc packed);  // NOLINT(*)

  inline TypedPackedFunc(const CVMRetValue& value);  // NOLINT(*)

  inline TypedPackedFunc(const CVMArgValue& value);  // NOLINT(*)

  inline TypedPackedFunc(CVMMovableArgValueWithContext_&& value);  // NOLINT(*)

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda, std::string name) {
    this->template AssignTypedLambda(typed_lambda, name);
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TypedPackedFunc(const FLambda& typed_lambda) {  // NOLINT(*)
    this->template AssignTypedLambda(typed_lambda);
  }

  template <typename FLambda, typename = typename std::enable_if<std::is_convertible<
                                  FLambda, std::function<R(Args...)>>::value>::type>
  TSelf& operator=(FLambda typed_lambda) {  // NOLINT(*)
    this->template AssignTypedLambda(typed_lambda);
    return *this;
  }

  TSelf& operator=(PackedFunc packed) {  // NOLINT(*)
    packed_ = packed;
    return *this;
  }

  CVM_ALWAYS_INLINE R operator()(Args&&... args) const;

  operator PackedFunc() const { return packed(); }  // NOLINT(*)

  const PackedFunc& packed() const { return packed_; }

  bool operator==(std::nullptr_t null) const { return packed_ == nullptr; }
  bool operator!=(std::nullptr_t null) const { return packed_ != nullptr; }

 private:
  friend class CVMRetValue;
  PackedFunc packed_;

  template <typename FLambda>
  inline void AssignTypedLambda(FLambda flambda, std::string name);

  template <typename FLambda>
  inline void AssignTypedLambda(FLambda flambda);
};

class CVMArgs {
 public:
  CVMArgs(const CVMValue* values, const int* type_codes, int num_args)
      : values(values), type_codes(type_codes), num_args(num_args) {}

  inline int size() const;

  inline CVMArgValue operator[](int i) const;

  const CVMValue* values;
  const int* type_codes;
  int num_args;
};

class CVMPODValue_ {
 public:
  template <typename T>
  T* ptr() const {
    return static_cast<T*>(value_.v_handle);
  }

 protected:
  CVMPODValue_() : type_code_(kCVMNullptr) {}
  CVMPODValue_(CVMValue value, int type_code) : value_(value), type_code_(type_code) {}
  CVMValue value_;
  int type_code_;
};

class CVMArgValue : public CVMPODValue_ {
 public:
  operator PackedFunc() const {  // NOLINT(*)
    if (type_code_ == kCVMNullptr) return PackedFunc();
    CVM_CHECK_TYPE_CODE(type_code_, kCVMPackedFuncHandle);
    return *ptr<PackedFunc>();
  }
};

class CVMMovableArgValue_ : public CVMPODValue_ {
 public:
  CVMMovableArgValue_(CVMValue value, int type_code) : CVMPODValue_(value, type_code) {}
};

class CVMMovableArgValueWithContext_ {
 public:
  CVMMovableArgValueWithContext_(CVMValue value, int type_code, int arg_index,
                                 const std::string* optional_name)
      : value_(value, type_code), arg_index_(arg_index), optional_name_(optional_name) {}

  template <typename T>
  operator T() const {  // NOLINT(*)
    try {
      return value_;
    } catch (Error& e) {
      LOG(FATAL) << "In function " << (optional_name_ == nullptr ? "<anonymous>" : *optional_name_)
                 << ": error while converting argument " << arg_index_ << ": " << e.what();
      throw;
    }
  }

 private:
  CVMMovableArgValue_ value_;
  int arg_index_;
  const std::string* optional_name_;
};

class CVMRetValue : public CVMPODValue_ {
 public:
  CVMRetValue() = default;

  operator PackedFunc() const {  // NOLINT(*)
    if (type_code_ == kCVMNullptr) return PackedFunc();
    CVM_CHECK_TYPE_CODE(type_code_, kCVMPackedFuncHandle);
    return *ptr<PackedFunc>();
  }
  template <typename FType>
  operator TypedPackedFunc<FType>() const {  // NOLINT(*)
    return TypedPackedFunc<FType>(operator PackedFunc());
  }
};

class CVMArgsSetter {
 public:
  CVMArgsSetter(CVMValue* values, int* type_codes) : values_(values), type_codes_(type_codes) {}

  template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
  CVM_ALWAYS_INLINE void operator()(std::size_t i, T value) const {
    values_[i].v_int64 = static_cast<int64_t>(value);
    type_codes_[i] = kDLInt;
  }

 private:
  CVMValue* values_;
  int* type_codes_;
};

namespace detail {

template <bool stop, std::size_t I, typename F>
struct for_each_dispatcher {
  template <typename T, typename... Args>
  static void run(const F& f, T&& value, Args&&... args) {
    f(I, std::forward<T>(value));
    for_each_dispatcher<sizeof...(Args) == 0, (I + 1), F>::run(f, std::forward<Args>(args)...);
  }
};

template <std::size_t I, typename F>
struct for_each_dispatcher<true, I, F> {
  static void run(const F& f) {}
};

template <typename F, typename... Args>
void for_each(const F& f, Args&&... args) {
  for_each_dispatcher<sizeof...(Args) == 0, 0, F>::run(f, std::forward<Args>(args)...);
}

}  // namespace detail

template <typename... Args>
inline CVMRetValue PackedFunc::operator()(Args&&... args) {
  const int kNumArgs = sizeof...(Args);
  const int kArraySize = kNumArgs > 0 ? kNumArgs : 1;
  CVMValue values[kArraySize];
  int type_codes[kArraySize];
  detail::for_each(CVMArgsSetter(values, type_codes), std::forward<Args>(args)...);
  CVMRetValue rv;
  body_(CVMArgs(values, type_codes, kNumArgs), &rv);
  return rv;
}

namespace detail {

template <typename R, int nleft, int index, typename F>
struct unpack_call_dispatcher {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpack_args) {
    unpack_call_dispatcher<R, nleft - 1, index + 1, F>::run(
        optional_name, f, args_pack, std::forward<Args>(unpack_args)...,
        CVMMovableArgValueWithContext_(args_pack.values[index], args_pack.type_codes[index], index,
                                       optional_name));
  }
};

template <typename R, int index, typename F>
struct unpack_call_dispatcher<R, 0, index, F> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpack_args) {
    using RetType = decltype(f(std::forward<Args>(unpack_args)...));
    if (std::is_same<RetType, R>::value) {
      *rv = f(std::forward<Args>(unpack_args)...);
    } else {
      *rv = R(f(std::forward<Args>(unpack_args)...));
    }
  }
};

template <int index, typename F>
struct unpack_call_dispatcher<void, 0, index, F> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const std::string* optional_name, const F& f,
                                    const CVMArgs& args_pack, CVMRetValue* rv,
                                    Args&&... unpacked_args) {
    f(std::forward<Args>(unpacked_args)...);
  }
};

template <typename R, int nargs, typename F>
CVM_ALWAYS_INLINE void unpack_call(const std::string* optional_name, const F& f,
                                   const CVMArgs& args, CVMRetValue* rv) {
  ICHECK_EQ(nargs, args.size()) << "Function "
                                << (optional_name == nullptr ? "<anonymous>" : *optional_name)
                                << " expects " << nargs << " arguments but " << args.size()
                                << " were provided";
}

template <typename R>
struct typed_packed_call_dispatcher {
  template <typename... Args>
  CVM_ALWAYS_INLINE static R run(const PackedFunc& pf, Args&&... args) {
    return pf(std::forward<Args>(args)...);
  }
};

template <>
struct typed_packed_call_dispatcher<void> {
  template <typename... Args>
  CVM_ALWAYS_INLINE static void run(const PackedFunc& pf, Args&&... args) {
    pf(std::forward<Args>(args)...);
  }
};
}  // namespace detail

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(PackedFunc packed) : packed_(std::move(packed)) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const CVMRetValue& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(const CVMArgValue& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
TypedPackedFunc<R(Args...)>::TypedPackedFunc(CVMMovableArgValueWithContext_&& value)
    : packed_(value.operator PackedFunc()) {}

template <typename R, typename... Args>
template <typename FType>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda, std::string name) {
  packed_ = PackedFunc([flambda, name](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function " << name << " expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided.";
      detail::unpack_call<R, sizeof...(Args)>(&name, flambda, args, rv);
    }
  });
}

template <typename R, typename... Args>
template <typename FType>
inline void TypedPackedFunc<R(Args...)>::AssignTypedLambda(FType flambda) {
  packed_ = PackedFunc([flambda](const CVMArgs& args, CVMRetValue* rv) {
    if (args.size() != sizeof...(Args)) {
      LOG(FATAL) << "Function <anonymous> expects " << sizeof...(Args) << " arguments, but "
                 << args.size() << " were provided.";
    }
    detail::unpack_call<R, sizeof...(Args)>(nullptr, flambda, args, rv);
  });
}

template <typename R, typename... Args>
CVM_ALWAYS_INLINE R TypedPackedFunc<R(Args...)>::operator()(Args&&... args) const {
  return detail::typed_packed_call_dispatcher<R>::run(packed_, std::forward<Args>(args)...);
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_PACKED_FUNC_H_
