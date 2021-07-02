//
// Created by WangJingYu on 2021/7/2.
//

#include <gtest/gtest.h>
#include <cvm/runtime/object.h>

namespace cvm {
namespace test {

using namespace cvm::runtime;

class ObjectBase : public Object {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const uint32_t _type_child_slots = 1;
  static constexpr const char* _type_key = "test.ObjectBase";
  CVM_DECLARE_BASE_OBJECT_INFO(ObjectBase, Object);
};

class ObjectA : public ObjectBase {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const uint32_t _type_child_slots = 0;
  static constexpr const char* _type_key = "test.ObjA";
  CVM_DECLARE_BASE_OBJECT_INFO(ObjectA, ObjectBase);
};

class ObjectB : public ObjectBase {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const uint32_t _type_child_slots = 0;
  static constexpr const char* _type_key = "test.ObjB";
  CVM_DECLARE_BASE_OBJECT_INFO(ObjectB, ObjectBase);
};

class ObjectAA : public ObjectA {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "test.ObjAA";
  CVM_DECLARE_FINAL_OBJECT_INFO(ObjectAA, ObjectA);
};

CVM_REGISTER_OBJECT_TYPE(ObjectBase);
CVM_REGISTER_OBJECT_TYPE(ObjectA);
CVM_REGISTER_OBJECT_TYPE(ObjectB);
CVM_REGISTER_OBJECT_TYPE(ObjectAA);

}  // namespace test
}  // namespace cvm

TEST(ObjectHierarchy, Basic) {
  using namespace cvm::runtime;
  using namespace cvm::test;
//
//  ObjectRef refA(make_object<ObjectA>());
//  ICHECK_EQ(refA->type_index(), ObjectA::RuntimeTypeIndex());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}