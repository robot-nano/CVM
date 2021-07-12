//
// Created by WangJingYu on 2021/7/5.
//

#ifndef CVM_SRC_RUNTIME_RUNTIME_BASE_H_
#define CVM_SRC_RUNTIME_RUNTIME_BASE_H_

#define API_BEGIN() try {
#define API_END()                                         \
  }                                                       \
  catch (::cvm::runtime::EnvErrorAlreadySet & _except_) { \
    return -2;                                            \
  }                                                       \
  catch (std::exception & _except_) {                     \
    return CVMAPIHandleException(_except_);               \
  }                                                       \
  return 0;

int CVMAPIHandleException(const std::exception& e);

#endif  // CVM_SRC_RUNTIME_RUNTIME_BASE_H_
