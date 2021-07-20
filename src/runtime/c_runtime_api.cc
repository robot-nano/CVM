//
// Created by WangJingYu on 2021/7/6.
//

#include <cvm/runtime/c_runtime_api.h>
#include <cvm/runtime/packed_func.h>
#include <cvm/runtime/registry.h>
#include <cvm/runtime/thread_local.h>

#include <memory>

#include "runtime_base.h"

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

std::string NormalizeError(std::string err_msg) {
  int line_number = 0;
  std::istringstream is(err_msg);
  std::string line, file_name, error_type, check_msg;

  auto parse_log_header = [&]() {
    if (is.peek() != '[') {
      getline(is, line);
      return true;
    }
    if (!(is >> line)) return false;
    // get filename
    while (is.peek() == ' ') is.get();
#ifdef _MSC_VER  // handle volume separator ":" in Windows path
    std::string drive;
    if (!getline(is, drive, ':')) return false;
    if (!getline(is, file_name, ':')) return false;
    file_name = drive + ":" + file_name;
#else
    if (!getline(is, file_name, ':')) return false;
#endif
    if (!(is >> line_number)) return false;

    while (is.peek() == ' ' || is.peek() == ':') is.get();
    if (!getline(is, line)) return false;

    if (line.compare(0, 13, "Check failed:") == 0) {
      size_t end_pos = line.find(":", 13);
      if (end_pos == std::string::npos) return false;
      check_msg = line.substr(0, end_pos + 1) + ' ';
      line = line.substr(end_pos + 1);
    }
    return true;
  };
  // if not in correct format, do not do any rewrite.
  if (!parse_log_header()) return err_msg;
  // Parse error type.
  {
    size_t start_pos = 0, end_pos;
    for (; start_pos < line.length() && line[start_pos] == ' '; ++start_pos) {
    }
    for (end_pos = start_pos; end_pos < line.length(); ++end_pos) {
      char ch = line[end_pos];
      if (ch == ':') {
        error_type = line.substr(start_pos, end_pos - start_pos);
        break;
      }
      // [A-Z0-9a-z_.]
      if (!std::isalpha(ch) && !std::isdigit(ch) && ch != '_' && ch != '.') break;
    }
    if (error_type.length() != 0) {
      // if we successfully detected error_type: trim the following space.
      for (start_pos = end_pos + 1; start_pos < line.length() && line[start_pos] == ' ';
           ++start_pos) {
      }
      line = line.substr(start_pos);
    } else {
      // did not detect error_type, use default value.
      line = line.substr(start_pos);
      error_type = "TVMError";
    }
  }
  // Seperate out stack trace.
  std::ostringstream os;
  os << error_type << ": " << check_msg << line << '\n';

  bool trace_mode = true;
  std::vector<std::string> stack_trace;
  while (getline(is, line)) {
    if (trace_mode) {
      if (line.compare(0, 2, "  ") == 0) {
        stack_trace.push_back(line);
      } else {
        trace_mode = false;
        // remove EOL trailing stacktrace.
        if (line.length() == 0) continue;
      }
    }
    if (!trace_mode) {
      if (line.compare(0, 11, "Stack trace") == 0) {
        trace_mode = true;
      } else {
        os << line << '\n';
      }
    }
  }
  if (stack_trace.size() != 0 || file_name.length() != 0) {
    os << "Stack trace:\n";
    if (file_name.length() != 0) {
      os << "  File \"" << file_name << "\", line " << line_number << "\n";
    }
    // Print out stack traces, optionally trim the c++ traces
    // about the frontends (as they will be provided by the frontends).
    bool ffi_boundary = false;
    for (const auto& line : stack_trace) {
      // Heuristic to detect python ffi.
      if (line.find("libffi.so") != std::string::npos ||
          line.find("core.cpython") != std::string::npos) {
        ffi_boundary = true;
      }
      // If the backtrace is not c++ backtrace with the prefix "  [bt]",
      // then we can stop trimming.
      if (ffi_boundary && line.compare(0, 6, "  [bt]") != 0) {
        ffi_boundary = false;
      }
      if (!ffi_boundary) {
        os << line << '\n';
      }
      // The line after TVMFuncCall cound be in FFI.
      if (line.find("(TVMFuncCall") != std::string::npos) {
        ffi_boundary = true;
      }
    }
  }
  return os.str();
}

}  // namespace runtime
}  // namespace cvm

