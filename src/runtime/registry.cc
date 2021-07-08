//
// Created by WangJingYu on 2021/7/5.
//

#include <cvm/runtime/registry.h>

#include <mutex>
#include <unordered_map>

#include "runtime_base.h"

namespace cvm {
namespace runtime {

class Registry::Manager {
 public:
  std::unordered_map<std::string, Registry*> fmap;
  std::mutex mutex;

  static Manager* Global() {
    static Manager* inst = new Manager();
    return inst;
  }
};

Registry& Registry::set_body(PackedFunc f) {
  func_ = std::move(f);
  return *this;
}

Registry& Registry::Register(const std::string& name, bool can_override) {
  Manager* m = Manager::Global();
  std::lock_guard<std::mutex> lock(m->mutex);
  if (m->fmap.count(name)) {
    ICHECK(can_override) << "Global PackedFunc " << name << " is already registered";
  }

  Registry* r = new Registry();
  r->name_ = name;
  m->fmap[name] = r;
  return *r;
}

bool Registry::Remove(const std::string& name) {
  Manager* m = Manager::Global();
  std::lock_guard<std::mutex> lock(m->mutex);
  auto it = m->fmap.find(name);
  if (it == m->fmap.end()) return false;
  m->fmap.erase(it);
  return true;
}

const PackedFunc* Registry::Get(const std::string& name) {
  Manager* m = Manager::Global();
  std::lock_guard<std::mutex> lock(m->mutex);
  auto it = m->fmap.find(name);
  if (it == m->fmap.end()) return nullptr;
  return &(it->second->func_);
}

std::vector<std::string> Registry::ListNames() {
  Manager* m = Manager::Global();
  std::lock_guard<std::mutex> lock(m->mutex);
  std::vector<std::string> keys;
  for (const auto& kv : m->fmap) {
    keys.emplace_back(kv.first);
  }
  return keys;
}

#define CVM_FUNC_REG_VAR_DEF static CVM_ATTRIBUTE_UNUSED ::cvm::runtime::Registry& __mk_##CVM

#define CVM_REGISTER_GLOBAL(OpName) \
  CVM_STR_CONCAT(CVM_FUNC_REG_VAR_DEF, __COUNTER__) = ::cvm::runtime::Registry::Register(OpName)

}  // namespace runtime
}  // namespace cvm

int CVMFuncListGlobalNames(int* out_size, const char*** out_array) {
  auto ret_vec = cvm::runtime::Registry::ListNames();
  std::vector<const char*> ret_vec_charp;
  ret_vec_charp.reserve(ret_vec.size());
  for (auto& i : ret_vec) {
    ret_vec_charp.push_back(i.c_str());
  }
  *out_array = &ret_vec_charp[0];
  *out_size = static_cast<int>(ret_vec.size());
  return 0;
}

int CVMFuncGetGlobal(const char* name, CVMFunctionHandle* out) {
  const cvm::runtime::PackedFunc* fp = cvm::runtime::Registry::Get(name);
  if (fp != nullptr) {
    *out = new cvm::runtime::PackedFunc(*fp);
  } else {
    *out = nullptr;
  }
  return 0;
}