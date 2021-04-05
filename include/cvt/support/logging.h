#ifndef CVT_INCLUDE_SUPPORT_LOGGING_H_
#define CVT_INCLUDE_SUPPORT_LOGGING_H_

#include <iostream>

#define ICHECK_BINARY_OP(name, op, x, y) \
  if (!((x) op (y)))                         \
  std::cerr << __FILE__ << " " << __LINE__ << "Check failed: " << #x " " #op " " #y << " "

#define ICHECK(x) \
  if (!(x)) std::cerr << __FILE__ << " " << __LINE__ << "Check failed: " #x << " == false: "

#define ICHECK_LT(x, y) ICHECK_BINARY_OP(_LT, <, x, y)
#define ICHECK_GT(x, y) ICHECK_BINARY_OP(_GT, >, x, y)
#define ICHECK_LE(x, y) ICHECK_BINARY_OP(_LE, <=, x, y)
#define ICHECK_GE(x, y) ICHECK_BINARY_OP(_GE, >=, x, y)
#define ICHECK_EQ(x, y) ICHECK_BINARY_OP(_EQ, ==, x, y)
#define ICHECK_NE(x, y) ICHECK_BINARY_OP(_NE, !=, x, y)

#endif //CVT_INCLUDE_SUPPORT_LOGGING_H_