using namespace cvm::runtime;

class CVMRuntimeEntry {
 public:
  std::string ret_str;
  std::string last_error;
  CVMByteArray ret_bytes;
};

typedef ThreadLocalStore<CVMRuntimeEntry> CVMAPIRuntimeStore;

CVM_DLL int CVMFuncFree(CVMFunctionHandle func) {
  API_BEGIN();
  delete static_cast<PackedFunc*>(func);
  API_END();
}

void CVMAPISetLastError(const char* msg) { CVMAPIRuntimeStore::Get()->last_error = msg; }

const char* CVMGetLastError() {
  return "GetLastError";  // TODO
}

int CVMFuncCall(CVMFunctionHandle func, CVMValue* arg_values, int* type_codes, int num_args,
                CVMValue* ret_val, int* ret_type_code) {
  API_BEGIN();
  CVMRetValue rv;
  (*static_cast<const PackedFunc*>(func))
      .CallPacked(CVMArgs(arg_values, type_codes, num_args), &rv);
  // handle return string
  if (rv.type_code() == kCVMStr || rv.type_code() == kCVMDataType || rv.type_code() == kCVMBytes) {
    CVMRuntimeEntry* e = CVMAPIRuntimeStore::Get();
    if (rv.type_code() != kCVMDataType) {
      e->ret_str = *rv.ptr<std::string>();
    } else {
      e->ret_str = rv.operator std::string();
    }
    if (rv.type_code() == kCVMBytes) {
      e->ret_bytes.data = e->ret_str.c_str();
      e->ret_bytes.size = e->ret_str.length();
      *ret_type_code = kCVMBytes;
      ret_val->v_handle = &(e->ret_bytes);
    } else {
      *ret_type_code = kCVMStr;
      ret_val->v_str = e->ret_str.c_str();
    }
  } else {
    rv.MoveToCHost(ret_val, ret_type_code);
  }
  API_END();
}

int CVMCFuncSetReturn(CVMRetValueHandle ret, CVMValue* value, int* type_code, int num_ret) {
  API_BEGIN();
  ICHECK_EQ(num_ret, 1);
  CVMRetValue* rv = static_cast<CVMRetValue*>(ret);
  *rv = CVMArgValue(value[0], type_code[0]);
  API_END();
}

int CVMCbArgToReturn(CVMValue* value, int* code) {
  API_BEGIN();
  cvm::runtime::CVMRetValue rv;
  rv = cvm::runtime::CVMMovableArgValue_(*value, *code);
  rv.MoveToCHost(value, code);
  API_END();
}

int CVMFuncCreateFromCFunc(CVMPackedCFunc func, void* resource_handle, CVMPackedFuncFinalizer fin,
                           CVMFunctionHandle* out) {
  API_BEGIN();
  if (fin == nullptr) {
    *out = new PackedFunc([func, resource_handle](CVMArgs args, CVMRetValue* rv) {
      int ret = func(const_cast<CVMValue*>(args.values), const_cast<int*>(args.type_codes),
                     args.num_args, rv, resource_handle);
      if (ret != 0) throw cvm::Error(CVMGetLastError() + cvm::runtime::Backtrace());
    });
  } else {
    std::shared_ptr<void> rpack(resource_handle, fin);
    *out = new PackedFunc([func, rpack](CVMArgs args, CVMRetValue* rv) {
      int ret = func(const_cast<CVMValue*>(args.values), const_cast<int*>(args.type_codes),
                     args.num_args, rv, rpack.get());
      if (ret != 0) throw cvm::Error(CVMGetLastError() + cvm::runtime::Backtrace());
    });
  }
  API_END();
}

int CVMAPIHandleException(const std::exception& e) {
  CVMAPISetLastError(NormalizeError(e.what()).c_str());
  return -1;
}