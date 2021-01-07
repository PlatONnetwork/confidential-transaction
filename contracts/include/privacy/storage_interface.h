#pragma once

#include <platon/platon.h>
#include <privacy/utxo.h>
namespace privacy {
class StorageInterface {
  /**
   * @brief Create a Registry object
   * @param  can_mint_burn  Whether to allow minting and destruction operations
   */
  virtual void CreateRegistry(bool can_mint_burn) = 0;

  /**
   * @brief Query Note information
   *
   * @param hash hash value of note
   * @return There is a return NoteStatus, there is no trigger revert operation
   */
  virtual NoteStatus GetNote(const platon::h256 &note_hash) = 0;

  /**
   * @brief Store approve information.
   * @param  outputs  approve information
   * @return ACTION
   */
  virtual void Approve(const platon::bytesConstRef &outputs) = 0;

  /**
   * @brief Get the Approval object.
   * @param  note_hash        output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  virtual platon::bytesConstRef GetApproval(const platon::h256 &note_hash) = 0;

  /**
   * @brief update storage
   *
   * @param proof_result Transfer result information
   */
  virtual void UpdateNotes(const platon::bytesConstRef &outputs) = 0;

  /**
   * @brief Store coin information
   * @param  outputs  Serialized byte stream of coin information
   * @return ACTION
   */
  virtual void Mint(const platon::bytesConstRef &outputs) = 0;

  /**
   * @brief Destroy tokens
   * @param  outputs   Destroy the information byte stream
   * @return ACTION
   */
  virtual void Burn(const platon::bytesConstRef &outputs) = 0;

  /**
   * @brief Migration upgrade
   *
   * @param address Template contract address
   * @return Contract address after upgrade
   */
  virtual Address Migrate(const Address &address) = 0;
};

}  // namespace privacy