//
// Created by WangJingYu on 2021/7/6.
//

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/packed_func.h>
#include <cvm/runtime/registry.h>

namespace cvm {
namespace runtime {

std::string GetCustomTypeName(uint8_t type_code) { return ""; }

uint8_t GetCustomTypeCode(const std::string& type_name) { return 0; }

uint8_t ParseCustomDataType(const std::string& s, const char** scan) {
  ICHECK(s.substr(0, 6) == "custom") << "Not a valid custom custom datatype string";

  auto tmp = s.c_str();

  ICHECK(s.c_str() == tmp);
  *scan = s.c_str() + 6;
  ICHECK(s.c_str() == tmp);
  if (**scan != '[') LOG(FATAL) << "expected opening brace after 'custom' type in" << s;
  ICHECK(s.c_str() == tmp);
  *scan += 1;
  ICHECK(s.c_str() == tmp);
  size_t custom_name_len = 0;
  ICHECK(s.c_str() == tmp);
  while (*scan + custom_name_len <= s.c_str() + s.length() && *(*scan + custom_name_len) != ']')
    ++custom_name_len;
  ICHECK(s.c_str() == tmp);
  if (*(*scan + custom_name_len) != ']')
    LOG(FATAL) << "expected closing brace after 'custom' type in" << s;
  ICHECK(s.c_str() == tmp);
  *scan += custom_name_len + 1;
  ICHECK(s.c_str() == tmp);

  auto type_name = s.substr(7, custom_name_len);
  ICHECK(s.c_str() == tmp);
  return GetCustomTypeCode(type_name);
}

int CVMFuncCreateFromCFunc(CVMPackedCFunc func, void* resource_handle, CVMPackedFuncFinalizer fin,
                           CVMFunctionHandle* out) {}

}  // namespace runtime
}  // namespace cvm

const char* CVMGetLastError() { return "GetLastError"; }
