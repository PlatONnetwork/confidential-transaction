#pragma once

namespace platon {

// rlp encode
template <typename T>
inline void rlp_encode_size(RLPSize &sizer, const T &head) {
  sizer << head;
}

template <typename T, typename... Args>
inline void rlp_encode_size(RLPSize &sizer, const T &head,
                            const Args &... args) {
  sizer << head;
  rlp_encode_size(sizer, args...);
}

template <typename T>
inline void rlp_encode_stream(RLPStream &stream, const T &head) {
  stream << head;
}

template <typename T, typename... Args>
inline void rlp_encode_stream(RLPStream &stream, const T &head,
                              const Args &... args) {
  stream << head;
  rlp_encode_stream(stream, args...);
}

template <typename... Args>
inline bytesConstRef rlp_encode(const Args &... args) {
  RLPSize sizer;
  rlp_encode_size(sizer, args...);

  RLPStream &stream = *new RLPStream();
  stream.reserve(sizer.size());

  size_t num = sizeof...(Args);
  stream.appendList(num);
  rlp_encode_stream(stream, args...);
  return bytesConstRef(stream.out().data(), stream.out().size());
}

namespace escape {
/**
 * @brief Construct the parameters of the call across contracts
 *
 * @param method The method name of the invoked contract
 * @param args The parameters corresponding to the contract method
 *
 * @return Parameter byte array
 */
template <typename... Args>
inline bytesConstRef cross_call_args_ref(const std::string &method,
                                         const Args &... args) {
  uint64_t t_method = Name(method).value;

  RLPSize sizer;
  rlp_encode_size(sizer, t_method, args...);

  RLPStream &stream = *new RLPStream();
  stream.reserve(sizer.size());

  size_t num = sizeof...(Args);
  stream.appendList(num + 1);
  rlp_encode_stream(stream, t_method, args...);
  return bytesConstRef(stream.out().data(), stream.out().size());
}

template <typename value_type, typename gas_type, typename... Args>
std::pair<Address, bool> platon_create_contract(const Address &address,
                                                value_type value, gas_type gas,
                                                const Args &... init_args) {
  // value and gas
  bytes value_bytes = value_to_bytes(value);
  bytes gas_bytes = value_to_bytes(gas);

  // init args
  bytesConstRef init_rlp = cross_call_args_ref("init", init_args...);

  // clone contract
  Address return_address;
  bool success =
      platon_clone(address.data(), return_address.data(), init_rlp.data(),
                   init_rlp.size(), value_bytes.data(), value_bytes.size(),
                   gas_bytes.data(), gas_bytes.size()) == 0;
  return std::make_pair(return_address, success);
}

/**
 * @brief Normal cross-contract invocation
 *
 * @param addr The contract address to be invoked
 * @param value The amount transferred to the contract
 * @param gas The called contract method estimates the gas consumed
 * @param method The method name of the invoked contract
 * @param args The parameters corresponding to the contract method
 *
 * @return The contract method returns whether the execution was
 * successful
 *
 * Example:
 *
 * @code
 *
  auto address_pair =
 make_address("lax10jc0t4ndqarj4q6ujl3g3ycmufgc77epxg02lt"); bool result =
 platon_call(address_pair.first,
  uint32_t(100), uint32_t(100), "add", 1,2,3);
  if(!result){
    platon_throw("cross call fail");
  }
 * @endcode
 */
template <typename value_type, typename gas_type, typename... Args>
inline bool platon_call(const Address &addr, const value_type &value,
                        const gas_type &gas, const std::string &method,
                        const Args &... args) {
  bytes value_bytes = value_to_bytes(value);
  bytes gas_bytes = value_to_bytes(gas);
  bytesConstRef paras = cross_call_args_ref(method, args...);

  int32_t result =
      ::platon_call(addr.data(), paras.data(), paras.size(), value_bytes.data(),
                    value_bytes.size(), gas_bytes.data(), gas_bytes.size());
  return 0 == result;
}

/**
 * @brief Normal cross-contract invocation
 *
 * @param addr The contract address to be invoked
 * @param value The amount transferred to the contract
 * @param gas The called contract method estimates the gas consumed
 * @param method The method name of the invoked contract
 * @param args The parameters corresponding to the contract method
 *
 * @return The contract method returns the value and whether the execution was
 * successful
 *
 * Example:
 *
 * @code
 *
  auto address_pair =
 make_address("lax10jc0t4ndqarj4q6ujl3g3ycmufgc77epxg02lt"); auto result =
 platon_call_with_return_value<int>(address_pair.first,
  uint32_t(100), uint32_t(100), "add", 1,2,3);
  if(!result.second){
    platon_throw("cross call fail");
  }
 * @endcode
 */
template <typename return_type, typename value_type, typename gas_type,
          typename... Args>
inline auto platon_call_with_return_value(const Address &addr,
                                          const value_type &value,
                                          const gas_type &gas,
                                          const std::string &method,
                                          const Args &... args) {
  bool result = escape::platon_call(addr, value, gas, method, args...);
  if (!result) {
    return std::pair<return_type, bool>(return_type(), false);
  }

  return_type return_value;
  get_call_output(return_value);
  return std::pair<return_type, bool>(return_value, true);
}
}  // namespace escape
}  // namespace platon