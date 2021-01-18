//
// Created by whale on 2021/1/12.
//

#include <gtest/gtest.h>
#include <cvt/runtime/container.h>

using namespace cvt;
using namespace cvt::runtime;

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
  ICHECK_EQ(s == m,  source == mismatch);
  ICHECK_EQ(s != m, source != mismatch);

  ICHECK_EQ( m < s, mismatch < source);
  ICHECK_EQ(m > s, mismatch > source);
  ICHECK_EQ(m <= s, mismatch <= source);
  ICHECK_EQ(m >= s, mismatch >= source);
  ICHECK_EQ(m == s, mismatch == source);
  ICHECK_EQ(m != s, mismatch != source);
}

TEST(String, Cast) {
  using namespace std;
  string source = "this is a string";
  String s{source};
  ObjectRef r = s;
  String s2 = Downcast<String>(r);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}