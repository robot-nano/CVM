//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_FUNCTOR_H_
#define CVM_INCLUDE_CVM_NODE_FUNCTOR_H_

#include <cvm/runtime/object.h>

#include <vector>

namespace cvm {

using runtime::ObjectRef;

/*!
 * \brief A dynamically dispatched functor on the type of the first argument.
 *
 * This is a class that is useful to construct polymorphic dispatching
 * base on the AST/IR node's type.
 *
 * \code
 *  NodeFunctor<std::string (const ObjectRef& n, std::string prefix)> tostr;
 *  tostr.set_dispatch<Add>([](const ObjectRef& op, std::string prefix) {
 *      return prefix + "Add";
 *  });
 *  tostr.set_dispatch<IntImm>([](const ObjectRef& op, std::string prefix){
 *      return prefix + "IntImm";
 *  });
 *
 *  Expr x = make_const(1);
 *  Expr y = x + x;
 *  // dispatch to IntImm, outputs "MyIntImm"
 *  LOG(INFO) << tostr(x, "My");
 *  // dispatch to MyAdd, outputs "MyAdd"
 *  LOG(INFO) << tostr(y, "My");
 * \endcode
 *
 * \tparam FType function signature
 *  This type if only defined for FType with function signature
 */
template <typename FType>
class NodeFunctor;

template <typename R, typename... Args>
class NodeFunctor<R(const ObjectRef& n, Args...)> {
 private:
  /*! \brief internal function pointer type */
  typedef R (*FPointer)(const ObjectRef& n, Args...);

  using TSelf = NodeFunctor<R(const ObjectRef& n, Args...)>;
  /*! \brief internal function table */
  std::vector<FPointer> func_;

 public:
  /*! \brief the result type of this functor */
  using result_type = R;
  /*!
   * \brief Whether the functor can dispatch the corresponding Node.
   * \param n The node to be dispatched.
   * \return Whether dispatching function is registered for n's type.
   */
  bool can_dispatch(const ObjectRef& n) const {
    uint32_t type_index = n->type_index();
    return type_index < func_.size() && func_[type_index] != nullptr;
  }
  /*!
   * \brief invoke the functor, dispatch on type of n
   * \param n The Node argument
   * \param args The additional arguments
   * \return The result.
   */
  R operator()(const ObjectRef& n, Args... args) const {
    ICHECK(can_dispatch(n)) << "NodeFunctor calls un-registered function on type "
                            << n->GetTypeKey();
    return (*func_[n->type_index()])(n, std::forward<Args>(args)...);
  }
  /*!
   * \brief set the dispatcher for type TNode
   * \tparam TNode the type of Node to be dispatched.
   * \param f The function to be set.
   * \return reference to self.
   */
  template <typename TNode>
  TSelf& set_dispatch(FPointer f) {
    uint32_t tindex = TNode::RuntimeTypeIndex();
    if (func_.size() <= tindex) {
      func_.resize(tindex + 1, nullptr);
    }
    ICHECK(func_[tindex] == nullptr) << "Dispatch for " << TNode::_type_key << " is already set";
    func_[tindex] = f;
    return *this;
  }
  /*!
   * \brief unset the dispatcher for type TNode
   *
   * \tparam TNode the type of Node to be dispatched.
   * \return reference to self.
   */
  template <typename TNode>
  TSelf& clear_dispatch() {
    uint32_t tindex = TNode::RuntimeTypeIndex();
    ICHECK_LT(tindex, func_.size()) << "clear_dispatch: index out of range";
    func_[tindex] = nullptr;
    return *this;
  }
};

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_FUNCTOR_H_
