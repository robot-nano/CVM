//
// Created by whale on 2021/1/12.
//

#include <cvt/runtime/container.h>
#include <gtest/gtest.h>

#include <unordered_map>

using namespace cvt;
using namespace cvt::runtime;

class TestErrorSwitch {
 public:
  // Need this so that destructor of temporary objects don't interrupt our
  // testing.
  TestErrorSwitch(const TestErrorSwitch& other) : should_fail(other.should_fail) {
    const_cast<TestErrorSwitch&>(other).should_fail = false;
  }

  TestErrorSwitch(bool fail_flag) : should_fail{fail_flag} {}
  bool should_fail{false};

  ~TestErrorSwitch() {
    if (should_fail) {
      std::cerr << "~" << should_fail << std::endl;
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

TEST(InplaceArrayBase, BadExceptionSafety) {
  auto wrong_init = []() {
    TestErrorSwitch f1{false};
    // WrongInit will set size to 3 so it will call destructor at index 1, which
    // will exit with error status.
    TestErrorSwitch f2{true};
    TestErrorSwitch f3{false};
    std::vector<TestErrorSwitch> fields{f1, f2, f3};
    auto ptr = make_inplace_array_object<TestArrayObj, TestErrorSwitch>(fields.size());
    try {
      ptr->WrongInit(fields.begin(), fields.end());
    } catch (...) {
    }
    // Call ~InplaceArrayBase
    ptr.reset();
    // never reaches here.
    exit(0);
  };
  ASSERT_EXIT(wrong_init(), ::testing::ExitedWithCode(1), "");
}

TEST(InplaceArrayBase, ExceptionSafety) {
  auto correct_init = []() {
    TestErrorSwitch f1{false};
    TestErrorSwitch f2{true};
    std::vector<TestErrorSwitch> fields{f1, f2};
    auto ptr = make_inplace_array_object<TestArrayObj, TestErrorSwitch>(fields.size());
    try {
      ptr->Init(fields.begin(), fields.end());
    } catch (...) {
    }

    ptr.reset();
    exit(0);
  };
  ASSERT_EXIT(correct_init(), ::testing::ExitedWithCode(0), "");
}

TEST(String, MoveFromStd) {
  using namespace std;
  string source = "this is a string";
  string expect = source;
  String s(std::move(source));
  string copy = (string)s;
  ICHECK_EQ(copy, expect);
  ICHECK_EQ(source.size(), 0);
}

TEST(String, CopyFromStd) {
  using namespace std;
  string source = "this is a string";
  string expect = source;
  String s(source);
  string copy = (string)s;
  ICHECK_EQ(copy, expect);
  ICHECK_EQ(source.size(), expect.size());
}

TEST(String, Assignment) {
  using namespace std;
  String s{string{"hello"}};
  s = string{"world"};
  ICHECK_EQ(s == "world", true);
  string s2{"world2"};
  s = std::move(s2);
  ICHECK_EQ(s == "world2", true);
}

TEST(String, empty) {
  using namespace std;
  String s{"hello"};
  ICHECK_EQ(s.empty(), false);
  s = std::string("");
  ICHECK_EQ(s.empty(), true);
}

TEST(String, Compareisons) {
  using namespace std;
  string source = "a string";
  string mismatch = "a string but longer";
  String s{source};
  String m{mismatch};

  ICHECK_EQ(s == source, true);
  ICHECK_EQ(s == mismatch, false);
  ICHECK_EQ(s == source.data(), true);
  ICHECK_EQ(s == mismatch.data(), false);

  ICHECK_EQ(s < m, source < mismatch);
  ICHECK_EQ(s > m, source > mismatch);
  ICHECK_EQ(s <= m, source <= mismatch);
  ICHECK_EQ(s >= m, source >= mismatch);
  ICHECK_EQ(s == m, source == mismatch);
  ICHECK_EQ(s != m, source != mismatch);

  ICHECK_EQ(m < s, mismatch < source);
  ICHECK_EQ(m > s, mismatch > source);
  ICHECK_EQ(m <= s, mismatch <= source);
  ICHECK_EQ(m >= s, mismatch >= source);
  ICHECK_EQ(m == s, mismatch == source);
  ICHECK_EQ(m != s, mismatch != source);
}

// check '\0' handling
TEST(String, null_byte_handling) {
  using namespace std;
  string v1 = "hello world";
  size_t v1_size = v1.size();
  v1[5] = '\0';
  ICHECK_EQ(v1[5], '\0');
  ICHECK_EQ(v1.size(), v1_size);
  String str_v1{v1};
  ICHECK_EQ(str_v1.compare(v1), 0);
  ICHECK_EQ(str_v1.size(), v1_size);

  // Ensure bytes after '\0' are taken into account for mismatches.
  string v2 = "aaa one";
  string v3 = "aaa two";
  v2[3] = '\0';
  v3[3] = '\0';
  String str_v2{v2};
  String str_v3{v3};
  ICHECK_EQ(str_v2.compare(str_v3), -1);
  ICHECK_EQ(str_v2.size(), 7);
  // strcmp won't be able to detect the mismatch
  ICHECK_EQ(strcmp(v2.data(), v3.data()), 0);
  // string::compare can handle \0 since it knows size
  ICHECK_LT(v2.compare(v3), 0);

  // If there is mismatch before '\0', should still handle it
  string v4 = "acc one";
  string v5 = "abb two";
  v4[3] = '\0';
  v5[3] = '\0';
  String str_v4{v4};
  String str_v5{v5};
  ICHECK_GT(str_v4.compare(str_v5), 0);
  ICHECK_EQ(str_v4.size(), 7);
  // strcmp is able to detect the mismatch
  ICHECK_GT(strcmp(v4.data(), v5.data()), 0);
  // string::compare can handle \0 since it knows size
  ICHECK_GT(v4.compare(v5), 0);
}

TEST(String, Cast) {
  using namespace std;
  string source = "this is a string";
  String s{source};
  ObjectRef r = s;
  String s2 = Downcast<String>(r);
}

TEST(String, hash) {
  using namespace std;
  string source = "this is a string";
  String s{source};
  std::hash<String>()(s);

  std::unordered_map<String, std::string> map;
  String k1{string{"k1"}};
  String v1{"v1"};
  String k2{string{"k2"}};
  String v2{"v2"};
  map[k1] = v1;
  map[k2] = v2;

  ICHECK_EQ(map[k1], v1);
  ICHECK_EQ(map[k2], v2);
}

TEST(String, Concat) {
  String s1("hello");
  String s2("world");
  std::string s3("world");
  String res1 = s1 + s2;
  String res2 = s1 + s3;
  String res3 = s3 + s1;
  String res4 = s1 + "world";
  String res5 = "world" + s1;

  ICHECK_EQ(res1.compare("helloworld"), 0);
  ICHECK_EQ(res2.compare("helloworld"), 0);
  ICHECK_EQ(res3.compare("worldhello"), 0);
  ICHECK_EQ(res4.compare("helloworld"), 0);
  ICHECK_EQ(res5.compare("worldhello"), 0);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}