//
// Created by WJY on 2021/7/6.
//

#include <cvm/runtime/data_type.h>
#include <gtest/gtest.h>

#include <iostream>

using namespace cvm::runtime;

TEST(DLDataType, ostream) {
  DLDataType dl_type;
  dl_type.bits = 1;
  dl_type.code = kDLInt;
  dl_type.lanes = 1;

  std::cout << dl_type << std::endl;

  dl_type = String2DLDataType("int32");
  ICHECK(dl_type.code == kDLInt);
  ICHECK(dl_type.bits == 32);
  ICHECK(dl_type.lanes == 1);

//  data_type = String2DLDataType("custom[]");

  DataType data_type(kDLInt, 32, 1);
  std::ostringstream os;
//  os << data_type;
  std::cout << data_type << std::endl;
  ICHECK(os.str() == "int32");

//  dl_type.code = 126;
//  auto type_str = DLDataType2String(data_type);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::FLAGS_gtest_death_test_style = "threadsafe";
  return RUN_ALL_TESTS();
}