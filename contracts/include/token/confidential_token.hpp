#pragma once
#include <platon/platon.h>

#include <string>
#include "privacy/acl_proxy.hpp"
#include "privacy/debug/gas/stack_helper.h"
#include "privacy/validator_interface.hpp"
#include "token/confidential_token_interface.h"
#include "platon/escape_event.hpp"

namespace privacy {

class BaseConfidentialToken : public ConfidentialTokenInterface {
 public:
  /**
   * @brief Token deployment initialization interface
   * @param  name             token name
   * @param  symbol           token symbol
   * @param  registry_address Registered contract address
   * @param  validator_version validator contarct version
   * @param  storage_version  storage contract version
   * @param  scaling_factor   Scaling factor
   * @param  token_address    arc20 token contract address
   */
  virtual void init(const std::string &name, const std::string &symbol,
                    const Address &registry_address, uint32_t validator_version,
                    uint32_t storage_version, u128 scaling_factor,
                    const Address &token_address) {
    privacy_assert(!name.empty(), "token name is empty");
    privacy_assert(!symbol.empty(), "token symbol is empty");
    SetName(name);
    SetSymbol(symbol);
    SetRegistry(registry_address);
    SetOwner(platon_caller());

    RegistryProxy registry(registry_address);
    Address acl = registry.GetContractAddress(acl_contract_name);

    DEBUG("acl address:", acl.toString());

    privacy_assert(acl != Address(), "invalid acl address");

    AclProxy ap(acl);
    ap.CreateRegistry(validator_version, storage_version, scaling_factor,
                      token_address, true);
  }

  /**
   * @brief Transfer notes to others
   *
   * @param proof Proof of transfer
   * @return Return true on success, Failure to trigger revert operation
   */
  ACTION bool Transfer(const bytesConstRef &proof) {
    Address acl = GetAcl();
    AclProxy ap(acl);
    DEBUG("acl:", acl.toString());
    auto result = ap.Transfer(proof);

    DEBUG("acl transfer success");

    RLP rlp(result);
    InputNotes inputs;
    OutputNotes outputs;
    fetch(rlp[0], inputs);
    fetch(rlp[1], outputs);

    DEBUG("validator success");

    EmitCreateNotes(outputs);
    EmitDestroyNotes(inputs);
    return true;
  }

  /**
   * @brief Authorize other users to spend notes.
   *
   * @param shared_secret Shared secret,
   * @return Failure to trigger revert operation
   */
  ACTION bool Approve(const bytesConstRef &shared_secret) {
    DEBUG("approve");
    Address acl = GetAcl();
    AclProxy ap(acl);
    auto outputs = ap.Approve(shared_secret);
    ApproveResult result;
    fetch(RLP(outputs), result);
    PLATON_EMIT_EVENT1(ApproveEvent, result.note_hash, result.shared_sign);
    return true;
  }

  /**
   * @brief Get the Approval object.
   *
   * @param note_hash  output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  CONST bytesConstRef GetApproval(const h256 &note_hash) {
    Address acl = GetAcl();
    AclProxy ap(acl);
    return ap.GetApproval(note_hash);
  }

  /**
   * @brief Get the acl contract address
   *
   * @return acl contract address
   */
  CONST Address GetAcl() {
    Address registry_address;
    platon_get_state((const byte *)&kRegistryKey, sizeof(kRegistryKey),
                     registry_address.data(), registry_address.size);

    RegistryProxy registry(registry_address);
    Address acl = registry.GetContractAddress(acl_contract_name);

    return acl;
  }

