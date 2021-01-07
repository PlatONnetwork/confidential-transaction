#pragma once

#include "admin/contract_manager.hpp"
#include "platon/platon.h"
#include "privacy/acl_interface.h"
#include "privacy/debug/gas/stack_helper.h"

class ContractAuthority {
 public:
  void SetAuthority(const Address &authority) {
    ::platon_set_state((const byte *)&kAuthorityKey, sizeof(kAuthorityKey),
                       (const byte *)authority.data(), authority.size);
  }

  Address GetAuthority() {
    Address addr;
    ::platon_get_state((const byte *)&kAuthorityKey, sizeof(kAuthorityKey),
                       addr.data(), addr.size);
    return addr;
  }

 private:
  const uint64_t kAuthorityKey = name_value("authority");
};

#define CREATE_UPDATE_CONTRACT(CONTRACT)                                       \
  template <typename Proxy, typename Interface, Name::Raw ManagerName>         \
  class CreateUpdate##CONTRACT : public Interface,                             \
                                 public virtual ContractAuthority {            \
   public:                                                                     \
    /**                                                                        \
     * @brief Create a new type of template contract (first time creation)     \
     *                                                                         \
     * @param version version number                                           \
     * @param addr Template contract address                                   \
     * @param description Template contract description information            \
     * @return Return true on success, false on failure and trigger the revert \
     * operation                                                               \
     */                                                                        \
    ACTION virtual bool Create##CONTRACT(                                      \
        uint32_t version, const Address &addr,                                 \
        const std::string &description) override {                             \
      privacy_assert(GetAuthority() == platon_caller(), "no permission");      \
      return manager_.Create(version, addr, description);                      \
    }                                                                          \
                                                                               \
    /**                                                                        \
     * @brief Upgrade existing types of template contract versions             \
     *                                                                         \
     * @param version version number                                           \
     * @param addr New version template contract address                       \
     * @param description  Template contract description information           \
     * @return Return true on success, false on failure and trigger the revert \
     * operation                                                               \
     */                                                                        \
    ACTION virtual bool Update##CONTRACT##Version(                             \
        uint32_t version, const Address &addr,                                 \
        const std::string &description) override {                             \
      privacy_assert(GetAuthority() == platon_caller(), "no permission");      \
      return manager_.Update(version, addr, description);                      \
    }                                                                          \
                                                                               \
    /**                                                                        \
     * @brief Get the highest version number of the template contract          \
     *                                                                         \
     * @param algorithm_name Algorithm type name                               \
     * @return The specific version information is returned successfully, 0 is \
     * returned on failure                                                     \
     */                                                                        \
    CONST virtual uint32_t CONTRACT##Latest(uint8_t algorithm_name) override { \
      return manager_.Latest(algorithm_name);                                  \
    }                                                                          \
                                                                               \
    /**                                                                        \
     * @brief Get the latest minor version number of a major version of the    \
     * contract                                                                \
     *                                                                         \
     * @param algorithm_name Contract type name                                \
     * @param major_version  Major version number                              \
     * @return The specific version information is returned successfully, 0 is \
     * returned on failure                                                     \
     */                                                                        \
    CONST virtual uint32_t CONTRACT##LatestMinor(                              \
        uint8_t algorithm_name, uint8_t major_version) override {              \
      return manager_.LatestMinor(algorithm_name, major_version);              \
    }                                                                          \
                                                                               \
    /**                                                                        \
     * @brief Users upgrade their contracts                                    \
     *                                                                         \
     * @param version version number                                           \
     * @return Return true on success, false on failure and trigger the revert \
     * operation                                                               \
     */                                                                        \
    ACTION virtual bool Update##CONTRACT(uint32_t version) override {          \
      Address sender = platon_caller();                                        \
      TokenRegistry registry;                                                  \
      get_state(sender.data(), sender.size, registry);                         \
                                                                               \
      Address version_address = manager_.VersionAddress(version);              \
      Proxy proxy(sender);                                                     \
      DEBUG("old version address", proxy.GetProxy().toString(),                \
            "new version address", version_address.toString());                \
      Address new_address = proxy.Migrate(version_address);                    \
      privacy_assert(new_address != Address(0), "migrate failed");             \
                                                                               \
      proxy.UpdateProxy(sender, new_address);                                  \
      registry.storage_version = version;                                      \
      set_state<TokenRegistry, 60>(sender.data(), sender.size, registry);      \
                                                                               \
      DEBUG(#CONTRACT, new_address.toString());                                \
                                                                               \
      return true;                                                             \
    }                                                                          \
                                                                               \
   protected:                                                                  \
    /**                                                                        \
     * @brief Deploy a certain version of the contract                         \
     *                                                                         \
     * @param version version number                                           \
     * @return Return true on success, false on failure and trigger the revert \
     * operation                                                               \
     */                                                                        \
    platon::Address Deploy(uint32_t version) {                                 \
      return manager_.Deploy(version);                                         \
    }                                                                          \
                                                                               \
   private:                                                                    \
    DefaultContractManager<ManagerName> manager_;                              \
  };

// Define verification contract management class
CREATE_UPDATE_CONTRACT(Validator)

// Define storage contract management class
CREATE_UPDATE_CONTRACT(Storage)

using ValidatorManager =
    CreateUpdateValidator<ValidatorAdminProxy, ValidatorManagerInterface,
                          "validator_manager"_n>;
using StorageManager =
    CreateUpdateStorage<StorageAdminProxy, StorageManagerInterface,
                        "storage_manager"_n>;