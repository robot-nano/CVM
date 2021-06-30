//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_REPR_PRINTER_H_
#define CVM_INCLUDE_CVM_NODE_REPR_PRINTER_H_

#include <cvm/node/functor.h>

#include <iostream>

namespace cvm {
/*! \brief A printer class to print the AST/IR nodes. */
class ReprPrinter {
 public:
  /*! \brief The output stream */
  std::ostream& stream;
  /*! \brief The indentation level. */
  int indent{0};

  explicit ReprPrinter(std::ostream& stream) : stream(stream) {}

  /*! \brief The node to be printed. */
  CVM_DLL void Print(const ObjectRef& node);
  /*! \brief Print indent to the stream */
  CVM_DLL void PrintIndent();
  // Allow registration to be printer.
  using FType = NodeFunctor<void(const ObjectRef&, ReprPrinter*)>;
  CVM_DLL static FType& vtable();
};

}  // namespace cvm

namespace cvm {
namespace runtime {

inline std::ostream& operator<<(std::ostream& os, const ObjectRef& n) {
  ReprPrinter(os).Print(n);
  return os;
}

}  // namespace runtime
}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_REPR_PRINTER_H_
