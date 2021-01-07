#pragma once
#include <platon/platon.h>
#include <memory>
#include "multisig/multisig_event.hpp"
#include "multisig/transaction.hpp"
#include "privacy/common.hpp"

namespace multisig {
class Actuator {
 public:
  virtual bool Execute(const bytes &paras) = 0;
  virtual ~Actuator() = default;
};

class Local : public Actuator, public MultisigEvent {
 public:
  Local(uint32_t transaction_id) : transactionId_(transaction_id) {}
  ~Local() = default;

  bool Execute(const bytes &paras) {
    RLP rlp(paras);
    uint64_t method = 0;
    fetch(rlp[0], method);

    bool success = false;
    switch (method) {
      case name_value("ChangeOwnersAndRequired"): {
        std::vector<Address> owners;
        fetch(rlp[1], owners);
        uint16_t required = 0;
        fetch(rlp[2], required);
        success = ChangeOwnersAndRequired(owners, required);
        break;
      }
      case name_value("CreateContract"): {
        bytes args;
        fetch(rlp[1], args);
        success = CreateContract(args);
        break;
      }
      case name_value("CloneContract"): {
        Address address;
        fetch(rlp[1], address);
        bytes args;
        fetch(rlp[2], args);
        success = CloneContract(address, args);
        break;
      }
      default:
        privacy_assert(true, "invalid parameter");
    }

    return success;
  }

 private:
  bool ChangeOwnersAndRequired(const std::vector<Address> &owners,
                               uint16_t required) {
    uint32_t transaction_id = 0;
    platon_get_state((const uint8_t *)&KTransactionId, sizeof(KTransactionId),
                     (uint8_t *)&transaction_id, sizeof(transaction_id));
    privacy_assert(0 == transaction_id || transaction_id == transactionId_,
                   "have pending transaction");

    privacy_assert(owners.size() > 0, "invalid owners");
    privacy_assert(required > 0 && owners.size() >= required,
                   "invalid required");

    privacy::set_state((const uint8_t *)&KOwners, sizeof(KOwners), owners);
    platon_set_state((const uint8_t *)&KRequired, sizeof(KRequired),
                     (const uint8_t *)&required, sizeof(required));

    PLATON_EMIT_EVENT1(ChangeMultisig, platon_address(), owners, required);
    return true;
  }

  bool CreateContract(const bytes &args) {
    bytes value_bytes = value_to_bytes(0U);
    bytes gas_bytes = value_to_bytes(::platon_gas());

    Address return_address;
    bool success =
        0 ==
        platon_deploy(return_address.data(), args.data(), args.size(),
                      value_bytes.data(), value_bytes.size(), gas_bytes.data(),
                      gas_bytes.size()) == 0;

    if (success) {
      PLATON_EMIT_EVENT2(MultisigCreate, platon_address(), transactionId_,
                         return_address);
    }

    return success;
  }

  bool CloneContract(const Address address, const bytes &args) {
    bytes value_bytes = value_to_bytes(0U);
    bytes gas_bytes = value_to_bytes(::platon_gas());

    Address return_address;
    bool success =
        platon_clone(address.data(), return_address.data(), args.data(),
                     args.size(), value_bytes.data(), value_bytes.size(),
                     gas_bytes.data(), gas_bytes.size()) == 0;

    if (success) {
      PLATON_EMIT_EVENT2(MultisigClone, platon_address(), transactionId_,
                         return_address);
    }

    return success;
  }

 private:
  uint32_t transactionId_;
};

class Remote : public Actuator {
 public:
  Remote(const Address &remote_address) : RemoteAddress_(remote_address) {}
  ~Remote() = default;
  bool Execute(const bytes &paras) {
    bytes value_bytes = value_to_bytes(0U);
    bytes gas_bytes = value_to_bytes(::platon_gas());
    return 0 == ::platon_call(RemoteAddress_.data(), paras.data(), paras.size(),
                              value_bytes.data(), value_bytes.size(),
                              gas_bytes.data(), gas_bytes.size());
  }

 private:
  Address RemoteAddress_;
};

std::unique_ptr<Actuator> CreateActuator(const Address &to,
                                         uint32_t transaction_id) {
  Actuator *actuator = nullptr;

  if (to == platon_address()) {
    actuator = new Local(transaction_id);
  } else {
    actuator = new Remote(to);
  }

  return std::unique_ptr<Actuator>(actuator);
}

}  // namespace multisig
