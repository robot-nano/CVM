//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_NODE_H_
#define CVM_INCLUDE_CVM_NODE_NODE_H_

#include <cvm/node/reflection.h>
#include <cvm/node/repr_printer.h>
#include <cvm/runtime/object.h>
#include <cvm/runtime/packed_func.h>

namespace cvm {

using runtime::CVMArgs;
using runtime::CVMRetValue;
using runtime::Downcast;
using runtime::GetRef;
using runtime::Object;
using runtime::ObjectPtrEqual;
using runtime::ObjectPtrHash;
using runtime::ObjectRef;

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_NODE_H_
