#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/registry.h>

#include <mutex>
#include <unordered_map>

#include "runtime_base.h"

namespace cvm {
namespace runtime {

struct Registry::Manager {
  std::unordered_map<std::string, Registry*> fmap;

  std::mutex mutex;

  Manager() {}

  static Manager* Global() {
    static Manager* inst = new Manager();
    return inst;
  }
};

Registry& Registry::set_body(PackedFunc f) {  // NOLINT(*)
  func_ = f;
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

const PackedFunc* Registry::Get(const std::string& name) {
  Manager* m = Manager::Global();
  std::lock_guard<std::mutex> lock(m->mutex);
  auto it = m->fmap.find(name);
  if (it == m->fmap.end()) return nullptr;
  return &(it->second->func_);
}

}  // namespace runtime
}  // namespace cvm

int CVMFuncRegisterGlobal(const char* name, CVMFunctionHandle f, int override) {
  API_BEGIN();
  cvm::runtime::Registry::Register(name, override != 0)
      .set_body(*static_cast<cvm::runtime::PackedFunc*>(f));
  API_END();
}

int CVMFuncGetGlobal(const char* name, CVMFunctionHandle* out) {
  API_BEGIN();
  const cvm::runtime::PackedFunc* fp = cvm::runtime::Registry::Get(name);
  if (fp != nullptr) {
    *out = new cvm::runtime::PackedFunc(*fp);
  } else {
    *out = nullptr;
  }
  API_END();
}
