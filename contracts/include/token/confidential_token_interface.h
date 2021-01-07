#pragma once
#include <platon/platon.h>
#include "privacy/acl_proxy.hpp"
#include "privacy/validator_interface.hpp"

class ConfidentialTokenInterface {
  /**
   * @brief Transfer notes to others
   *
   * @param proof Proof of transfer
   * @return Return true on success, Failure to trigger revert operation
   */
  virtual bool Transfer(const platon::bytesConstRef &proof) = 0;

  /**
   * @brief Authorize other users to spend notes.
   *
   * @param shared_secret Shared secret,
   * @return Failure to trigger revert operation
   */
  virtual bool Approve(const platon::bytesConstRef &proof) = 0;

  /**
   * @brief Update note remarks information
   *
   * @param note_hash note hash
   * @param meta_data remarks information
   * @param signature note sender's signature information
   * @return Return true on success, and trigger revert on failure.
   */
  virtual bool UpdateMetaData(const platon::h256 &note_hash,
                              const platon::bytesConstRef &meta_data,
                              const platon::bytesConstRef &signature) = 0;
  /**
   * @brief Get the token name
   *
   * @return token name
   */
  virtual std::string Name() = 0;

  /**
   * @brief Get the token symbol
   *
   * @return token symbol
   */
  virtual std::string Symbol() = 0;

  /**
   * @brief Get the token conversion ratio
   *
   * @return token conversion ratio
   */
  virtual u128 ScalingFactor() = 0;

  /**
   * @brief Get the total supply of tokens
   *
   * @return Total token supply
   */
  virtual u128 TotalSupply() = 0;

  /**
   * @brief Determine whether the plug-in supports a certain version certificate
   *
   * @param version version number
   * @return Supports returning true, does not support returning false
   */
  virtual bool SupportProof(uint32_t version) = 0;

  /**
   * @brief Get the Approval object.
   *
   * @param  note_hash        output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  virtual platon::bytesConstRef GetApproval(const platon::h256 &note_hash) = 0;

  /**
   * @brief Get the acl contract address
   *
   * @return acl contract address
   */
  virtual platon::Address GetAcl() = 0;

  /**
   * @brief Upgrade validator contract
   *
   * @param version version number
   * @return Successful true, false trigger revert operation
   */
  virtual bool UpdateValidator(uint32_t version) = 0;

  /**
   * @brief Upgrade storage contract
   *
   * @param version version number
   * @return Successful true, false trigger revert operation
   */
  virtual bool UpdateStorage(uint32_t version) = 0;
};

class MintBurnConfidentialTokenInterface {
  /**
   * @brief An operation that generates notes out of nothing
   *
   * @param proof Proof of coinage
   * @return Return true on success, Failure to trigger revert operation
   */
  virtual bool Mint(const platon::bytesConstRef &proof) = 0;

  /**
   * @brief An operation to destroy notes
   *
   * @param proof Proof of destruction
   * @return Return true on success, Failure to trigger revert operation
   */
  virtual bool Burn(const platon::bytesConstRef &proof) = 0;
};
