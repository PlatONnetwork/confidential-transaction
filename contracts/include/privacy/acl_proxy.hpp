#pragma once
#include <platon/platon.h>
#include "privacy/acl_interface.h"
#include "privacy/common.hpp"
#include "privacy/contract_proxy.h"
#include "privacy/utxo.h"

namespace privacy {
class AclProxy : public ContractProxy {
 public:
  AclProxy() = default;
  explicit AclProxy(const Address &addr) : ContractProxy(addr) {}

 public:
  PROXY_INTERFACE(CreateRegistry, bool, uint32_t, uint32_t, u128,
                  const Address &, bool)
  PROXY_INTERFACE(ValidateProof, bytesConstRef, const bytesConstRef &)
  PROXY_INTERFACE(Transfer, bytesConstRef, const bytesConstRef &)
  PROXY_INTERFACE_VOID(UpdateNotes, const bytesConstRef &)
  PROXY_INTERFACE(Approve, bytesConstRef, const bytesConstRef &)
  PROXY_INTERFACE(GetApproval, bytesConstRef, const h256 &)
  PROXY_INTERFACE(Mint, bytesConstRef, const bytesConstRef &)
  PROXY_INTERFACE(Burn, bytesConstRef, const bytesConstRef &)
  PROXY_INTERFACE(GetNote, NoteStatus, const h256 &)
  PROXY_INTERFACE(ValidateSignature, bool, const bytesConstRef &, const h256 &,
                  const bytesConstRef &)
  PROXY_INTERFACE(GetRegistry, Registry, const Address &)
  PROXY_INTERFACE(UpdateValidator, bool, uint32_t)
  PROXY_INTERFACE(UpdateStorage, bool, uint32_t)
  PROXY_INTERFACE(SupportProof, bool, uint32_t)
};
};  // namespace privacy
