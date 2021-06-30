//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_NODE_H_
#define CVM_INCLUDE_CVM_NODE_NODE_H_

#include <cvm/node/repr_printer.h>
#include <cvm/runtime/object.h>

namespace cvm {

using runtime::Downcast;
using runtime::GetRef;
using runtime::ObjectPtrEqual;
using runtime::ObjectPtrHash;

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_NODE_H_
