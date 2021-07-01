//
// Created by WangJingYu on 2021/7/1.
//

#include <cvm/node/reflection.h>

namespace cvm {

void ReflectionVTable::SHashReduce(const Object* self, SHashReducer hash_reduce) const {
  uint32_t tindex = self->type_index();
  if (tindex >= fshash_reduce_.size() || fshash_reduce_[tindex] == nullptr) {
    LOG(FATAL) << "TypeError: SHashReduce of " << self->GetTypeKey()
               << " is not registered via CVM_REGISTER_NODE_TYPE";
  }
  fshash_reduce_[tindex](self, hash_reduce);
}

}  // namespace cvm