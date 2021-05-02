#include <cvm/node/container.h>

namespace cvm {
#if (USE_FALLBACK_STL_MAP == 0)
  constexpr uint64_t DenseMapNode::kNextProbeLocation[];
#endif
}  // namespace cvm