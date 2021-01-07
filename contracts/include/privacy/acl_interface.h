
#pragma once

#include <platon/platon.h>
#include "privacy/validator_interface.hpp"
namespace privacy {

/**
 * @brief  Privacy token contract registration information.
 */
struct TokenRegistry {
  platon::Address token_addr;
  platon::u128 scaling_factor;
  uint32_t validator_version;
  uint32_t storage_version;
  bool can_mint_burn;
  platon::u128 total_supply;
  platon::h256 last_mint_hash;
  platon::h256 last_burn_hash;
  PLATON_SERIALIZE(
      TokenRegistry,
      (token_addr)(scaling_factor)(validator_version)(storage_version)(
          can_mint_burn)(total_supply)(last_mint_hash)(last_burn_hash))
};

struct Registry : public TokenRegistry {
  platon::Address storage_addr;
  platon::Address validator_addr;
  PLATON_SERIALIZE_DERIVED(Registry, TokenRegistry,
                           (storage_addr)(validator_addr))
};

class ValidatorManagerInterface {
  /**
   * @brief Create a new type of validator template contract (first time
   * creation)
   *
   * @param version version number
   * @param addr Template contract address
   * @param description Template contract description information
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool CreateValidator(uint32_t version, const platon::Address& addr,
                               const std::string& description) = 0;

  /**
   * @brief Upgrade existing types of validator template contract versions
   *
   * @param version version number
   * @param addr New version template contract address
   * @param description  Template contract description information
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool UpdateValidatorVersion(uint32_t version,
                                      const platon::Address& addr,
                                      const std::string& description) = 0;

  /**
   * @brief Get the highest version number of the validator template contract
   *
   * @param algorithm_name Algorithm type name
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t ValidatorLatest(uint8_t algorithm_name) = 0;

  /**
   * @brief Get the latest minor version number of a major version of the
   * validator template contract
   *
   * @param algorithm_name Contract type name
   * @param major_version  Major version number
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t ValidatorLatestMinor(uint8_t algorithm_name,
                                        uint8_t major_version) = 0;

  /**
   * @brief  Users upgrade their contracts
   *
   * @param version version number
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool UpdateValidator(uint32_t version) = 0;
};

class StorageManagerInterface {
  /**
   * @brief Create a new type of storage template contract (first time creation)
   *
   * @param version version number
   * @param addr Template contract address
   * @param description Template contract description information
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool CreateStorage(uint32_t version, const platon::Address& addr,
                             const std::string& description) = 0;

  /**
   * @brief Upgrade existing types of storage template contract versions
   *
   * @param version version number
   * @param addr New version template contract address
   * @param description  Template contract description information
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool UpdateStorageVersion(uint32_t version,
                                    const platon::Address& addr,
                                    const std::string& description) = 0;

  /**
   * @brief Get the highest version number of the storage template contract
   *
   * @param algorithm_name Algorithm type name
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t StorageLatest(uint8_t algorithm_name) = 0;

  /**
   * @brief Get the latest minor version number of a major version of the
   * storage template contract
   *
   * @param algorithm_name Contract type name
   * @param major_version  Major version number
   * @return The specific version information is returned successfully, 0 is
   * returned on failure
   */
  virtual uint32_t StorageLatestMinor(uint8_t algorithm_name,
                                      uint8_t major_version) = 0;

  /**
   * @brief  Users upgrade their contracts
   *
   * @param version version number
   * @return Return true on success, false on failure and trigger the revert
   * operation
   */
  virtual bool UpdateStorage(uint32_t version) = 0;
};

class AclInterface {
 public:
  /**
   * @brief Privacy contract registration interface
   *
   *
   * @param validator_version validataor contract version number
   * @param scaling_factor Scaling factor
   * @param storage_version storage contract version number
   * @param token_address token address
   * @param can_mint_burn Whether to support minting and destruction
   * @return Success returns true, failure triggers revert
   */
  virtual bool CreateRegistry(uint32_t validator_version,
                              uint32_t storage_version, u128 scaling_factor,
                              const platon::Address& token_address,
                              bool can_mint_burn) = 0;

  /**
   * @brief Transfer notes to others
   *
   * @param proof Proof of transfer
   * @return The result of the transfer
   */
  virtual platon::bytesConstRef Transfer(
      const platon::bytesConstRef& proof) = 0;

  /**
   * @brief Authorize other users to spend notes
   *
   * @param outputs Verification proof output
   * @return Failure to trigger revert operation
   */
  virtual platon::bytesConstRef Approve(
      const platon::bytesConstRef& outputs) = 0;

  /**
   * @brief Get the Approval object.
   *
   * @param  note_hash  output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  virtual bytesConstRef GetApproval(const h256 &note_hash) = 0;

  /**
   * @brief An operation that generates notes out of nothing
   *
   * @param proof Proof of coinage
   * @return The result of minting
   */
  virtual platon::bytesConstRef Mint(const platon::bytesConstRef& outputs) = 0;

  /**
   * @brief An operation to destroy notes
   *
   * @param proof Proof of destruction
   * @return Result of destruction
   */
  virtual platon::bytesConstRef Burn(const platon::bytesConstRef& outputs) = 0;

  /**
   * @brief Obtain privacy contract registration information
   *
   * @param addr privacy contract address
   * @return Privacy contract registration information
   */
  virtual Registry GetRegistry(const platon::Address& addr) = 0;

  /**
   * @brief Obtain privacy contract registration information
   *
   * @param addr privacy contract address
   * @return Privacy contract registration information
   */
  virtual NoteStatus GetNote(const platon::h256& note_hash) = 0;

  /**
   * @brief verification proof interface
   *
   * @param proof proof information
   * @return Verification result, including verification result, inputs,
   * outputs, public_owner, public_value
   */
  virtual platon::bytesConstRef ValidateProof(const platon::bytesConstRef& outputs) = 0;

  /**
   * @brief Verify the legality of the signature
   *
   * @param note_sender note sender address
   * @param hash data hash
   * @param signature data signature
   * @return Return true on success, false on failure
   */
  virtual bool ValidateSignature(const bytes &note_sender, const h256 &hash, const bytesConstRef &signature) = 0;
  /**
   * @brief Determine whether the verification contract supports a certain
   * version
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
  virtual Address Migrate(const Address &address) = 0;
};

const std::string acl_contract_name = "acl";
}  // namespace privacy
