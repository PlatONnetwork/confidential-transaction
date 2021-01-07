#pragma once

#include <platon/platon.h>
#include "privacy/contract_proxy.h"

namespace privacy {
class RegistryProxy : public ContractProxy {
 public:
  explicit RegistryProxy(const Address &addr) : ContractProxy(addr) {}

 public:
  PROXY_INTERFACE(GetContractAddress, platon::Address, const std::string &)
  PROXY_INTERFACE(SetContractAddress, bool, const std::string &,
                  const platon::Address &)
  PROXY_INTERFACE(GetManager, platon::Address, const std::string &)
  PROXY_INTERFACE(SetManager, bool, const std::string &,
                  const platon::Address &)
};
}  // namespace privacy
