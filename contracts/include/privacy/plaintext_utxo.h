#pragma once
#include <platon/platon.h>
#include "common.hpp"
namespace privacy {

struct PlaintextProof {
  // Proof data, each proof type has a corresponding contract, no external
  // pointing type is required.
  platon::bytes data;
  platon::bytes signature;
  PLATON_SERIALIZE(PlaintextProof, (data)(signature));
};

struct PlaintextData {
  uint32_t version;    // Version information of specific parameters
  platon::bytes data;  // Find the decoder to decode according to version
  PLATON_SERIALIZE(PlaintextData, (version)(data));
};

// struct PlaintextOutputNote {
//  platon::Address owner;
//  platon::u128 value;
//  PLATON_SERIALIZE(PlaintextOutputNote, (owner)(value));
//};

struct PlaintextNote {
  platon::bytes owner;
  platon::u128 value;
  platon::bytes random;
  PLATON_SERIALIZE(PlaintextNote, (owner)(value)(random))
};
struct PlaintextOutputNote : public PlaintextNote {
  platon::bytes meta_data;
  PLATON_SERIALIZE(PlaintextOutputNote, (owner)(value)(random)(meta_data));
};

struct SpenderNote : public PlaintextNote {
  platon::Address spender;
  PLATON_SERIALIZE(SpenderNote, (owner)(value)(random)(spender));
};
struct PlaintextInputNote : public SpenderNote {
  platon::bytes signature;
  PLATON_SERIALIZE(PlaintextInputNote, (owner)(value)(random)(signature));
};

struct PlaintextUTXO {
  std::vector<PlaintextInputNote> inputs;
  std::vector<PlaintextOutputNote> outputs;
  platon::Address public_owner;  // The transfer account of the public amount
  i128 public_value;  // Disclosure amount, amount transferred in or out from
                      // the privacy contract
  platon::bytes meta_data;  // Remarks information
  PLATON_SERIALIZE(PlaintextUTXO,
                   (inputs)(outputs)(public_owner)(public_value)(meta_data));
};

struct PlaintextApprove : public PlaintextNote {
  platon::bytes shared_sign;  // Shared signature
  PLATON_SERIALIZE(PlaintextApprove, (owner)(value)(random)(shared_sign));
};

struct PlaintextMint {
  platon::h256 old_mint_hash;  // Last minting proof hash
  std::vector<PlaintextOutputNote> outputs;
  PLATON_SERIALIZE(PlaintextMint, (old_mint_hash)(outputs));
};

struct PlaintextBurn {
  platon::h256 old_burn_hash;  // The last time the proof hash was destroyed
  std::vector<PlaintextInputNote> inputs;
  PLATON_SERIALIZE(PlaintextBurn, (old_burn_hash)(inputs));
};

}  // namespace privacy
