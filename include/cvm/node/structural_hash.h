//
// Created by WangJingYu on 2021/6/30.
//

#ifndef CVM_INCLUDE_CVM_NODE_STRUCTURAL_HASH_H_
#define CVM_INCLUDE_CVM_NODE_STRUCTURAL_HASH_H_

namespace cvm {

/*!
 * \brief Hash definition of base value classes.
 */
class BaseValueHash {
 public:
  size_t operator()(const double& key) const { return std::hash<double>()(key); }
  size_t operator()(const int64_t& key) const { return std::hash<int64_t>()(key); }
  size_t operator()(const uint64_t& key) const { return std::hash<uint64_t>()(key); }
  size_t operator()(const int& key) const { return std::hash<int>()(key); }
  size_t operator()(const bool& key) const { return std::hash<bool>()(key); }
  size_t operator()(const std::string& key) const { return std::hash<std::string>()(key); }
  size_t operator()(const runtime::DataType& key) const {
    return std::hash<int32_t>()(static_cast<int32_t>(key.code()) |
                                (static_cast<int32_t>(key.bits()) << 8) |
                                (static_cast<int32_t>(key.lanes() << 16)));
  }
  template <typename ENum, typename = typename std::enable_if<std::is_enum<ENum>::value>::type>
  bool operator()(const ENum& key) const {
    return std::hash<size_t>()(static_cast<size_t>(key));
  }
};

/*!
 * \brief Content-aware structural hashing
 *
 * The structural hash value is recursively defined in the DAG of IRNodes.
 * There are two kinds of node:
 *
 * - Normal node: the hash value is defined by its content and type only.
 * - Graph node: each graph node will be assigned a unique index ordered by the
 *   first occurrence during the visit. The hash value of a graph node is
 *   combined from the hash values of its contents and the index.
 */
class StructuralHash : public BaseValueHash {
 public:
  using BaseValueHash::operator();

  CVM_DLL size_t operator()(const ObjectRef& key) const;
};

class SHashReducer {
 public:
  class Handler {
   public:

  };
};

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_STRUCTURAL_HASH_H_
