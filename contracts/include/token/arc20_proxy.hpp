#pragma once

#include <platon/platon.h>

#include "token/iarc20.h"

#include "privacy/contract_proxy.h"

namespace privacy {
class Arc20Proxy : ContractProxy {
 public:
  explicit Arc20Proxy(const Address &addr) : ContractProxy(addr) {}

  PROXY_INTERFACE(GetTotalSupply, u128)

  PROXY_INTERFACE(BalanceOf, u128, const Address &)

  PROXY_INTERFACE(Allowance, u128, const Address &, const Address &)

  PROXY_INTERFACE(Transfer, bool, const Address &, u128)

  PROXY_INTERFACE(TransferFrom, bool, const Address &, const Address &, u128)

  PROXY_INTERFACE(Approve, bool, const Address &, u128)

 private:
  const u128 kValue = 0;
};

}  // namespace privacy