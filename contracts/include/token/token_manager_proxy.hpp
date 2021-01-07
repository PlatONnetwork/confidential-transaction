#pragma once

#include <platon/platon.h>

#include "token/iarc20.h"

#include "privacy/contract_proxy.h"

namespace privacy {
class TokenManagerProxy : ContractProxy {
 public:
  explicit TokenManagerProxy(const Address &addr) : ContractProxy(addr) {}

  PROXY_INTERFACE(SetOwner, bool, const platon::Address &)

  PROXY_INTERFACE(GetOwner, platon::Address)

  PROXY_INTERFACE(Deposit, bool, const platon::Address &,
                  const platon::Address &, platon::u128)

  PROXY_INTERFACE(Withdraw, bool, const platon::Address &,
                  const platon::Address &, platon::u128)
};

}  // namespace privacy