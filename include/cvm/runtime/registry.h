//
// Created by WangJingYu on 2021/7/5.
//

#ifndef CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
#define CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_

#include <cvm/runtime/packed_func.h>

namespace cvm {
namespace runtime {

class Registry {
 public:
  CVM_DLL Registry& set_body(PackedFunc f);

  Registry& set_body(PackedFunc::FType f) { return set_body(PackedFunc(std::move(f))); }

  /*!
   * \brief Set the body of the function to the given function.
   *    Note that this will ignore default arg values and always require all arguments to be
   *    provided.
   *
   * \code
   *  int multiply(int x, int y) {
   *    return x * y;
   *  }
   *
   *  CVM_REGISTER_GLOBAL("multiply")
   *  .set_body_typed(multiply);    // will have type int(int, int)
   *
   *  // will have type int(int, int)
   *  CVM_REGISTER_GLOBAL("sub")
   *  .set_body_typed([](int a, int b) -> int {return a - b; });
   *
   * \endcode
   *
   * \tparam FLambda The signature of the function.
   * \param f The function to forward to.
   */
  template <typename FLambda>
  Registry& set_body_typed(FLambda f) {
    using FType = typename detail::function_signature<FLambda>::FType;
  }
  /*!
   * \brief Set the body of the function to be the passed method pointer.
   *    Note that this will ignore default arg values and always require all arguments to be
   *    provided.
   *
   * \code
   *
   * // node subclass:
   * struct Example {
   *   int doThing(int x);
   * }
   * CVM_REGISTER_GLOBAL("Example_doThing")
   * .set_body_method(&Example::doThing);   // will have type int(Example, int)
   *
   * \endcode
   *
   * \tparam T The type containing the method (inferred)
   * \tparam R The return type of the function (inferred)
   * \tparam Args The arguments types of the function (inferred)
   * \param f The method pointer to forward to
   */
  template <typename T, typename R, typename... Args>
  Registry& set_body_method(R (T::*f)(Args...)) {
    auto fwrap = [f](T target, Args... params) -> R {
      // call method pointer
      return (target.*f)(params...);
    };
    return set_body(TypedPackedFunc<R(T, Args...)>(fwrap, name_));
  }

  template <typename T, typename R, typename... Args>
  Registry& set_body_method(R (T::*f)(Args...) const) {
    auto fwrap = [f](const T target, Args... params) -> R {
      // call method pointer
      return (target.*f)(params...);
    };
    return set_body(TypedPackedFunc<R(const T, Args...)>(fwrap, name_));
  }

  template <typename TObjectRef, typename TNode, typename R, typename... Args,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  Registry& set_body_method(R (TNode::*f)(Args...)) {
    auto fwrap = [f](TObjectRef ref, Args... params) {
      TNode* target = ref->operator->();
      // call method pointer
      return (target->*f)(params...);
    };
    return set_body(TypedPackedFunc<R(TObjectRef, Args...)>(fwrap, name_));
  }

  template <typename TObjectRef, typename TNode, typename R, typename... Args,
            typename = typename std::enable_if<std::is_base_of<ObjectRef, TObjectRef>::value>::type>
  Registry& set_body_method(R (TNode::*f)(Args...) const) {
    auto fwrap = [f](TObjectRef ref, Args... params) {
      const TNode* target = ref.operator->();
      // call method pointer
      return (target->*f)(params...);
    };
    return set_body(TypedPackedFunc<R(TObjectRef, Args...)>(fwrap, name_));
  }

  CVM_DLL static Registry& Register(const std::string& name, bool can_override = false);

  CVM_DLL static bool Remove(const std::string& name);

  CVM_DLL static const PackedFunc* Get(const std::string& name);

  CVM_DLL static std::vector<std::string> ListNames();

  class Manager;

 protected:
  std::string name_;
  PackedFunc func_;
};

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_RUNTIME_REGISTRY_H_
