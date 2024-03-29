//
// Created by WangJingYu on 2021/7/2.
//

#include <cvm/runtime/object.h>

#include <mutex>
#include <unordered_map>
#include <vector>

#include "runtime_base.h"

namespace cvm {
namespace runtime {

struct TypeInfo {
  uint32_t index{0};
  uint32_t parent_index{0};
  uint32_t num_slots{0};
  uint32_t allocated_slots{0};
  bool child_slots_can_overflow{true};
  std::string name;
  size_t name_hash{0};
};

class TypeContext {
 public:
  bool DerivedFrom(uint32_t child_tindex, uint32_t parent_tindex) {
    if (child_tindex < parent_tindex) return false;
    if (child_tindex == parent_tindex) return true;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      ICHECK_LT(child_tindex, type_table_.size());
      while (child_tindex > parent_tindex) {
        child_tindex = type_table_[child_tindex].parent_index;
      }
    }
    return child_tindex == parent_tindex;
  }

  uint32_t GetOrAllocRuntimeTypeIndex(const std::string& skey, uint32_t static_tindex,
                                      uint32_t parent_tindex, uint32_t num_child_slots,
                                      bool child_slots_can_overflow) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = type_key2index_.find(skey);
    if (it != type_key2index_.end()) {
      return it->second;
    }

    ICHECK_LT(parent_tindex, type_table_.size())
        << " skey=" << skey << ", static_index=" << static_tindex;
    TypeInfo& pinfo = type_table_[parent_tindex];
    ICHECK_EQ(pinfo.index, parent_tindex);

    // if parent cannot overflow, then this class cannot.
    if (!pinfo.child_slots_can_overflow) {
      child_slots_can_overflow = false;
    }

    // total number of slots include the type itself.
    uint32_t num_slots = num_child_slots + 1;
    uint32_t allocated_tindex;

    if (static_tindex != TypeIndex::kDynamic) {
      allocated_tindex = static_tindex;
      ICHECK_LT(static_tindex, type_table_.size());
      ICHECK_EQ(type_table_[allocated_tindex].allocated_slots, 0U)
          << "Conflicting static index " << static_tindex << " between "
          << type_table_[allocated_tindex].name << " and " << skey;
    } else if (pinfo.allocated_slots + num_slots <= pinfo.num_slots) {
      // allocate the slot from parent's reserved pool
      allocated_tindex = parent_tindex + pinfo.allocated_slots;
      // update parent's state
      pinfo.allocated_slots += num_slots;
    } else {
      ICHECK(pinfo.child_slots_can_overflow)
          << "Reach maximum number of sub-class for " << pinfo.name;
      // allocate new entries.
      allocated_tindex = type_counter_;
      type_counter_ += num_slots;
      ICHECK_LE(type_table_.size(), type_counter_);
      type_table_.resize(type_counter_, TypeInfo());
    }
    ICHECK_GT(allocated_tindex, parent_tindex);
    // initialize the slot.
    type_table_[allocated_tindex].index = allocated_tindex;
    type_table_[allocated_tindex].parent_index = parent_tindex;
    type_table_[allocated_tindex].num_slots = num_slots;
    type_table_[allocated_tindex].allocated_slots = 1;
    type_table_[allocated_tindex].child_slots_can_overflow = child_slots_can_overflow;
    type_table_[allocated_tindex].name = skey;
    type_table_[allocated_tindex].name_hash = std::hash<std::string>()(skey);
    // update the key2index mapping.
    type_key2index_[skey] = allocated_tindex;
    return allocated_tindex;
  }

  std::string TypeIndex2Key(uint32_t tindex) {
    std::lock_guard<std::mutex> lock(mutex_);
    ICHECK(tindex < type_table_.size() && type_table_[tindex].allocated_slots != 0)
        << "Unknown type index " << tindex;
    return type_table_[tindex].name;
  }

  size_t TypeIndex2KeyHash(uint32_t tindex) {
    std::lock_guard<std::mutex> lock(mutex_);
    ICHECK(tindex < type_table_.size() && type_table_[tindex].allocated_slots != 0)
        << "Unknown type index " << tindex;
    return type_table_[tindex].name_hash;
  }

  uint32_t TypeKey2Index(const std::string& skey) {
    auto it = type_key2index_.find(skey);
    ICHECK(it != type_key2index_.end())
        << "Cannot find type " << skey
        << ". Did you forget to register the node by CVM_REGISTER_NODE_TYPE ?";
    return it->second;
  }

  void Dump(int min_children_count) {
    std::vector<int> num_children(type_table_.size(), 0);
    // reverse accumulation so we can get total counts in a bottom-up manner.
    for (auto it = type_table_.rbegin(); it != type_table_.rend(); ++it) {
      if (it->index != 0) {
        num_children[it->parent_index] += num_children[it->index] + 1;
      }
    }

    for (const auto& info : type_table_) {
      if (info.index != 0 && num_children[info.index] >= min_children_count) {
        std::cerr << "[" << info.index << "]" << info.name
                  << "\tparent=" << type_table_[info.parent_index].name
                  << "\tnum_child_slots=" << info.num_slots + 1
                  << "\tnum_children=" << num_children[info.index] << std::endl;
      }
    }
  }

  static TypeContext* Global() {
    static TypeContext inst;
    return &inst;
  }

 private:
  TypeContext() {
    type_table_.resize(TypeIndex::kStaticIndexEnd, TypeInfo());
    type_table_[0].name = "runtime.Object";
  }
  // mutex to avoid registration from multiple threads.
  std::mutex mutex_;
  std::atomic<uint32_t> type_counter_{TypeIndex::kStaticIndexEnd};
  std::vector<TypeInfo> type_table_;
  std::unordered_map<std::string, uint32_t> type_key2index_;
};

uint32_t Object::GetOrAllocRuntimeTypeIndex(const std::string& skey, uint32_t static_tindex,
                                            uint32_t parent_tindex, uint32_t num_child_slots,
                                            bool child_slots_can_overflow) {
  return TypeContext::Global()->GetOrAllocRuntimeTypeIndex(
      skey, static_tindex, parent_tindex, num_child_slots, child_slots_can_overflow);
}

bool Object::DerivedFrom(uint32_t parent_tindex) const {
  return TypeContext::Global()->DerivedFrom(this->type_index_, parent_tindex);
}

std::string Object::TypeIndex2Key(uint32_t tindex) {
  return TypeContext::Global()->TypeIndex2Key(tindex);
}

size_t Object::TypeIndex2KeyHash(uint32_t tindex) {
  return TypeContext::Global()->TypeIndex2KeyHash(tindex);
}

uint32_t Object::TypeKey2Index(const std::string& key) {
  return TypeContext::Global()->TypeKey2Index(key);
}

}  // namespace runtime
}  // namespace cvm

int CVMObjectGetTypeIndex(CVMObjectHandle obj, unsigned* out_tindex) {
  API_BEGIN();
  ICHECK(obj != nullptr);
  out_tindex[0] = static_cast<cvm::runtime::Object*>(obj)->type_index();
  API_END();
}

int CVMObjectTypeKey2Index(const char* type_key, unsigned* out_tindex) {
  API_BEGIN();
  API_END();
}