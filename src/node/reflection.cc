//
// Created by WangJingYu on 2021/6/30.
//

#include <cvm/ir/attrs.h>
#include <cvm/node/node.h>
#include <cvm/node/reflection.h>

#include <unordered_map>

namespace cvm {

class AttrGetter : public AttrVisitor {
 public:
  const String& skey;
  CVMRetValue* ret;

  AttrGetter(const String& skey, CVMRetValue* ret) : skey(skey), ret(ret) {}

  bool found_ref_object{false};

  void Visit(const char* key, double* value) final {
    if (skey == key) *ret = value[0];
  }
  void Visit(const char* key, int64_t* value) final {
    if (skey == key) *ret = value[0];
  }
  void Visit(const char* key, uint64_t* value) final {
    ICHECK_LE(value[0], static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
        << "cannot return too big constant";
    if (skey == key) *ret = static_cast<int64_t>(value[0]);
  }
  void Visit(const char* key, int* value) final {
    if (skey == key) *ret = static_cast<int64_t>(value[0]);
  }
  void Visit(const char* key, bool* value) final {
    if (skey == key) *ret = static_cast<int64_t>(value[0]);
  }
  void Visit(const char* key, void** value) final {
    if (skey == key) *ret = static_cast<void*>(value[0]);
  }
  void Visit(const char* key, DataType* value) final {
    if (skey == key) *ret = value[0];
  }
  void Visit(const char* key, std::string* value) final {
    if (skey == key) *ret = value[0];
  }
  void Visit(const char* key, runtime::NDArray* value) final {
    if (skey == key) {
      *ret = value[0];
      found_ref_object = true;
    }
  }
  void Visit(const char* key, runtime::ObjectRef* value) final {
    if (skey == key) {
      *ret = value[0];
      found_ref_object = true;
    }
  }
};

runtime::CVMRetValue ReflectionVTable::GetAttr(Object* self, const String& attr_name) const {
  runtime::CVMRetValue ret;
  AttrGetter getter(attr_name, &ret);

  bool success;
  if (getter.skey == "type_key") {
    ret = self->GetTypeKey();
    success = true;
  } else if (!self->IsInstance<DictAttrsNode>()) {
    VisitAttrs(self, &getter);
    success = getter.found_ref_object || ret.type_code() != kCVMNullptr;
  } else {
    // specially handle dict attr
    DictAttrsNode* dnode = static_cast<DictAttrsNode*>(self);
    auto it = dnode->dict.find(getter.skey);
    if (it != dnode->dict.end()) {
      success = true;
      ret = (*it).second;
    } else {
      success = false;
    }
  }
  if (!success) {
    LOG(FATAL) << "AttributeError: " << self->GetTypeKey() << " object has no attributed "
               << getter.skey;
  }
  return ret;
}

class AttrDir : public AttrVisitor {
 public:
  std::vector<std::string>* names;

  void Visit(const char* key, double* value) final { names->push_back(key); }
  void Visit(const char* key, int64_t* value) final { names->push_back(key); }
  void Visit(const char* key, uint64_t* value) final { names->push_back(key); }
  void Visit(const char* key, bool* value) final { names->push_back(key); }
  void Visit(const char* key, int* value) final { names->push_back(key); }
  void Visit(const char* key, void** value) final { names->push_back(key); }
  void Visit(const char* key, DataType* value) final { names->push_back(key); }
  void Visit(const char* key, std::string* value) final { names->push_back(key); }
  void Visit(const char* key, runtime::NDArray* value) final { names->push_back(key); }
  void Visit(const char* key, runtime::ObjectRef* value) final { names->push_back(key); }
};

std::vector<std::string> ReflectionVTable::ListAttrNames(Object* self) const {
  std::vector<std::string> names;
  AttrDir dir;
  dir.names = &names;

  if (!self->IsInstance<DictAttrsNode>()) {
    VisitAttrs(self, &dir);
  } else {
    DictAttrsNode* dnode = static_cast<DictAttrsNode*>(self);
    for (const auto& kv : dnode->dict) {
      names.push_back(kv.first);
    }
  }
  return names;
}

ReflectionVTable* ReflectionVTable::Global() {
  static ReflectionVTable inst;
  return &inst;
}

ObjectPtr<Object> ReflectionVTable::CreateInitObject(const std::string& type_key,
                                                     const std::string& repr_bytes) const {
  uint32_t tindex = Object::TypeKey2Index(type_key);
  if (tindex >= fcreate_.size() || fcreate_[tindex] == nullptr) {
    LOG(FATAL) << "TypeError: " << type_key << " is not registered via  CVM_REGISTER_NODE_TYPE";
  }
  return fcreate_[tindex](repr_bytes);
}

class NodeAttrSetter : public AttrVisitor {
 public:
  std::string type_key;
  std::unordered_map<std::string, runtime::CVMArgValue> attrs;

