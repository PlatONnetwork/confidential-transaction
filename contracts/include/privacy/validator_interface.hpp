#pragma once

#include "common.hpp"
#include "utxo.h"
namespace privacy {
struct TransferResult {
  InputNotes inputs;
  OutputNotes outputs;
  platon::Address public_owner;
  i128 public_value;
  platon::bytesConstRef sender;

  PLATON_SERIALIZE(TransferResult,
                   (inputs)(outputs)(public_owner)(public_value)(sender))
};

struct ApproveResult {
  platon::h256 note_hash;
  platon::bytesConstRef shared_sign;
  platon::bytesConstRef sender;
  PLATON_SERIALIZE(ApproveResult, (note_hash)(shared_sign)(sender))
};

struct MintResult {
  platon::h256 old_mint_hash;  // Last minting hash
  platon::h256 new_mint_hash;  // New coin hash
  platon::u128 total_mint;     // Coin amount
  OutputNotes outputs;
  platon::bytesConstRef sender;
  PLATON_SERIALIZE(MintResult,
                   (old_mint_hash)(new_mint_hash)(total_mint)(outputs)(sender))
};

struct BurnResult {
  platon::h256 old_burn_hash;  // The last time the hash was destroyed
  platon::h256 new_burn_hash;  // New destroyed hash
  platon::u128 total_burn;     // Amount of destruction
  InputNotes inputs;
  platon::bytesConstRef sender;
  PLATON_SERIALIZE(BurnResult,
                   (old_burn_hash)(new_burn_hash)(total_burn)(inputs)(sender))
};

enum class ProofType : uint8_t {
  kTransfer = 1,
  kMint = 2,
  kBurn = 3,
  kDeposit = 4,
  kWithdraw = 5,
  kApprove = 6
};

class ValidatorInterface {
  /**
   * @brief Verify that the proof is legal
   *
   * @param proof Proof byte stream
   * @return Return a serialized byte stream of authentication information
   */
  virtual platon::bytesConstRef ValidateProof(
      const platon::bytesConstRef& proof) = 0;

  /**
   * @brief Verify the legality of the signature
   *
   * @param note_sender note sender's address
   * @param hash The hash value of the signature data
   * @param signature Signature information
   * @return Return true on success, false on failure
   */
  virtual bool ValidateSignature(const platon::bytes& note_sender,
                                 const platon::h256& hash,
                                 const platon::bytesConstRef& signature) = 0;

  /**
   * @brief Determine whether the plug-in supports the specified version
   * certificate
   *
   * @param version version number
   * @return Support return true, fail return false
   */
  virtual bool SupportProof(uint32_t version) = 0;

  /**
   * @brief Migration upgrade
   *
   * @param address Template contract address
   * @return Contract address after upgrade
   */
  virtual Address Migrate(const Address& address) = 0;
};

}  // namespace privacy
