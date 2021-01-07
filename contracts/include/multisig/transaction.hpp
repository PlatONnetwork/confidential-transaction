#pragma once
#include <platon/platon.h>

namespace multisig {
struct Transaction {
  platon::Address sender_;
  platon::Address to_;
  platon::bytes paras_;
  int64_t timestamp_;
  uint32_t secondLimit_;
  std::vector<platon::Address> signedAddress_;
  PLATON_SERIALIZE(Transaction, (sender_)(to_)(paras_)(timestamp_)(
                                    secondLimit_)(signedAddress_))
};

const uint64_t KOwners = platon::name_value("owners");
const uint64_t KRequired = platon::name_value("required");
const uint64_t KTransactionId = platon::name_value("transaction_id");

}  // namespace multisig