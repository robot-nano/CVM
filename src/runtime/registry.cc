//
// Created by WangJingYu on 2021/1/19.
//

#include <cvt/runtime/registry.h>

#include <mutex>
#include <unordered_map>

namespace cvt {
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

}  // namespace runtime
}  // namespace cvt