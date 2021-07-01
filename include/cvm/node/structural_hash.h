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
    virtual void SHashReduceHashedValue(size_t hashed_value) = 0;

    virtual void SHashReduce(const ObjectRef& key, bool map_free_vars) = 0;

    virtual void SHashReduceFreeVar(const runtime::Object* var, bool map_free_vars) = 0;

    virtual void LookupHashedValue(const ObjectRef& key, size_t* hashed_value) = 0;

    virtual void MarkGraphNode() = 0;
  };

  SHashReducer() = default;

  explicit SHashReducer(Handler* handler, bool map_free_vars)
      : handler_(handler), map_free_vars_(map_free_vars) {}

  template <typename T,
            typename = typename std::enable_if<!std::is_base_of<ObjectRef, T>::value>::type>
  void operator()(const T& key) const {
    // handle normal values.
    handler_->SHashReduceHashedValue(BaseValueHash()(key));
  }

  void operator()(const ObjectRef& key) const { return handler_->SHashReduce(key, map_free_vars_); }

  void DefHash(const ObjectRef& key) const { return handler_->SHashReduce(key, true); }

  void FreeVarHashImpl(const runtime::Object* var) const {
    handler_->SHashReduceFreeVar(var, map_free_vars_);
  }

  Handler* operator->() const { return handler_; }

 private:
  Handler* handler_;
  bool map_free_vars_;
};

}  // namespace cvm

#endif  // CVM_INCLUDE_CVM_NODE_STRUCTURAL_HASH_H_
