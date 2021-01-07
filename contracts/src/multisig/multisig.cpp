#include <platon/platon.h>
#include "multisig/actuator.hpp"
#include "privacy/common.hpp"
#include "privacy/debug/gas/stack_helper.h"

class Multisig : public multisig::MultisigEvent, public Contract {
 public:
  /**
   * @brief Multi-signature contract initialization
   *
   * @param owners Committee members
   * @param required Proposal effective number threshold
   * @return Failure to trigger revert operation
   */
  ACTION void init(const std::vector<Address> &owners, uint16_t required) {
    privacy_assert(owners.size() > 0, "invalid owners");
    privacy_assert(required > 0 && owners.size() >= required,
                   "invalid required");

    privacy::set_state((const uint8_t *)&multisig::KOwners,
                       sizeof(multisig::KOwners), owners);
    platon_set_state((const uint8_t *)&multisig::KRequired,
                     sizeof(multisig::KRequired), (const uint8_t *)&required,
                     sizeof(required));

    PLATON_EMIT_EVENT1(CreateMultisig, platon_address(), owners, required);
  }

  /**
   * @brief Change the threshold for the number of members of the committee or
   * the number of effective proposals. Within the timeout period, the number of
   * signatures of the old committee will reach the threshold before the change
   * takes effect.
   *
   * @param owners New committee members
   * @param required Proposal effective number threshold
   * @param second_limit overtime time
   * @return Failure to trigger revert operation
   */
  ACTION void ChangeOwnersAndRequired(const std::vector<Address> &owners,
                                      uint16_t required,
                                      uint32_t second_limit) {
    LocalTransaction(second_limit);
  }

  CONST std::vector<Address> GetOwners() {
    std::vector<Address> owners;
    privacy::get_state((const uint8_t *)&multisig::KOwners,
                       sizeof(multisig::KOwners), owners);
    return owners;
  }

  CONST uint16_t GetRequired() {
    uint16_t required = 0;
    platon_get_state((const uint8_t *)&multisig::KRequired,
                     sizeof(multisig::KRequired), (uint8_t *)&required,
                     sizeof(required));
    return required;
  }

  ACTION void CreateContract(const bytes &args, uint32_t second_limit) {
    LocalTransaction(second_limit);
  }

  /**
   * @brief Clone create contract
   *
   * @param address The address of the template contract
   * @param args Contract initialization parameters
   * RLP([[fnvHash(funcName),args1, args2, ...])
   * @param second_limit overtime time
   * @return Failure, trigger revert operation
   */
  ACTION void CloneContract(const Address address, const bytes &args,
                            uint32_t second_limit) {
    LocalTransaction(second_limit);
  }

  /**
   * @brief Submit a proposal transaction.
   * Within the timeout period, the transaction will only take effect until the
   * number of signatures of the committee reaches the threshold, and the
   * transaction content will be executed.
   *
   * @param to Contract address for executing the transaction (multi-signature
   * contract address or other contract address for cross-contract calls)
   * @param paras The serialized value of the contract call parameters,
   * RLP([[fnvHash(funcName),args1, args2, ...])
   * @param second_limit overtime time
   * @return Failure, trigger revert operation
   */
  ACTION void PushTransaction(const Address &to, bytes paras,
                              uint32_t second_limit) {
    auto functor = [](const multisig::Transaction &one, uint32_t id) {};
    ClearTimeout(functor);

    privacy_assert(second_limit > 0, "invalid second limit");

    uint32_t transaction_id = 0;
    platon_get_state((const uint8_t *)&multisig::KTransactionId,
                     sizeof(multisig::KTransactionId),
                     (uint8_t *)&transaction_id, sizeof(transaction_id));
    transaction_id++;

    Address sender = platon_caller();
    multisig::Transaction one_transaction{
        sender, to, paras, ::platon_timestamp(), second_limit, {}};
    PLATON_EMIT_EVENT2(NewMultisigTransaction, platon_address(), transaction_id,
                       sender, to, paras, second_limit);

    std::vector<Address> owners;
    privacy::get_state((const uint8_t *)&multisig::KOwners,
                       sizeof(multisig::KOwners), owners);
    bool has_execute = false;
    if (owners.end() != std::find(owners.begin(), owners.end(), sender)) {
      one_transaction.signedAddress_.push_back(sender);
      PLATON_EMIT_EVENT2(SignMultisigTransaction, platon_address(),
                         transaction_id, sender);

      uint16_t required = 0;
      platon_get_state((const uint8_t *)&multisig::KRequired,
                       sizeof(multisig::KRequired), (uint8_t *)&required,
                       sizeof(required));
      if (1 == required) {
        auto actuator = multisig::CreateActuator(to, transaction_id);
        bool success = actuator->Execute(paras);

        privacy_assert(success, "multisig transaction execute failed");
        PLATON_EMIT_EVENT2(MultisigTransactionExecute, platon_address(),
                           transaction_id, one_transaction.signedAddress_);
        has_execute = true;
      }
    }

    if (!has_execute) {
      set_state(transaction_id, one_transaction);
      platon_set_state((const uint8_t *)&multisig::KTransactionId,
                       sizeof(multisig::KTransactionId),
                       (const uint8_t *)&transaction_id,
                       sizeof(transaction_id));
    }
  }