  void Visit(const char* key, double* value) final { *value = GetAttr(key).operator double(); }
  void Visit(const char* key, int64_t* value) final { *value = GetAttr(key).operator int64_t(); }
  void Visit(const char* key, uint64_t* value) final { *value = GetAttr(key).operator uint64_t(); }
  void Visit(const char* key, int* value) final { *value = GetAttr(key).operator int(); }
  void Visit(const char* key, bool* value) final { *value = GetAttr(key).operator bool(); }
  void Visit(const char* key, std::string* value) final {
    *value = GetAttr(key).operator std::string();
  }
  void Visit(const char* key, void** value) final { *value = GetAttr(key).operator void*(); }
  void Visit(const char* key, DataType* value) final { *value = GetAttr(key).operator DataType(); }
  void Visit(const char* key, runtime::NDArray* value) final {
    *value = GetAttr(key).operator runtime::NDArray();
  }
  void Visit(const char* key, ObjectRef* value) final {
    *value = GetAttr(key).operator ObjectRef();
  }

 private:
  runtime::CVMArgValue GetAttr(const char* key) {
    auto it = attrs.find(key);
    if (it == attrs.end()) {
      LOG(FATAL) << type_key << ": require field " << key;
    }
    runtime::CVMArgValue v = it->second;
    attrs.erase(it);
    return v;
  }
};

void InitNodeByPackedArgs(ReflectionVTable* reflection, Object* n, const CVMArgs& args) {
  NodeAttrSetter setter;
  setter.type_key = n->GetTypeKey();
  ICHECK_EQ(args.size() % 2, 0);
  for (int i = 0; i < args.size(); i += 2) {
    setter.attrs.emplace(args[i].operator std::string(), args[i + 1]);
  }
  reflection->VisitAttrs(n, &setter);

  if (setter.attrs.size() != 0) {
    std::ostringstream os;
    os << setter.type_key << " does not contain field ";
    for (const auto& kv : setter.attrs) {
      os << " " << kv.first;
    }
    LOG(FATAL) << os.str();
  }
}

ObjectRef ReflectionVTable::CreateObject(const std::string& type_key, const CVMArgs& kwargs) {
  ObjectPtr<Object> n = this->CreateInitObject(type_key);
  if (n->IsInstance<BaseAttrsNode>()) {
    static_cast<BaseAttrsNode*>(n.get())->InitByPackedArgs(kwargs);
  } else {
    InitNodeByPackedArgs(this, n.get(), kwargs);
  }
  return ObjectRef(n);
}

ObjectRef ReflectionVTable::CreateObject(const std::string& type_key,
                                         const Map<String, ObjectRef>& kwargs) {
  std::vector<CVMValue> values(kwargs.size() * 2);
  std::vector<int32_t> tcodes(kwargs.size() * 2);
  runtime::CVMArgsSetter setter(values.data(), tcodes.data());
  int index = 0;

  for (const auto& kv : *static_cast<const MapNode*>(kwargs.get())) {
    setter(index, Downcast<String>(kv.first).c_str());
    setter(index + 1, kv.second);
    index += 2;
  }

  return CreateObject(type_key, runtime::CVMArgs(values.data(), tcodes.data(), kwargs.size() * 2));
}

}  // namespace cvm