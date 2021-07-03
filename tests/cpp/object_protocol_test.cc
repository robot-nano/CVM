//
// Created by WangJingYu on 2021/7/2.
//

#include <gtest/gtest.h>
#include <cvm/runtime/object.h>
#include <cvm/runtime/memory.h>

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

class ObjectC : public Object {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const bool _type_child_slots_can_overflow = false;
  static constexpr const uint32_t _type_child_slots = 0;
  static constexpr const char* _type_key = "test.ObjC";
  CVM_DECLARE_BASE_OBJECT_INFO(ObjectC, Object);
};

class ObjectCC : public ObjectC {
 public:
  static constexpr const char* _type_key = "test.ObjCC";
  CVM_DECLARE_FINAL_OBJECT_INFO(ObjectCC, ObjectC);
};

CVM_REGISTER_OBJECT_TYPE(ObjectBase);
CVM_REGISTER_OBJECT_TYPE(ObjectA);
CVM_REGISTER_OBJECT_TYPE(ObjectB);
CVM_REGISTER_OBJECT_TYPE(ObjectAA);
CVM_REGISTER_OBJECT_TYPE(ObjectC);
CVM_REGISTER_OBJECT_TYPE(ObjectCC);

}  // namespace test
}  // namespace cvm

TEST(ObjectHierarchy, Basic) {
  using namespace cvm::runtime;
  using namespace cvm::test;

  ObjectRef refA(make_object<ObjectA>());
  ICHECK_EQ(refA->type_index(), ObjectA::RuntimeTypeIndex());
  ICHECK(refA.as<Object>() != nullptr);
  ICHECK(refA.as<ObjectA>() != nullptr);
  ICHECK(refA.as<ObjectBase>() != nullptr);
  ICHECK(refA.as<ObjectB>() == nullptr);
  ICHECK(refA.as<ObjectAA>() == nullptr);

  ObjectRef refAA(make_object<ObjectAA>());
  ICHECK_EQ(refAA->type_index(), ObjectAA::RuntimeTypeIndex());
  ICHECK(refAA.as<Object>() != nullptr);
  ICHECK(refAA.as<ObjectA>() != nullptr);
  ICHECK(refAA.as<ObjectBase>() != nullptr);
  ICHECK(refAA.as<ObjectAA>() != nullptr);
  ICHECK(refAA.as<ObjectB>() == nullptr);

  ObjectRef refB(make_object<ObjectB>());
  ICHECK_EQ(refB->type_index(), ObjectB::RuntimeTypeIndex());
  ICHECK(refB.as<Object>() != nullptr);
  ICHECK(refB.as<ObjectBase>() != nullptr);
  ICHECK(refB.as<ObjectB>() != nullptr);
  ICHECK(refB.as<ObjectA>() == nullptr);
  ICHECK(refB.as<ObjectAA>() == nullptr);

  ObjectRef refC(make_object<ObjectC>());
//  ObjectRef refCC(make_object<ObjectCC>());
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}