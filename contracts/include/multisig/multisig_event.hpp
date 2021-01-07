#pragma once
#include <platon/platon.h>
#include "platon/escape_event.hpp"

namespace multisig {
class MultisigEvent {
 protected:
  using VectorAddress = std::vector<Address>;
  // contract address, owners, required
  PLATON_EVENT1(CreateMultisig, const Address&, const VectorAddress&, uint16_t)

  // contract address, owners, required
  PLATON_EVENT1(ChangeMultisig, const Address&, const VectorAddress&, uint16_t)

  // contract address, transaction id, sender, to, parameters, second limit
  PLATON_EVENT2(NewMultisigTransaction, const Address&, uint32_t,
                const Address&, const Address&, bytes, uint32_t)

  // contract address, transaction id, signature address
  PLATON_EVENT2(SignMultisigTransaction, const Address&, uint32_t,
                const Address&)

  // contract address, transaction id
  PLATON_EVENT2(MultisigTransactionTimeout, const Address&, uint32_t)

  // contract address, transaction id, all signature address
  PLATON_EVENT2(MultisigTransactionExecute, const Address&, uint32_t,
                const VectorAddress&)

  // contract address, transaction id, return transaction
  PLATON_EVENT2(MultisigCreate, const Address&, uint32_t, const Address&)

  // contract address, transaction id, return transaction
  PLATON_EVENT2(MultisigClone, const Address&, uint32_t, const Address&)
};
}  // namespace multisig
