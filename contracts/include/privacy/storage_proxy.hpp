#pragma once
#include "privacy/contract_proxy.h"
#include "privacy/utxo.h"
namespace privacy {
class StorageProxy : public ContractProxy {
 public:
  StorageProxy() = default;
  explicit StorageProxy(const Address &addr) : ContractProxy(addr) {}

  PROXY_INTERFACE_VOID(CreateRegistry, bool)
  PROXY_INTERFACE(GetNote, NoteStatus, const h256 &)

  PROXY_INTERFACE_VOID(UpdateNotes, const bytesConstRef &)
  PROXY_INTERFACE(Approve, bytesConstRef, const bytesConstRef &)

  PROXY_INTERFACE(GetApproval, bytesConstRef, const h256 &)

  PROXY_INTERFACE_VOID(Mint, const bytesConstRef &)
  PROXY_INTERFACE_VOID(Burn, const bytesConstRef &)

  PROXY_INTERFACE(Migrate, Address, const Address &)
};

}  // namespace privacy