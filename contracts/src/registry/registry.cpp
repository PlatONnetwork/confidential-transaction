#include <platon/platon.h>
#include "privacy/common.hpp"

class Registry : public Contract {
 private:
  // Contract identifier, Contract address
  PLATON_EVENT1(SetContractAddressEvent, const std::string &,
                const platon::Address &);

  // Contract identifier, Contract manager
  PLATON_EVENT1(SetContractManagerEvent, const std::string &,
                const platon::Address &);

 public:
  ACTION void init() {}

 public:
  /**
   * @brief Set the address of a certain contract.
   * 1. The address of the contract has not been set, the address must be set by
   * the contract itself, and the contract administrator is initialized as the
   * contract itself.
   * 2. The address of the contract has already been set and needs to be set by
   * the administrator. (The administrator of each contract is its own until a
   * new address is set).
   *
   * @param contract_identifier Contract identifier (string of contract name,
   * length cannot be greater than 16 bytes)
   * @param addr Contract address
   * @return Success or failure of setting contract address
   */
  ACTION bool SetContractAddress(const std::string &contract_identifier,
                                 const platon::Address &addr) {
    privacy_assert(contract_identifier.size() <= 16,
                   "Too long contract name string");
    DEBUG("platon caller:", platon_caller().toString(),
          "contract identifier:", contract_identifier,
          "contract address:", addr.toString())

    platon::bytes address_key = AddressKey(contract_identifier);
    platon::bytes manager_key = ManagerKey(contract_identifier);
    size_t len =
        ::platon_get_state_length(address_key.data(), address_key.size());
    if (0 == len) {
      privacy_assert(
          platon_caller() == addr,
          "The registered contract address can only be the contract itself");
      ::platon_set_state(address_key.data(), address_key.size(), addr.data(),
                         platon::Address::size);
      PLATON_EMIT_EVENT1(SetContractManagerEvent, contract_identifier, addr);

      ::platon_set_state(manager_key.data(), manager_key.size(), addr.data(),
                         platon::Address::size);
      PLATON_EMIT_EVENT1(SetContractManagerEvent, contract_identifier, addr);
      DEBUG("Initially set the contract address successfully");
      return true;
    }

    privacy_assert(0 != ::platon_contract_code_length(addr.data()),
                   "Invalid contract address");
    platon::Address manager;
    ::platon_get_state(manager_key.data(), manager_key.size(), manager.data(),
                       platon::Address::size);
    privacy_assert(platon_caller() == manager, "Not the manager");

    ::platon_set_state(address_key.data(), address_key.size(), addr.data(),
                       platon::Address::size);
    PLATON_EMIT_EVENT1(SetContractAddressEvent, contract_identifier, addr);
    DEBUG("Change the contract address successfully");
    return true;
  }

  /**
   * @brief Query the address of a contract
   *
   * @param contract_identifier Contract identifier (string of contract name,
   * length cannot be greater than 16 bytes)
   * @return Return the address of the contract, no address 0 is returned
   */
  CONST platon::Address GetContractAddress(
      const std::string &contract_identifier) {
    privacy_assert(contract_identifier.size() <= 16,
                   "Too long contract name string");
    platon::Address result;
    platon::bytes key = AddressKey(contract_identifier);
    ::platon_get_state(key.data(), key.size(), result.data(),
                       platon::Address::size);
    DEBUG("contract address:", result.toString());
    return result;
  }

  /**
   * @brief Set a new manager address new_manager for the contract_identifier
   * contract, the new manager can call setContractAddress to set the new
   * contract address
   *
   * @param contract_identifier Contract identifier (string of contract name,
   * length cannot be greater than 16 bytes)
   * @param new_manager new manager address
   * @return Success or failure of setting the administrator address
   */
  ACTION bool SetManager(const std::string &contract_identifier,
                         const platon::Address &new_manager) {
    privacy_assert(contract_identifier.size() <= 16,
                   "Too long contract name string");
    platon::Address manager;
    platon::bytes key = ManagerKey(contract_identifier);
    ::platon_get_state(key.data(), key.size(), manager.data(),
                       platon::Address::size);
    privacy_assert(platon_caller() == manager, "Not the manager");

    ::platon_set_state(key.data(), key.size(), new_manager.data(),
                       platon::Address::size);
    PLATON_EMIT_EVENT1(SetContractManagerEvent, contract_identifier,
                       new_manager);
    DEBUG("contract identifier:", contract_identifier, "old manager",
          manager.toString(), "new manager:", new_manager.toString());
    return true;
  }

  /**
   * @brief Get contract_identifier contract manager
   *
   * @param contract_identifier Contract identifier (string of contract name,
   * length cannot be greater than 16 bytes)
   * @return Contract manager address
   */
  CONST platon::Address GetManager(const std::string &contract_identifier) {
    privacy_assert(contract_identifier.size() <= 16,
                   "Too long contract name string");
    platon::Address manager;
    platon::bytes key = ManagerKey(contract_identifier);
    ::platon_get_state(key.data(), key.size(), manager.data(),
                       platon::Address::size);
    return manager;
  }

 private:
  platon::bytes AddressKey(const std::string &contract_identifier) {
    platon::bytes result(
        (platon::byte *)&kAddressPostfix,
        (platon::byte *)&kAddressPostfix + sizeof(kAddressPostfix));
    std::copy(contract_identifier.cbegin(), contract_identifier.cend(),
              std::back_inserter(result));
    return result;
  }

  platon::bytes ManagerKey(const std::string &contract_identifier) {
    platon::bytes result(
        (platon::byte *)&kManagerPostfix,
        (platon::byte *)&kManagerPostfix + sizeof(kManagerPostfix));
    std::copy(contract_identifier.cbegin(), contract_identifier.cend(),
              std::back_inserter(result));
    return result;
  }

  const uint64_t kAddressPostfix = name_value("address");
  const uint64_t kManagerPostfix = name_value("manager");
};

PLATON_DISPATCH(Registry, (init)(SetContractAddress)(GetContractAddress)(
                              SetManager)(GetManager))