  /**
   * @brief Update note remarks information
   *
   * @param note_hash note hash
   * @param meta_data remarks information
   * @param signature note sender's signature information
   * @return Return true on success, and trigger revert on failure.
   */
  ACTION bool UpdateMetaData(const h256 &note_hash,
                             const bytesConstRef &meta_data,
                             const bytesConstRef &signature) {
    Address acl = GetAcl();
    AclProxy ap(acl);
    NoteStatus status = ap.GetNote(note_hash);
    h256 hash;
    ::platon_sha3(meta_data.data(), meta_data.size(), hash.data(), hash.size);

    DEBUG("note_hash", note_hash.toString(), "sender",
          Address(status.sender).toString(), "meta_data", toHex(meta_data),
          "signature", toHex(signature));

    privacy_assert(ap.ValidateSignature(bytesConstRef(status.sender.data(),
                                                      status.sender.size()),
                                        hash, signature),
                   "validate signature failed");
    PLATON_EMIT_EVENT1(MetaDataEvent, note_hash, meta_data);
    return true;
  }

  /**
   * @brief Get the token name
   *
   * @return token name
   */
  CONST std::string Name() {
    size_t len =
        ::platon_get_state_length((const uint8_t *)&kNameKey, sizeof(kNameKey));
    privacy_assert(len != 0, "PlatON ARC20: get token name failed");
    std::string name;
    name.resize(len);
    ::platon_get_state((const uint8_t *)&kNameKey, sizeof(kNameKey),
                       (byte *)name.data(), name.length());
    DEBUG("name:", name);
    return name;
  }

  /**
   * @brief Get the token symbol
   *
   * @return token symbol
   */
  CONST std::string Symbol() {
    size_t len = ::platon_get_state_length((const uint8_t *)&kSymbolKey,
                                           sizeof(kSymbolKey));
    privacy_assert(len != 0, "PlatON ARC20: get token symbol failed");

    std::string symbol;
    symbol.resize(len);
    ::platon_get_state((const uint8_t *)&kSymbolKey, sizeof(kSymbolKey),
                       (byte *)symbol.data(), symbol.length());
    DEBUG("symbol:", symbol);
    return symbol;
  }

  /**
   * @brief Get the token conversion ratio
   *
   * @return token conversion ratio
   */
  CONST u128 ScalingFactor() {
    Address acl = GetAcl();
    AclProxy ap(acl);
    Registry registry = ap.GetRegistry(platon_address());
    return registry.scaling_factor;
  }

  /**
   * @brief Get the total supply of tokens
   *
   * @return Total token supply
   */
  CONST u128 TotalSupply() {
    Address acl = GetAcl();
    AclProxy ap(acl);
    Registry registry = ap.GetRegistry(platon_address());
    return registry.total_supply;
  }

  /**
   * @brief Upgrade validator contract
   *
   * @param version version number
   * @return Successful true, false trigger revert operation
   */
  ACTION bool UpdateValidator(uint32_t version) {
    privacy_assert(GetOwner() == platon_caller(), "illegal update owner");
    Address acl = GetAcl();
    AclProxy ap(acl);
    return ap.UpdateValidator(version);
  }

  /**
   * @brief Upgrade storage contract
   *
   * @param version version number
   * @return Successful true, false trigger revert operation
   */
  ACTION bool UpdateStorage(uint32_t version) {
    privacy_assert(GetOwner() == platon_caller(), "illegal update owner");
    Address acl = GetAcl();
    AclProxy ap(acl);
    return ap.UpdateStorage(version);
  }

  /**
   * @brief Determine whether the plug-in supports a certain version certificate
   *
   * @param version version number
   * @return Supports returning true, does not support returning false
   */
  CONST bool SupportProof(uint32_t version) {
    Address acl = GetAcl();
    AclProxy ap(acl);
    return ap.SupportProof(version);
  }

 protected:
  void SetName(const std::string &name) {
    ::platon_set_state((const uint8_t *)&kNameKey, sizeof(kNameKey),
                       (const byte *)name.data(), name.length());
  }

