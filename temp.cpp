//
// Created by WangJingYu on 2021/1/21.
//

#include <cvt/runtime/container.h>
#include <cvt/runtime/object.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
using namespace cvt::runtime;

class TestErrorSwitch {
 public:
  TestErrorSwitch(const TestErrorSwitch& other) : should_fail(other.should_fail) {
    const_cast<TestErrorSwitch&>(other).should_fail = false;
  }

  TestErrorSwitch(bool fail_flag) : should_fail{fail_flag} {}
  bool should_fail{false};

  ~TestErrorSwitch() {
    if (should_fail) {
      exit(1);
    }
  }
};

class TestArrayObj : public Object, public InplaceArrayBase<TestArrayObj, TestErrorSwitch> {
 public:
  static constexpr const uint32_t _type_index = TypeIndex::kDynamic;
  static constexpr const char* _type_key = "test.TestArrayObj";
  CVT_DECLARE_FINAL_OBJECT_INFO(TestArrayObj, Object);
  uint32_t size;

  size_t GetSize() const { return size; }

  template <typename Iterator>
  void Init(Iterator begin, Iterator end) {
    size_t num_elems = std::distance(begin, end);
    this->size = 0;
    auto it = begin;
    for (size_t i = 0; i < num_elems; ++i) {
      InplaceArrayBase::EmplaceInit(i, *it++);
      if (i == 1) {
        throw std::bad_alloc();
      }
      // Only increment size after the initialization succeeds
      this->size++;
    }
  }

  template <typename Iterator>
  void WrongInit(Iterator begin, Iterator end) {
    size_t num_elems = std::distance(begin, end);
    this->size = num_elems;
    auto it = begin;
    for (size_t i = 0; i < num_elems; ++i) {
      InplaceArrayBase::EmplaceInit(i, *it++);
      if (i == 1) {
        throw std::bad_alloc();
      }
    }
  }

  friend class InplaceArrayBase;
};

void wrong_init() {
  TestErrorSwitch f1{false};
  TestErrorSwitch f2{true};
  TestErrorSwitch f3{false};
  std::vector<TestErrorSwitch> fields{f1, f2, f3};
  auto ptr = make_inplace_array_object<TestArrayObj, TestErrorSwitch>(fields.size());
  try {
    ptr->WrongInit(fields.begin(), fields.end());
  } catch (...) {
  }
  ptr.reset();
  exit(0);
}

int main() { wrong_init(); }