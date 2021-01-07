#pragma once

constexpr const char* file_name(const char* path) {
  const char* file = path;
  while (*path) {
    if (*path++ == '/') {
      file = path;
    }
  }
  return file;
}

constexpr const size_t str_len(const char* str) {
  size_t result = 0;
  if (str) {
    while (*str++) ++result;
  }
  return result;
}

#define STACK_HELP(NAME, ...) \
  NAME##_m(#NAME, __FUNCTION__, file_name(__FILE__), ##__VA_ARGS__)

class Gas {
  const char* origin_ = nullptr;
  const char* caller_ = nullptr;
  const char* file_ = nullptr;
  uint64_t gas_;

 public:
  Gas() : gas_(platon_gas()) {}
  Gas(const char* origin) : origin_(origin), gas_(platon_gas()) {}

  Gas(const char* origin, const char* caller, const char* file)
      : origin_(origin), caller_(caller), file_(file), gas_(platon_gas()) {}
  ~Gas() { print(); }
  void Reset(const char* origin, const char* caller, const char* file) {
    print();
    origin_ = origin;
    caller_ = caller;
    file_ = file;
    gas_ = platon_gas();
  }
  void print() {
    uint64_t cost = gas_ - platon_gas();
    printf("gas file :%s origin:%s caller:%s cost:%lld debug: 8000", file_, origin_, caller_, cost);
    // platon_debug((const uint8_t*)"gas file: ", str_len("gas file "));
    // platon_debug((const uint8_t*)file_, str_len(file_));
    // platon_debug((const uint8_t*)" origin:", str_len(" origin:"));
    // platon_debug((const uint8_t*)origin_, str_len(origin_));
    // platon_debug((const uint8_t*)" caller:", str_len(" caller:"));
    // platon_debug((const uint8_t*)caller_, str_len(caller_));
    // platon_debug((const uint8_t*)"\n", str_len("\n"));
    // println("gas file:", file_ != nullptr ? file_ : "",
    //         "origin:", origin_ != nullptr ? origin_ : "",
    //         "caller:", caller_ != nullptr ? caller_ : "", "cost:", cost);
  }
};

// SIMULATOR
#define SIMULATOR_DEFINE(NAME)                                  \
  struct NAME##_simulator {                                     \
    using func_type = decltype(&NAME);                          \
    NAME##_simulator(const char* origin) : gasGuard_(origin) {} \
    NAME##_simulator(const char* origin, const char* caller)    \
        : gasGuard_(origin, caller) {}                          \
    func_type get_func() const { return &NAME; }                \
    Gas gasGuard_;                                              \
  };

#define NAMESPACE_SIMULATOR_DEFINE(NAMESPACE, NAME) \
  namespace NAMESPACE {                             \
  SIMULATOR_DEFINE(NAME)                            \
  }

#define SIMULATOR_CALL(NAME) NAME##_simulator(#NAME, __func__).get_func()
