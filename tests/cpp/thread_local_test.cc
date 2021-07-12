//
// Created by WJY on 2021/7/9.
//

#include <cvm/runtime/logging.h>
#include <cvm/runtime/thread_local.h>
#include <gtest/gtest.h>

#include <thread>

using namespace cvm::runtime;

class Entry {
 public:
  std::string info1;
  std::string info2;
  int index;
};

template <typename T>
void TestFunc() {
  ICHECK(T::Get()->info1.empty());
}

TEST(ThreadLocalStore, single) {
  typedef ThreadLocalStore<Entry> RuntimeStore;
  ICHECK(RuntimeStore::Get()->info1.empty());
  RuntimeStore::Get()->info1 = "test_info1";
  ICHECK(RuntimeStore::Get()->info1 == "test_info1");
}

TEST(ThreadLocalStore, multi) {
  typedef ThreadLocalStore<Entry> RuntimeStore;
  RuntimeStore::Get()->info1 = "main";

  std::thread t1(TestFunc<RuntimeStore>);
  TestFunc<RuntimeStore>();
  t1.join();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}