#pragma once

void platon_event_m(const char* origin, const char* caller, const char* file,
                    const uint8_t* topic, size_t topic_len, const uint8_t* args,
                    size_t args_len) {
  Gas gas(origin, caller, file);
  platon_event(topic, topic_len, args, args_len);
}
#define platon_event(topic, topic_len, args, args_len) \
  STACK_HELP(platon_event, topic, topic_len, args, args_len)

//   template <typename... Args>
// inline void emit_event_m(const char* origin, const char* caller, const char*
// file, const Args &... args) {
//                    Gas gas(origin, caller, file);
//      emit_event(args...);
// }
template <typename... Args>
inline void event_args_m(RLPStream& stream, const Args&... args) {
  std::tuple<Args...> tuple_args = std::make_tuple(args...);
  size_t size = 0;
  {
    Gas gas("pack_size_single", __FUNCTION__, "");
    size = pack_size(tuple_args);
  }
  {
    Gas gas("reserve size", __FUNCTION__, "");
    stream.reserve(size);
  }

  {
    Gas gas("stream <<", __FUNCTION__, "");

    stream << tuple_args;
  }
}

template <typename... Args>
void emit_event0_m(const std::string& name, const Args&... args) {
  RLPStream stream(1);

  {
    Gas gas("event_sign", __FUNCTION__, "");

    auto event_sign = event_data_convert(name);
    stream.reserve(pack_size(event_sign));
    stream << event_sign;
  }
  const bytesRef topic_data = stream.out();

  RLPStream args_stream;

  {
    Gas gas("event_args", __FUNCTION__, "");

    event_args_m(args_stream, args...);
  }
  bytesRef args_data = args_stream.out();

  ::platon_event(topic_data.data(), topic_data.size(), args_data.data(),
                 args_data.size());
}

template <class Topic, typename... Args>
void emit_event1_m(const std::string& name, const Topic& topic,
                   const Args&... args) {
  RLPStream stream(2);

  {
    Gas gas("data convert", __FUNCTION__, "");

    auto event_sign = event_data_convert(name);
    auto topic1_data = event_data_convert(topic);
    gas.Reset("data rlp size", __FUNCTION__, "");
    RLPSize rlps;
    rlps << event_sign << topic1_data;
    gas.Reset("reverve size", __FUNCTION__, "");

    stream.reserve(rlps.size());
    gas.Reset("stream topic", __FUNCTION__, "");
    stream << event_sign << topic1_data;
  }
  const bytesRef topic_data = stream.out();
  RLPStream args_stream;

  {
    Gas gas("event_args_m", __FUNCTION__, "");
    event_args_m(args_stream, args...);
  }
  bytesRef rlp_data = args_stream.out();

  ::platon_event(topic_data.data(), topic_data.size(), rlp_data.data(),
                 rlp_data.size());
}

template <class Topic1, class Topic2, typename... Args>
void emit_event2_m(const std::string& name, const Topic1& topic1,
                   const Topic2& topic2, const Args&... args) {
  RLPStream stream(3);

  {
    Gas gas("data convert", __FUNCTION__, "");
    auto event_sign = event_data_convert(name);
    auto topic1_data = event_data_convert(topic1);
    auto topic2_data = event_data_convert(topic2);
    gas.Reset("event_sign rlp size", __FUNCTION__, "");

    RLPSize rlps;
    rlps << event_sign << topic1_data << topic2_data;
    gas.Reset("reverve size", __FUNCTION__, "");

    stream.reserve(rlps.size());
    gas.Reset("event_sign", __FUNCTION__, "");

    stream << event_sign << topic1_data << topic2_data;
  }
  const bytesRef topic_data = stream.out();

  RLPStream args_stream;

  {
    Gas gas("event_args_m", __FUNCTION__, "");
    event_args_m(args_stream, args...);
  }
  bytesRef rlp_data = args_stream.out();

  ::platon_event(topic_data.data(), topic_data.size(), rlp_data.data(),
                 rlp_data.size());
}

template <class T>
bytes SerializeResult(const T&& result, const char* caller = __FUNCTION__) {
  T t = std::move(result);
  Gas gas("SerializeResult", caller, "");
  return SerializeResult(t);
}

// rlp stream wrapper
template <class T>
RLPStream& operator<<(RLPStream& stream, T& demo) {
  Gas gas("RLPStream <<", "", "");
  stream << std::as_const(demo);
  return stream;
}

#define PLATON_EVENT0(NAME, ...)            \
  EVENT void NAME(VA_F(__VA_ARGS__)) {      \
    Gas gas(__FUNCTION__, "", __FILE__);    \
    emit_event0_m(#NAME PA_F(__VA_ARGS__)); \
  }

#define PLATON_EVENT1(NAME, TOPIC_TYPE, ...)              \
  EVENT1 void NAME(TOPIC_TYPE topic _VA_F(__VA_ARGS__)) { \
    Gas gas(__FUNCTION__, "", __FILE__);                  \
    emit_event1_m(#NAME, topic PA_F(__VA_ARGS__));        \
  }

#define PLATON_EVENT2(NAME, TOPIC_TYPE1, TOPIC_TYPE2, ...)  \
  EVENT2 void NAME(TOPIC_TYPE1 topic1,                      \
                   TOPIC_TYPE2 topic2 _VA_F(__VA_ARGS__)) { \
    Gas gas(__FUNCTION__, "", __FILE__);                    \
    emit_event2_m(#NAME, topic1, topic2 PA_F(__VA_ARGS__)); \
  }

#define PLATON_EVENT3(NAME, TOPIC_TYPE1, TOPIC_TYPE2, TOPIC_TYPE3, ...)   \
  EVENT3 void NAME(TOPIC_TYPE1 topic1, TOPIC_TYPE2 topic2,                \
                   TOPIC_TYPE3 topic3 _VA_F(__VA_ARGS__)) {               \
    platon::emit_event3(#NAME, topic1, topic2, topic3 PA_F(__VA_ARGS__)); \
  }