  /**
   * @brief Obtain pending transactions that have not reached the limit time and
   * threshold
   *
   * @return Unsigned transaction details
   */
  CONST std::vector<multisig::Transaction> GetPendingTransactions() {
    std::vector<multisig::Transaction> result;
    auto functor = [&result](const multisig::Transaction &one_transaction,
                             uint32_t transaction_id) mutable {
      result.push_back(one_transaction);
    };
    ClearTimeout(functor);
    return result;
  }

  /**
   * @brief Get detailed information about the transaction to be signed
   *
   * @param transaction_id Id of the transaction to be signed
   * @return Transaction details
   */
  CONST multisig::Transaction GetTransactionInfo(uint32_t transaction_id) {
    bool find = false;
    multisig::Transaction result;
    auto functor = [&](const multisig::Transaction &one_transaction,
                       uint32_t id) mutable {
      if (transaction_id == id) {
        find = true;
        result = one_transaction;
      }
    };

    ClearTimeout(functor);
    privacy_assert(find, "invalid transaction id");
    return result;
  }

  /**
   * @brief Commission signature pending transaction
   *
   * @param transaction_id Id of the transaction to be signed
   * @return Failure, trigger revert operation
   */
  ACTION void SignTransaction(uint32_t transaction_id) {
    auto functor = [](const multisig::Transaction &one, uint32_t id) {};
    ClearTimeout(functor);

    Address sender = platon_caller();
    std::vector<Address> owners;
    privacy::get_state((const uint8_t *)&multisig::KOwners,
                       sizeof(multisig::KOwners), owners);
    auto iter = std::find(owners.begin(), owners.end(), sender);
    privacy_assert(owners.end() != iter, "invalid sender");

    multisig::Transaction one_transaction;
    size_t size = get_state(transaction_id, one_transaction);
    privacy_assert(size > 0, "invalid transaction id");

    int64_t now = ::platon_timestamp();
    privacy_assert(now - one_transaction.timestamp_ <
                       int64_t(one_transaction.secondLimit_),
                   "out of second limit");

    privacy_assert(one_transaction.signedAddress_.end() ==
                       std::find(one_transaction.signedAddress_.begin(),
                                 one_transaction.signedAddress_.end(), sender),
                   "duplicate signature");
    one_transaction.signedAddress_.push_back(*iter);
    PLATON_EMIT_EVENT2(SignMultisigTransaction, platon_address(),
                       transaction_id, sender);

    uint16_t required = 0;
    platon_get_state((const uint8_t *)&multisig::KRequired,
                     sizeof(multisig::KRequired), (uint8_t *)&required,
                     sizeof(required));
    if (one_transaction.signedAddress_.size() >= required) {
      auto actuator =
          multisig::CreateActuator(one_transaction.to_, transaction_id);
      bool success = actuator->Execute(one_transaction.paras_);

      privacy_assert(success, "multisig transaction execute failed");

      del_state(transaction_id);
      PLATON_EMIT_EVENT2(MultisigTransactionExecute, platon_address(),
                         transaction_id, one_transaction.signedAddress_);
    } else {
      set_state(transaction_id, one_transaction);
    }
  }

 private:
  template <typename Function>
  void ClearTimeout(Function functor) {
    uint32_t transaction_id = 0;
    platon_get_state((const uint8_t *)&multisig::KTransactionId,
                     sizeof(multisig::KTransactionId),
                     (uint8_t *)&transaction_id, sizeof(transaction_id));

    int64_t now = ::platon_timestamp();
    uint32_t max_id = 0;

    for (uint32_t i = 0; i <= transaction_id; i++) {
      multisig::Transaction one_transaction;
      size_t size = get_state(i, one_transaction);
      if (size > 0) {
        if (now - one_transaction.timestamp_ >=
            int64_t(one_transaction.secondLimit_)) {
          del_state(i);
          PLATON_EMIT_EVENT2(MultisigTransactionTimeout, platon_address(), i);
        } else {
          max_id = i;
          functor(one_transaction, i);
        }
      }
    }

    platon_set_state((const uint8_t *)&multisig::KTransactionId,
                     sizeof(multisig::KTransactionId), (const uint8_t *)&max_id,
                     sizeof(max_id));
  }

  void LocalTransaction(uint32_t second_limit) {
    Address to = platon_address();

    size_t len = ::platon_get_input_length();
    bytes paras(len);
    ::platon_get_input(paras.data());

    PushTransaction(to, paras, second_limit);
  }
};

PLATON_DISPATCH(
    Multisig, (init)(ChangeOwnersAndRequired)(GetOwners)(GetRequired)(
                  CreateContract)(CloneContract)(PushTransaction)(
                  GetPendingTransactions)(GetTransactionInfo)(SignTransaction))