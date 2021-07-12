#ifndef CVM_INCLUDE_SUPPORT_LOGGING_H_
#define CVM_INCLUDE_SUPPORT_LOGGING_H_

#include <cvm/runtime/c_runtime_api.h>

#include <iostream>
#include <sstream>

#if defined(_MSC_VER)
#define CVM_NO_INLINE __declspec(noinline)
#else
#define CVM_NO_INLINE __attribute__((noinline))
#endif

#ifdef _MSC_VER
#define CVM_ALWAYS_INLINE __forceinline
#else
#define CVM_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#define LOG(level) LOG_##level
#define LOG_FATAL std::cerr << __FILE__ << " " << __LINE__ << " "

#define ICHECK_BINARY_OP(name, op, x, y) \
  if (!((x)op(y)))                       \
  std::cerr << __FILE__ << " " << __LINE__ << "Check failed: " << #x " " #op " " #y << " "

#define ICHECK(x) \
  if (!(x)) std::cerr << __FILE__ << " " << __LINE__ << "Check failed: " #x << " == false: "

#define ICHECK_LT(x, y) ICHECK_BINARY_OP(_LT, <, x, y)
#define ICHECK_GT(x, y) ICHECK_BINARY_OP(_GT, >, x, y)
#define ICHECK_LE(x, y) ICHECK_BINARY_OP(_LE, <=, x, y)
#define ICHECK_GE(x, y) ICHECK_BINARY_OP(_GE, >=, x, y)
#define ICHECK_EQ(x, y) ICHECK_BINARY_OP(_EQ, ==, x, y)
#define ICHECK_NE(x, y) ICHECK_BINARY_OP(_NE, !=, x, y)

namespace cvm {
namespace runtime {

CVM_DLL std::string Backtrace();

class Error : public std::runtime_error {
 public:
  explicit Error(const std::string& s) : std::runtime_error(s) {}
};

class EnvErrorAlreadySet : public std::runtime_error {
 public:
  explicit EnvErrorAlreadySet(const std::string& s) : std::runtime_error(s) {}
};

class InternalError : public Error {

};

}  // namespace runtime
using runtime::Error;
}  // namespace cvm

#endif  // CVM_INCLUDE_SUPPORT_LOGGING_H_
