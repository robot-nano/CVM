//
// Created by WangJingYu on 2021/1/12.
//

#ifndef CVT_CONTAINER_H
#define CVT_CONTAINER_H

#include <cvt/runtime/object.h>

namespace cvt {
namespace runtime {

class StringObj : public Object {
 public:
  const char* data_;

  uint64_t size;

  static constexpr const uint32_t _type_index = TypeIndex::kRuntimeString;
  static constexpr const char* _type_key = "runtime.String";

};

}  // namespace runtime
}  // namespace cvt

#endif  // CVT_CONTAINER_H
