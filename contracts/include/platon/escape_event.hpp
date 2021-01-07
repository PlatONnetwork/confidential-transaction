#pragma once

#include "platon/RLP.h"
#include "platon/chain.hpp"
#include "platon/common.h"
#include "platon/escape_event.hpp"
#include "platon/rlp_extend.hpp"
#include "platon/rlp_size.hpp"

// Cancel the previous macro definition
#ifdef PLATON_EVENT0
#undef PLATON_EVENT0
#endif
#ifdef PLATON_EMIT_EVENT0
#undef PLATON_EMIT_EVENT0
#endif

#ifdef PLATON_EVENT1
#undef PLATON_EVENT1
#endif
#ifdef PLATON_EMIT_EVENT1
#undef PLATON_EMIT_EVENT1
#endif

#ifdef PLATON_EVENT2
#undef PLATON_EVENT2
#endif
#ifdef PLATON_EMIT_EVENT2
#undef PLATON_EMIT_EVENT2
#endif

#ifdef PLATON_EVENT3
#undef PLATON_EVENT3
#endif
#ifdef PLATON_EMIT_EVENT3
#undef PLATON_EMIT_EVENT3
#endif

// macro definition
#define PLATON_EVENT0(NAME, ...)                          \
  EVENT void NAME(VA_F(__VA_ARGS__)) {                    \
    platon::escape::emit_event0(#NAME PA_F(__VA_ARGS__)); \
  }

#define PLATON_EMIT_EVENT0(NAME, ...) NAME(__VA_ARGS__)

#define PLATON_EVENT1(NAME, TOPIC_TYPE, ...)                     \
  EVENT1 void NAME(TOPIC_TYPE topic _VA_F(__VA_ARGS__)) {        \
    platon::escape::emit_event1(#NAME, topic PA_F(__VA_ARGS__)); \
  }

#define PLATON_EMIT_EVENT1(NAME, ...) NAME(__VA_ARGS__)

#define PLATON_EVENT2(NAME, TOPIC_TYPE1, TOPIC_TYPE2, ...)                \
  EVENT2 void NAME(TOPIC_TYPE1 topic1,                                    \
                   TOPIC_TYPE2 topic2 _VA_F(__VA_ARGS__)) {               \
    platon::escape::emit_event2(#NAME, topic1, topic2 PA_F(__VA_ARGS__)); \
  }

#define PLATON_EMIT_EVENT2(NAME, ...) NAME(__VA_ARGS__)

#define PLATON_EVENT3(NAME, TOPIC_TYPE1, TOPIC_TYPE2, TOPIC_TYPE3, ...) \
  EVENT3 void NAME(TOPIC_TYPE1 topic1, TOPIC_TYPE2 topic2,              \
                   TOPIC_TYPE3 topic3 _VA_F(__VA_ARGS__)) {             \
    platon::escape::emit_event3(#NAME, topic1, topic2,                  \
                                topic3 PA_F(__VA_ARGS__));              \
  }

#define PLATON_EMIT_EVENT3(NAME, ...) NAME(__VA_ARGS__)

namespace platon {
namespace escape {

// topic data type
template <typename T, bool>
struct event_topic_type {
  using type = bytesConstRef;
};

template <typename T>
struct event_topic_type<T, true> {
  using type = typename std::decay<T>::type;
};

// bytes convert function
template <typename T>
bytesConstRef event_bytes_data_convert(const T &data) {
  if (data.size() <= 32) {
    bytesConstRef result(reinterpret_cast<const uint8_t *>(data.data()),
                         data.size());
    return result;
  }

  platon::h256 &hash = *new platon::h256;
  bytesConstRef result(hash.data(), platon::h256::size);
  ::platon_sha3(reinterpret_cast<const uint8_t *>(data.data()), data.size(),
                const_cast<uint8_t *>(result.data()), result.size());
  return result;
}

template <typename T>
bytesConstRef event_other_data_convert(const T &data) {
  RLPStream &stream = *new RLPStream();
  stream.reserve(pack_size(data));
  stream << data;
  bytesConstRef result(stream.out().data(), stream.out().size());
  if (result.size() <= 32) return result;

  platon::h256 &hash = *new platon::h256;
  bytesConstRef hash_result(hash.data(), platon::h256::size);
  ::platon_sha3(reinterpret_cast<const uint8_t *>(result.data()), result.size(),
                const_cast<uint8_t *>(hash_result.data()), hash_result.size());
  return hash_result;
}

// convert dispatch class
template <bool>
struct dispatch_convert_type {
  template <typename T>
  static bytesConstRef dispatch_convert(const T &one_data) {
    return platon::escape::event_other_data_convert(one_data);
  }

  template <std::size_t N>
  static bytesConstRef dispatch_convert(const std::array<int8_t, N> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  template <std::size_t N>
  static bytesConstRef dispatch_convert(
      const std::array<uint8_t, N> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const std::vector<int8_t> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const std::vector<uint8_t> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const bytesConstRef &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const std::list<int8_t> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const std::list<uint8_t> &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  template <unsigned N>
  static bytesConstRef dispatch_convert(const FixedHash<N> &one_data) {
    if (one_data.size <= 32) {
      bytesConstRef result(reinterpret_cast<const uint8_t *>(one_data.data()),
                           one_data.size);
      return result;
    }

    platon::h256 &hash = *new platon::h256;
    bytesConstRef result(hash.data(), platon::h256::size);
    ::platon_sha3(reinterpret_cast<const uint8_t *>(one_data.data()),
                  one_data.size, const_cast<uint8_t *>(result.data()),
                  result.size());
    return result;
  }

  static bytesConstRef dispatch_convert(const std::string &one_data) {
    return platon::escape::event_bytes_data_convert(one_data);
  }

  static bytesConstRef dispatch_convert(const char *one_data) {
    if (strlen(one_data) <= 32) {
      bytesConstRef result(reinterpret_cast<const uint8_t *>(one_data),
                           strlen(one_data));
      return result;
    }

    platon::h256 &hash = *new platon::h256;
    bytesConstRef result(hash.data(), platon::h256::size);
    ::platon_sha3(reinterpret_cast<const uint8_t *>(one_data), strlen(one_data),
                  const_cast<uint8_t *>(result.data()), result.size());
    return result;
  }
};

template <>
struct dispatch_convert_type<true> {
  template <typename T>
  static typename std::decay<T>::type dispatch_convert(const T &one_data) {
    return one_data;
  }
};

// real convert function
template <typename T>
constexpr bool is_data_number() {
  return std::numeric_limits<std::decay_t<T>>::is_integer ||
         std::numeric_limits<std::decay_t<T>>::is_iec559;
}

template <typename T, bool is_number = is_data_number<T>()>
typename event_topic_type<T, is_number>::type event_data_convert(
    const T &data) {
  return dispatch_convert_type<is_number>::dispatch_convert(data);
}

/**
 * @brief Send events that are unindexed and anonymous
 *
 * @param args Any number of event parameters of any type
 *
 * @return void
 */
template <typename... Args>
inline void emit_event(const Args &... args) {
  RLPStream stream;
  event_args(stream, args...);
  bytesRef topic_data = stream.out();
  ::platon_event(NULL, 0, topic_data.data(), topic_data.size());
}

/**
 * @brief Send event items that are not indexed
 *
 * @param name The name of the event
 * @param args Any number of event parameters of any type
 *
 * @return void
 */
template <typename... Args>
inline void emit_event0(const std::string &name, const Args &... args) {
  RLPStream stream(1);
  auto event_sign = platon::escape::event_data_convert(name);
  stream.reserve(pack_size(event_sign));
  stream << event_sign;
  const bytesRef topic_data = stream.out();
  RLPStream args_stream;
  event_args(args_stream, args...);
  bytesRef args_data = args_stream.out();
  ::platon_event(topic_data.data(), topic_data.size(), args_data.data(),
                 args_data.size());
}

/**
 * @brief Sends an event that has only one index entry
 *
 * @param name The name of the event
 * @param topic Event index value
 * @param args Any number of event parameters of any type
 *
 * @return void
 */
template <class Topic, typename... Args>
inline void emit_event1(const std::string &name, const Topic &topic,
                        const Args &... args) {
  RLPStream stream(2);
  auto event_sign = platon::escape::event_data_convert(name);
  auto topic1_data = platon::escape::event_data_convert(topic);
  RLPSize rlps;
  rlps << event_sign << topic1_data;
  stream.reserve(rlps.size());
  stream << event_sign << topic1_data;
  const bytesRef topic_data = stream.out();
  RLPStream args_stream;
  event_args(args_stream, args...);
  bytesRef rlp_data = args_stream.out();
  ::platon_event(topic_data.data(), topic_data.size(), rlp_data.data(),
                 rlp_data.size());
}

/**
 * @brief Sends an event with two index values
 *
 * @param name The name of the event
 * @param topic1 The first index value of the event
 * @param topic2 The second index value of the event
 * @param args Any number of event parameters of any type
 *
 * @return void
 */
template <class Topic1, class Topic2, typename... Args>
inline void emit_event2(const std::string &name, const Topic1 &topic1,
                        const Topic2 &topic2, const Args &... args) {
  RLPStream stream(3);
  auto event_sign = platon::escape::event_data_convert(name);
  auto topic1_data = platon::escape::event_data_convert(topic1);
  auto topic2_data = platon::escape::event_data_convert(topic2);
  RLPSize rlps;
  rlps << event_sign << topic1_data << topic2_data;
  stream.reserve(rlps.size());
  stream << event_sign << topic1_data << topic2_data;
  const bytesRef topic_data = stream.out();
  RLPStream args_stream;
  event_args(args_stream, args...);
  bytesRef rlp_data = args_stream.out();
  ::platon_event(topic_data.data(), topic_data.size(), rlp_data.data(),
                 rlp_data.size());
}

/**
 * @brief Sends an event with three event index values
 *
 * @param name The name of the event
 * @param topic1 The first index value of the event
 * @param topic2 The second index value of the event
 * @param topic3 The third index value of the event
 * @param args Any number of event parameters of any type
 *
 * @return void
 */
template <class Topic1, class Topic2, class Topic3, typename... Args>
inline void emit_event3(const std::string &name, const Topic1 &topic1,
                        const Topic2 &topic2, const Topic3 &topic3,
                        const Args &... args) {
  RLPStream stream(4);
  auto event_sign = platon::escape::event_data_convert(name);
  auto topic1_data = platon::escape::event_data_convert(topic1);
  auto topic2_data = platon::escape::event_data_convert(topic2);
  auto topic3_data = platon::escape::event_data_convert(topic3);
  RLPSize rlps;
  rlps << event_sign << topic1_data << topic2_data << topic3_data;
  stream.reserve(rlps.size());
  stream << event_sign << topic1_data << topic2_data << topic3_data;
  const bytesRef topic_data = stream.out();
  RLPStream args_stream;
  event_args(args_stream, args...);
  bytesRef rlp_data = args_stream.out();
  ::platon_event(topic_data.data(), topic_data.size(), rlp_data.data(),
                 rlp_data.size());
}
}  // namespace escape
}  // namespace platon
