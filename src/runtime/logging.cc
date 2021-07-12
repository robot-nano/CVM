//
// Created by WJY on 2021/7/9.
//

#include <cvm/runtime/logging.h>

/**/
#include <backtrace.h>
#include <cxxabi.h>

#include <iomanip>
#include <memory>
#include <mutex>
#include <vector>

namespace cvm {
namespace runtime {

class BacktraceInfo {
 public:
  std::vector<std::string> lines;
  size_t max_size;
  std::string error_message;
};

void BacktraceCreateErrorCallback(void* data, const char* msg, int err_num) {
  std::cerr << "Could not initialize backtrace state: " << msg << std::endl;
}

backtrace_state* BacktraceCreate() {
  return backtrace_create_state(nullptr, 1, BacktraceCreateErrorCallback, nullptr);
}

static backtrace_state* _bt_state = BacktraceCreate();

std::string DemangleName(std::string name) {
  int status = 0;
  size_t length = name.size();
  std::unique_ptr<char, void (*)(void* _ptr)> demangled_name = {
      abi::__cxa_demangle(name.c_str(), nullptr, &length, &status), &std::free};
  if (demangled_name && status == 0 && length > 0) {
    return demangled_name.get();
  } else {
    return name;
  }
}

void BacktraceErrorCallback(void* data, const char* msg, int err_num) {}

void BacktraceSyminfoCallback(void* data, uintptr_t pc, const char* symname, uintptr_t symval,
                              uintptr_t symsize) {
  auto str = reinterpret_cast<std::string*>(data);

  if (symname != nullptr) {
    std::string tmp(symname, symsize);
    *str = DemangleName(tmp.c_str());
  } else {
    std::ostringstream s;
    s << "0x" << std::setfill('0') << std::setw(sizeof(uintptr_t) * 2) << std::hex << pc;
    *str = s.str();
  }
}

int BacktraceFullCallback(void* data, uintptr_t pc, const char* filename, int lineno,
                          const char* symbol) {
  auto stack_trace = reinterpret_cast<BacktraceInfo*>(data);
  std::stringstream s;

  std::unique_ptr<std::string> symbol_str = std::make_unique<std::string>("<unknown>");
  if (symbol != nullptr) {
    *symbol_str = DemangleName(symbol);
  } else {
    // see if syminfo gives anything
    backtrace_syminfo(_bt_state, pc, BacktraceSyminfoCallback, BacktraceErrorCallback,
                      symbol_str.get());
  }
  s << *symbol_str;

  if (filename != nullptr) {
    s << std::endl << "        at " << filename;
    if (lineno != 0) {
      s << ":" << lineno;
    }
  }
  if (!(stack_trace->lines.size() == 0 &&
        (symbol_str->find("cvm::runtime::Backtrace", 0) == 0 ||
         symbol_str->find("cvm::runtime::detail::LogFatal", 0) == 0))) {
    stack_trace->lines.push_back(s.str());
  }
  if (*symbol_str == "CVMFuncCall" || stack_trace->lines.size() >= stack_trace->max_size) {
    return 1;
  }
  return 0;
}

std::string Backtrace() {
  BacktraceInfo bt;
  bt.max_size = 100;
  if (_bt_state == nullptr) {
    return "";
  }

  static std::mutex m;
  std::lock_guard<std::mutex> lock(m);
  backtrace_full(_bt_state, 0, BacktraceFullCallback, BacktraceErrorCallback, &bt);

  std::ostringstream s;
  s << "Stack trace:\n";
  for (size_t i = 0; i < bt.lines.size(); ++i) {
    s << "  " << i << ": " << bt.lines[i] << "\n";
  }

  return s.str();
}

}  // namespace runtime
}  // namespace cvm