  void SetSymbol(const std::string &symbol) {
    ::platon_set_state((const uint8_t *)&kSymbolKey, sizeof(kSymbolKey),
                       (const byte *)symbol.data(), symbol.length());
  }
  void SetRegistry(const Address &addr) {
    platon_set_state((const byte *)&kRegistryKey, sizeof(kRegistryKey),
                     addr.data(), addr.size);
  }

  void SetOwner(const Address &owner) {
    ::platon_set_state((const byte *)&kOwnerKey, sizeof(kOwnerKey),
                       (const byte *)owner.data(), owner.size);
  }

  Address GetOwner() {
    Address addr;
    ::platon_get_state((const byte *)&kOwnerKey, sizeof(kOwnerKey), addr.data(),
                       addr.size);
    return addr;
  }

 protected:
  void EmitCreateNotes(const OutputNotes &notes) {
    for (const OutputNote &note : notes) {
      PLATON_EMIT_EVENT2(CreateNoteEvent, note.owner.ToBytesConstRef(),
                         note.hash, note.owner.ToBytesConstRef());
      if (!note.meta_data.empty()) {
        PLATON_EMIT_EVENT1(MetaDataEvent, note.hash,
                           note.meta_data.ToBytesConstRef());
      }
    }
  }

  void EmitDestroyNotes(const InputNotes &notes) {
    for (const InputNote &note : notes) {
      PLATON_EMIT_EVENT2(DestroyNoteEvent, note.owner.ToBytesConstRef(),
                         note.hash, note.owner.ToBytesConstRef());
    }
  }

  PLATON_EVENT1(ApproveEvent, const h256 &, const bytesConstRef &);
  PLATON_EVENT2(CreateNoteEvent, const bytesConstRef &, const h256 &,
                const bytesConstRef &);
  PLATON_EVENT2(DestroyNoteEvent, const bytesConstRef &, const h256 &,
                const bytesConstRef &);
  PLATON_EVENT1(MetaDataEvent, const h256 &, const bytesConstRef &);

 private:
  const uint64_t kRegistryKey = uint64_t(Name::Raw("registry"_n));
  const uint64_t kOwnerKey = uint64_t(Name::Raw("owner"_n));
  const uint64_t kNameKey = uint64_t(Name::Raw("name"_n));
  const uint64_t kSymbolKey = uint64_t(Name::Raw("symbol"_n));
};

class MintBurnConfidentialToken : public BaseConfidentialToken,
                                  public MintBurnConfidentialTokenInterface {
 public:
  /**
   * @brief An operation that generates notes out of nothing
   *
   * @param proof Proof of coinage
   * @return Success returns true, failure triggers revert
   */
  ACTION bool Mint(const bytesConstRef &proof) {
    DEBUG("owner", GetOwner().toString(), "caller", platon_caller().toString());
    privacy_assert(GetOwner() == platon_caller(), "illegal minter owner");

    Address acl = GetAcl();
    AclProxy ap(acl);
    auto outputs = ap.Mint(proof);

    MintResult result;
    fetch(RLP(outputs), result);
    PLATON_EMIT_EVENT0(MintEvent, result.new_mint_hash, result.total_mint);
    EmitCreateNotes(result.outputs);
    return true;
  }

  /**
   * @brief An operation to destroy notes
   *
   * @param proof Proof of destruction
   * @return Success returns true, failure triggers revert
   */
  ACTION bool Burn(const bytesConstRef &proof) {
    privacy_assert(GetOwner() == platon_caller(), "illegal burner owner");

    Address acl = GetAcl();
    AclProxy ap(acl);
    auto outputs = ap.Burn(proof);

    BurnResult result;
    fetch(RLP(outputs), result);
    PLATON_EMIT_EVENT0(BurnEvent, result.new_burn_hash, result.total_burn);
    EmitDestroyNotes(result.inputs);
    return true;
  }

 private:
  PLATON_EVENT0(MintEvent, const h256 &, u128);
  PLATON_EVENT0(BurnEvent, const h256 &, u128);
};

}  // namespace privacy
