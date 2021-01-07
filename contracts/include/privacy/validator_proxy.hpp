#pragma once
#include "privacy/contract_proxy.h"
#include "privacy/validator_interface.hpp"

namespace privacy {
class ValidatorProxy : public ContractProxy {
 public:
  ValidatorProxy() = default;
  explicit ValidatorProxy(const platon::Address &addr) : ContractProxy(addr) {}

 public:
  PROXY_INTERFACE(ValidateProof, platon::bytesConstRef,
                  const platon::bytesConstRef &)
  PROXY_INTERFACE(ValidateSignature, bool, const platon::bytes &,
                  const platon::h256 &, const platon::bytesConstRef &)
  PROXY_INTERFACE(SupportProof, bool, uint32_t)

  PROXY_INTERFACE(Migrate, platon::Address, const platon::Address &)
};
}  // namespace privacy
