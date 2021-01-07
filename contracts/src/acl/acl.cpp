

#include <platon/platon.h>
#include <memory>
#include "admin/acl_contract_manager.hpp"
#include "admin/contract_manager.hpp"
#include "platon/call.hpp"
#include "platon/safety_math.hpp"
#include "privacy/acl_interface.h"
#include "privacy/debug/gas/stack_helper.h"
#include "privacy/registry_proxy.hpp"
#include "privacy/storage_admin_proxy.hpp"
#include "privacy/validator_admin_proxy.hpp"
#include "privacy/validator_interface.hpp"
#include "token/arc20_proxy.hpp"
#include "token/token_manager_proxy.hpp"
#include "platon/escape_event.hpp"

class Acl : public ValidatorManager,
            public StorageManager,
            public privacy::AclInterface,
            public Contract {
 public:
  /**
   * @brief ACL contract initialization
   *
   * @param registry_address Register contract address(If the address is 0, it
   * means that the acl contract address does not need to be registered with the
   * registration contract during initialization)
   * @param token_manager token management contract address(If it is 0 address,
   * it is not the address of token manager contract set during initialization)
   */
  ACTION void init(const Address &registry_address,
                   const Address &token_manager) {
    SetAuthority(platon_caller());

    if (registry_address != Address()) {
      RegistryProxy registry(registry_address);
      registry.SetContractAddress(acl_contract_name, platon_address());
      DEBUG("acl identifier:", acl_contract_name,
            "acl address:", platon_address().toString())
      platon_set_state((const byte *)&kRegistryKey, sizeof(kRegistryKey),
                       registry_address.data(), registry_address.size);
    }

    if (token_manager != Address()) {
      auto res = platon::escape::platon_create_contract(
          token_manager, u128(0), ::platon_gas(), platon_address());
      privacy_assert(res.second, "clone token manager fail");

      DEBUG("acl address:", platon_address().toString(),
            "token manager address:", res.first.toString())
      platon_set_state((const byte *)&kTokenManagerKey,
                       sizeof(kTokenManagerKey), res.first.data(),
                       res.first.size);
    }
  }

  /**
   * @brief Get tokenManager contract address
   *
   * @return tokenManager contract address
   */
  CONST Address GetTokenManager() {
    Address addr;
    ::platon_get_state((const byte *)&kTokenManagerKey,
                       sizeof(kTokenManagerKey), addr.data(), addr.size);
    return addr;
  }

  /**
   * @brief Privacy contract registration interface
   *
   *
   * @param validator_version validataor contract version number
   * @param storage_version storage contract version number
   * @param scaling_factor Scaling factor
   * @param token_address token address
   * @param can_mint_burn Whether to support minting and destruction
   * @return Success returns true, failure triggers revert
   */
  ACTION bool CreateRegistry(uint32_t validator_version,
                             uint32_t storage_version, u128 scaling_factor,
                             const Address &token_address,
                             bool can_mint_burn) override {
    Address sender = platon_caller();
    privacy_assert(platon_get_state_length(sender.data(), sender.size) == 0,
                   "had registered");

    TokenRegistry registry =
        TokenRegistry{token_address,   scaling_factor, validator_version,
                      storage_version, can_mint_burn,  0};

    Address validator = ValidatorManager::Deploy(validator_version);
    Address storage = StorageManager::Deploy(storage_version);

    DEBUG("sender", sender.toString(), "registry storage", storage.toString(),
          "validator", validator.toString(), "token", token_address.toString())

    ValidatorAdminProxy vp(sender, validator);
    StorageAdminProxy sp(sender, storage);
    sp.CreateRegistry(can_mint_burn);

    set_state<TokenRegistry, 60>(sender.data(), sender.size, registry);

    PLATON_EMIT_EVENT1(CreateRegistryNote, sender, storage, validator,
                       token_address);
    return true;
  }

  /**
   * @brief Transfer notes to others
   *
   * @param proof Proof of transfer
   * @return The result of the transfer
   */
  ACTION virtual bytesConstRef Transfer(const bytesConstRef &proof) override {
    bytesConstRef outputs = ValidateProof(proof);

    UpdateNotes(outputs);

    return outputs;
  }

  /**
   * @brief Authorize other users to spend notes.
   *
   * @param shared_secret Shared secret,
   * @return Failure to trigger revert operation
   */
  ACTION bytesConstRef Approve(const bytesConstRef &proof) override {
    Address sender = platon_caller();
    TokenRegistry registry;
    get_state(sender.data(), sender.size, registry);
    DEBUG("validate approve");
    ValidatorAdminProxy validator(sender);
    auto outputs = validator.ValidateProof(proof);

    StorageAdminProxy storage(sender);
    storage.Approve(outputs);
    DEBUG("storage approve");
    return outputs;
  }

  /**
   * @brief Get the Approval object.
   *
   * @param  note_hash  output hash
   * @return Return the approve information, if there is no note hash, trigger
   * the revert operation.
   */
  CONST bytesConstRef GetApproval(const h256 &note_hash) override {
    DEBUG("get approval", note_hash.toString());
    Address sender = platon_caller();
    StorageAdminProxy storage(sender);
    return storage.GetApproval(note_hash);
  }

  /**
   * @brief An operation that generates notes out of nothing
   *
   * @param proof Proof of coinage
   * @return The result of minting
   */
  ACTION bytesConstRef Mint(const bytesConstRef &proof) override {
    Address sender = platon_caller();
    TokenRegistry registry;
    get_state(sender.data(), sender.size, registry);

    privacy_assert(registry.can_mint_burn, "this asset is not mintable");

    ValidatorAdminProxy validator(sender);
    auto outputs = validator.ValidateProof(proof);

    MintResult result;
    RLP rlp = RLP(outputs);
    fetch(rlp[0], result.old_mint_hash);
    fetch(rlp[1], result.new_mint_hash);
    fetch(rlp[2], result.total_mint);

    privacy_assert(result.old_mint_hash == registry.last_mint_hash,
                   "mint hash doesn't match");

    registry.last_mint_hash = result.new_mint_hash;
    auto res = SafeAdd(registry.total_supply, result.total_mint);
    privacy_assert(!res.second, "mint exceed limit");
    registry.total_supply = res.first;
    set_state<TokenRegistry, 60>(sender.data(), sender.size, registry);

    StorageAdminProxy storage(sender);
    storage.Mint(outputs);

    return outputs;
  }

  /**
   * @brief An operation to destroy notes
   *
   * @param proof Proof of destruction
   * @return Result of destruction
   */
  ACTION bytesConstRef Burn(const bytesConstRef &proof) override {
    Address sender = platon_caller();
    TokenRegistry registry;
    get_state(sender.data(), sender.size, registry);

    privacy_assert(registry.can_mint_burn, "this asset is not burnable");

    ValidatorAdminProxy validator(sender);
    auto outputs = validator.ValidateProof(proof);

    BurnResult result;
    RLP rlp = RLP(outputs);
    fetch(rlp[0], result.old_burn_hash);
    fetch(rlp[1], result.new_burn_hash);
    fetch(rlp[2], result.total_burn);

    privacy_assert(result.old_burn_hash == registry.last_burn_hash,
                   "burn hash doesn't match");

    registry.last_burn_hash = result.new_burn_hash;
    auto res = SafeSub(registry.total_supply, result.total_burn);
    privacy_assert(!res.second, "burn exceed limit");
    registry.total_supply = res.first;
    set_state<TokenRegistry, 60>(sender.data(), sender.size, registry);

    StorageAdminProxy storage(sender);
    storage.Burn(outputs);

    return outputs;
  }

    /**
   * @brief verification proof interface
   *
   * @param proof proof information
   * @return Verification result, including verification result, inputs,
   * outputs, public_owner, public_value
   */
  CONST bytesConstRef ValidateProof(const bytesConstRef &proof) override {
    Address sender = platon_caller();

    ValidatorAdminProxy validator(sender);
    DEBUG("sender", sender.toString(), "validator",
          validator.GetProxy().toString());

    // ValidatorProxy validator(registry.validator_addr);
    auto result = validator.ValidateProof(proof);
    return result;
  }

  /**
   * @brief Verify the legality of the signature
   *
   * @param note_sender note sender address
   * @param hash data hash
   * @param signature data signature
   * @return Return true on success, false on failure
   */
  CONST bool ValidateSignature(const bytes &note_sender, const h256 &hash,
                               const bytesConstRef &signature) override{
    Address sender = platon_caller();
    TokenRegistry registry;
    get_state(sender.data(), sender.size, registry);

    ValidatorAdminProxy validator(sender);
    return validator.ValidateSignature(note_sender, hash, signature);
  }

  /**
   * @brief Obtain privacy contract registration information
   *
   * @param addr privacy contract address
   * @return Privacy contract registration information
   */
  CONST Registry GetRegistry(const Address &addr) override {
    Registry registry;
    get_state(addr.data(), addr.size,
              *reinterpret_cast<TokenRegistry *>(&registry));

    ValidatorAdminProxy validator(addr);
    registry.validator_addr = validator.GetProxy();
    StorageAdminProxy storage(addr);
    registry.storage_addr = storage.GetProxy();
    return registry;
  }

  /**
   * @brief Obtain privacy contract registration information
   *
   * @param addr privacy contract address
   * @return Privacy contract registration information
   */
  CONST NoteStatus GetNote(const h256 &note_hash) override {
    Address sender = platon_caller();
    TokenRegistry registry;
    get_state(sender.data(), sender.size, registry);
    StorageAdminProxy storage(sender);
    return storage.GetNote(note_hash);
  }

  /**
   * @brief Determine whether the verification contract supports a certain
   * version
   *
   * @param version version number
   * @return Support return true, fail return false
   */
  CONST virtual bool SupportProof(uint32_t version) override {
    Address sender = platon_caller();
    ValidatorAdminProxy validator(sender);
    return validator.SupportProof(version);
  }

  /**
   * @brief Upgrade acl contract
   * 1. Only contract administrators can upgrade contracts.
   * 2. The acl template contract does not need to register the address with the
   * registered contract. The registered contract address passed in when the
   * template contract is initialized is 0, and the template contract does not
   * support migration and upgrade.
   * 3. Only the contract administrator can change the address of the contract
   * in the registered contract, After the upgrade, the registered contract
   * address is 0 when the contract is initialized, After the upgrade is
   * successful, the contract administrator will change the registered acl
   * contract address before the upgrade.
   *
   * @param address acl template contract address.
   * @return After the upgrade is successful, return to the upgraded acl
   * contract address.
   */
  ACTION Address Migrate(const Address &address) override{
    privacy_assert(GetAuthority() == platon_caller(), "no permission");

    Address registry_address;
    int32_t registry_len =
        platon_get_state((const byte *)&kRegistryKey, sizeof(kRegistryKey),
                         registry_address.data(), registry_address.size);
    privacy_assert(registry_len > 0, "invalid registry address");

    bytesConstRef init_rlp =
        platon::escape::cross_call_args_ref("init", Address(), Address());

    bytes value_bytes = value_to_bytes(u128(0));
    bytes gas_bytes = value_to_bytes(::platon_gas());

    Address new_address;
    bool success = ::platon_clone_migrate(
                       address.data(), new_address.data(), init_rlp.data(),
                       init_rlp.size(), value_bytes.data(), value_bytes.size(),
                       gas_bytes.data(), gas_bytes.size()) == 0;

    privacy_assert(success, "migrate failed");
    DEBUG("migrate success, new address:", new_address.toString());
    PLATON_EMIT_EVENT2(AclMigrateEvent, platon_address(), new_address);

    RegistryProxy registry(registry_address);
    registry.SetContractAddress(acl_contract_name, new_address);
    registry.SetManager(acl_contract_name, new_address);
    DEBUG("acl identifier:", acl_contract_name,
          "new acl address:", new_address.toString(),
          "new manager:", new_address.toString());

    Address token_manager_address = GetTokenManager();
    TokenManagerProxy token_manager_proxy(token_manager_address);
    bool set_result = token_manager_proxy.SetOwner(new_address);
    privacy_assert(set_result, "set token manager owner fail");

    return new_address;
  }

 private:
  /**
   * @brief Update transfer data
   *
   * @param outputs Data after transfer transaction verification
   * @return Failure to trigger revert operation
   */
  void UpdateNotes(const bytesConstRef &outputs) {
    Address sender = platon_caller();

    StorageAdminProxy storage(sender);
    storage.UpdateNotes(outputs);
    DEBUG("storage update success");

    TransferResult result;
    RLP rlp = RLP(outputs);
    fetch(rlp[2], result.public_owner);
    fetch(rlp[3], result.public_value);

    if (result.public_value != 0) {
      TokenRegistry registry;
      get_state(sender.data(), sender.size, registry);

      Address token_manager_address = GetTokenManager();

      if (result.public_value > 0) {
        auto res = SafeMul(u128(result.public_value), registry.scaling_factor);
        privacy_assert(!res.second, "transfer value exceed limit");

        TokenManagerProxy token_manager_proxy(token_manager_address);
        bool withdraw_result = token_manager_proxy.Withdraw(
            registry.token_addr, result.public_owner, res.first);
        privacy_assert(withdraw_result, "withdraw fail");

        res = SafeSub(registry.total_supply, u128(result.public_value));
        privacy_assert(!res.second, "transfer exceed limit");

        registry.total_supply = res.first;
        DEBUG("public value:", result.public_value,
              "public owner:", result.public_owner.toString());
      } else if (result.public_value < 0) {
        auto res = SafeMul(u128(-result.public_value), registry.scaling_factor);
        privacy_assert(!res.second, "transfer value exceed limit");

        TokenManagerProxy token_manager_proxy(token_manager_address);
        bool deposit_result = token_manager_proxy.Deposit(
            registry.token_addr, result.public_owner, res.first);
        privacy_assert(deposit_result, "deposit fail");

        res = SafeAdd(registry.total_supply, u128(-result.public_value));
        privacy_assert(!res.second, "transfer from exceed limit");

        registry.total_supply = res.first;
        DEBUG("public value:", "owner:", result.public_owner.toString(),
              "origin:", platon_origin().toString());
      }
      set_state<TokenRegistry, 60>(sender.data(), sender.size, registry);
    }
  }

 private:
  PLATON_EVENT1(CreateRegistryNote, const Address &, const Address &,
                const Address &, const Address &);

  PLATON_EVENT2(AclMigrateEvent, const Address &, const Address &);

 private:
  const uint64_t kRegistryKey = uint64_t(Name::Raw("registry"_n));
  const uint64_t kTokenManagerKey = uint64_t(Name::Raw("tokenManager"_n));
};

PLATON_DISPATCH(
    Acl, (init)(GetTokenManager)(CreateRegistry)(ValidateProof)(Approve)(
             GetApproval)(Mint)(Burn)(GetRegistry)(Transfer)(GetNote)(
             ValidateSignature)(SupportProof)(Migrate)(CreateValidator)(
             UpdateValidatorVersion)(ValidatorLatest)(ValidatorLatestMinor)(
             UpdateValidator)(CreateStorage)(UpdateStorageVersion)(
             StorageLatest)(StorageLatestMinor)(UpdateStorage))
