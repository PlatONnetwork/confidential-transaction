#pragma once

#include <platon/platon.h>
#include <stdint.h>
#include "platon/escape_event.hpp"

namespace privacy {
typedef int128_t i128;

class PrivacyRevert {
  PLATON_EVENT0(PrivacyRevertEvent, const std::string &)
 public:
  static void Revert(const std::string &log) {
    PrivacyRevert pr;
    pr.EmitRevert(log);
  }

 private:
  void EmitRevert(const std::string &log) {
    PLATON_EMIT_EVENT0(PrivacyRevertEvent, log);
  }
};

#define privacy_assert(A, ...)                                       \
  ::privacy::privacy_assert_aux(A, #A, __LINE__, __FILE__, __func__, \
                                ##__VA_ARGS__)

template <typename... Args>
inline void privacy_assert_aux(bool cond, const char *cond_str, unsigned line,
                               const char *file, const char *func,
                               Args &&... args) {
  if (!cond) {
    std::string all_info;
    print(all_info, std::forward<Args>(args)...);
    println("Assertion failed:", cond_str, "func:", func, "line:", line,
            "file:", file, all_info);
    PrivacyRevert::Revert(all_info);
    ::platon_revert();
  }
}

template <typename VALUE, unsigned N = 40>
inline void set_state(const uint8_t *key, size_t klen, const VALUE &value) {
  RLPStream value_stream;
  bytes vect_value(sizeof(value_prefix), value_prefix);
  value_stream.appendPrefix(vect_value);
  value_stream.reserve(vect_value.size() + N);
  value_stream << value;
  const bytesRef out = value_stream.out();
  ::platon_set_state(key, klen, out.data(), out.size());
}

template <typename VALUE>
inline size_t get_state(const uint8_t *key, size_t klen, VALUE &value) {
  size_t len = ::platon_get_state_length(key, klen);
  privacy_assert(len != 0, "key does not exist");
  bytes result;
  result.resize(len);
  ::platon_get_state(key, klen, result.data(), result.size());

  fetch(RLP(result.data() + sizeof(value_prefix),
            result.size() - sizeof(value_prefix)),
        value);
  return len;
}

template <typename T>
bytes SerializeResult(const T &result) {
  RLPStream stream;
  stream << result;
  bytes out;
  out.resize(stream.out().size());
  memcpy(out.data(), stream.out().data(), stream.out().size());
  return std::move(out);
}

template <typename T>
bytesRef SerializeResultRef(const T &result) {
  // The execution time of the contract is short, so there is no need for memory
  // recovery. By means of reference, the copy times are reduced and Gas
  // consumption is reduced
  RLPStream &stream = *new RLPStream();
  stream << result;
  return stream.out();
}

}  // namespace privacy
