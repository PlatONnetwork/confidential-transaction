Address platon_caller_m(const char* origin, const char* caller,
                        const char* file) {
  //   funcName.push(caller);
  return platon_caller();
}
#define platon_caller() STACK_HELP(platon_caller)
#pragma once

// get data from the RLP instance
template <class T>
inline void fetch_m(const char* origin, const char* caller, const char* file,
                    const RLP& rlp, T& value) {
  Gas gas(origin, caller, file);
  //   funcName.push(caller);
  fetch(rlp, value);
}

#define fetch(rlp, value) STACK_HELP(fetch, rlp, value)

void platon_set_state_m(const char* origin, const char* caller,
                        const char* file, const uint8_t* key, size_t klen,
                        const uint8_t* value, size_t vlen) {
  Gas gas(origin, caller, file);
  platon_set_state(key, klen, value, vlen);
}
#define platon_set_state(key, klen, value, vlen) \
  STACK_HELP(platon_set_state, key, klen, value, vlen)

int32_t platon_get_state_m(const char* origin, const char* caller,
                           const char* file, const uint8_t* key, size_t klen,
                           uint8_t* value, size_t vlen) {
  Gas gas(origin, caller, file);
  return platon_get_state(key, klen, value, vlen);
}

#define platon_get_state(key, klen, value, vlen) \
  STACK_HELP(platon_get_state, key, klen, value, vlen)