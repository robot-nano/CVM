//
// Created by WangJingYu on 2021/1/23.
//

#include <cvt/node/container.h>

namespace cvt {
#if (USE_FALLBACK_STL_MAP == 0)
  constexpr uint64_t DenseMapNode::kNextProbeLocation[];
#endif
}  // namespace cvt