#pragma once

#include <platon/platon.h>
#include "common.hpp"
#include "platon/vector_wrapper.hpp"
namespace privacy {

struct ConfidentialInputNote {
  platon::VectorWrapper note_id;  // unique identifier of note
  platon::VectorWrapper
      ephemeral_pk;               // Temporary public key created by the sender
  platon::VectorWrapper sign_pk;  // Encrypted address
  platon::VectorWrapper token;    // Pedersen Commitment Value
  PLATON_SERIALIZE(ConfidentialInputNote,
                   (note_id)(ephemeral_pk)(sign_pk)(token))
};

struct ConfidentialOutputNote : public ConfidentialInputNote {
  platon::VectorWrapper
      cipher_value;  // Quatity and blinding encrypted with view public key
  PLATON_SERIALIZE(ConfidentialOutputNote,
                   (note_id)(ephemeral_pk)(sign_pk)(token)(cipher_value))
};

struct ConfidentialUTXO {
  uint8_t tx_type;
  std::vector<ConfidentialInputNote> inputs;
  std::vector<ConfidentialOutputNote> outputs;
  platon::u128 public_value;  // Disclosure amount, amount transferred in or out
                              // from the privacy contract
  platon::Address authorized_address;  // The address to sign the proof data and
                                       // additional data
  PLATON_SERIALIZE(ConfidentialUTXO, (tx_type)(inputs)(outputs)(public_value)(
                                         authorized_address));
};

struct ConfidentialProof {
  platon::VectorWrapper data;       // ConfidentialData rlp encoded data
  platon::VectorWrapper signature;  // data signed data
  PLATON_SERIALIZE(ConfidentialProof, (data)(signature));
};

struct ConfidentialData {
  uint32_t version;
  platon::VectorWrapper confidential_tx;  // Transaction information required
                                          // for cryptography after rlp encoding
  platon::VectorWrapper
      extra_data;  // Additional information (different types of transaction
                   // additional information are different)
  PLATON_SERIALIZE(ConfidentialData, (version)(confidential_tx)(extra_data));
};

struct TransferExtra {
  platon::Address public_owner;  // Address for transfer in and out
  platon::VectorWrapper
      deposit_signature;  // When recharging, transfer the signature information
                          // of the account
  std::vector<platon::VectorWrapper> meta_data;  // Remarks for each output
  PLATON_SERIALIZE(TransferExtra, (public_owner)(deposit_signature)(meta_data));
};

struct MintExtra {
  platon::h256 old_mint_hash;                    // Hash of the last operation
  std::vector<platon::VectorWrapper> meta_data;  // Remarks for each output
  PLATON_SERIALIZE(MintExtra, (old_mint_hash)(meta_data));
};

struct BurnExtra {
  platon::h256 old_burn_hash;  // Hash of the last operation
  PLATON_SERIALIZE(BurnExtra, (old_burn_hash));
};

}  // namespace privacy
