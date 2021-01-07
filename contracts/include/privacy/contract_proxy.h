#pragma once

#include <platon/platon.h>
#include "platon/call.hpp"
#define PROXY_INTERFACE(NAME, RETURN, ...)                        \
  RETURN NAME(VA_F(__VA_ARGS__)) {                                \
    auto res = escape::platon_call_with_return_value<RETURN>(     \
        addr_, u128(0), ::platon_gas(), #NAME PA_F(__VA_ARGS__)); \
    privacy_assert(res.second, "call contract failed");           \
    return res.first;                                             \
  }

#define PROXY_INTERFACE_VOID(NAME, ...)                                      \
  void NAME(VA_F(__VA_ARGS__)) {                                             \
    bytesConstRef paras =                                                    \
        escape::cross_call_args_ref(#NAME PA_F(__VA_ARGS__));                \
    bytes value_bytes = value_to_bytes(u128(0));                             \
    bytes gas_bytes = value_to_bytes(::platon_gas());                        \
    int32_t result = ::platon_call(addr_.data(), paras.data(), paras.size(), \
                                   value_bytes.data(), value_bytes.size(),   \
                                   gas_bytes.data(), gas_bytes.size());      \
    privacy_assert(0 == result, "call contract failed");                     \
  }

class ContractProxy {
 public:
  ContractProxy() = default;
  explicit ContractProxy(Address addr) : addr_(addr) {}

  void SetContractProxy(const Address &proxy) {
    privacy_assert(proxy != Address(), "invalid proxy contract address");
    addr_ = proxy;
  }
  
  void GetContractProxy(const Address &proxy) { addr_ = proxy; }

 protected:
  Address addr_;
};